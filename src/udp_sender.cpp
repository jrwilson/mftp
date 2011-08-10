#include <iostream>
#include <ioa/ioa.hpp>
#include <ioa/global_fifo_scheduler.hpp>
#include <ioa/alarm_automaton.hpp>
#include <ioa/udp_sender_automaton.hpp>
#include <queue>
#include <config.hpp>

class udp_sender :
  public ioa::automaton
{
private:
  enum alarm_state_t {
    ALARM_SET,
    ALARM_WAIT
  };

  enum send_state_t {
    SEND_SEND,
    SEND_WAIT
  };

  ioa::handle_manager<udp_sender> m_self;
  alarm_state_t m_alarm_state;
  send_state_t m_send_state;
  const ioa::inet_address m_address;
  const ioa::time m_interval;
  const uint32_t m_message_size;
  uint32_t m_message_count;
  std::queue<ioa::const_shared_ptr<std::string> > m_sendq;

public:
  udp_sender (const ioa::inet_address& address,
	      const uint32_t message_rate,
	      const uint32_t message_size,
	      const uint32_t message_count) :
    m_self (ioa::get_aid ()),
    m_alarm_state (ALARM_SET),
    m_send_state (SEND_SEND),
    m_address (address),
    m_interval (0, 1000000 / message_rate),
    m_message_size (message_size),
    m_message_count (message_count)
  {
    ioa::automaton_manager<ioa::alarm_automaton>* alarm = new ioa::automaton_manager<ioa::alarm_automaton> (this, ioa::make_generator<ioa::alarm_automaton> ());
    ioa::make_binding_manager (this,
			       &m_self,
			       &udp_sender::set_alarm,
			       alarm,
			       &ioa::alarm_automaton::set);
    ioa::make_binding_manager (this,
			       alarm,
			       &ioa::alarm_automaton::alarm,
			       &m_self,
			       &udp_sender::alarm_interrupt);
    
    ioa::automaton_manager<ioa::udp_sender_automaton>* sender = new ioa::automaton_manager<ioa::udp_sender_automaton> (this, ioa::make_generator<ioa::udp_sender_automaton> (message_size));
    ioa::make_binding_manager (this,
			       &m_self, &udp_sender::send,
			       sender, &ioa::udp_sender_automaton::send);
    
    ioa::make_binding_manager (this,
			       sender, &ioa::udp_sender_automaton::send_complete,
			       &m_self, &udp_sender::send_complete);
 
  }

private:
  void schedule () const {
    if (set_alarm_precondition ()) {
      ioa::schedule (&udp_sender::set_alarm);
    }
    if (send_precondition ()) {
      ioa::schedule (&udp_sender::send);
    }
  }

  bool set_alarm_precondition () const {
    return m_message_count != 0 && m_alarm_state == ALARM_SET && ioa::binding_count (&udp_sender::set_alarm) != 0;
  }

  ioa::time set_alarm_effect () {
    m_alarm_state = ALARM_WAIT;
    return m_interval;
  }

  V_UP_OUTPUT (udp_sender, set_alarm, ioa::time);

  void alarm_interrupt_effect () {
    m_alarm_state = ALARM_SET;

    uint32_t t = htonl (m_message_count--);
    std::string* buf = new std::string (reinterpret_cast<char *> (&t), sizeof (t));
    buf->append ('\0', m_message_size - sizeof (t));
    m_sendq.push (ioa::const_shared_ptr<std::string> (buf));
    if (m_sendq.size () > 1) {
      std::cout << "Queuing: " << m_sendq.size () << std::endl;
    }
  }

  UV_UP_INPUT (udp_sender, alarm_interrupt);

  bool send_precondition () const {
    return !m_sendq.empty () && m_send_state == SEND_SEND && ioa::binding_count (&udp_sender::send) != 0;
  }

  ioa::udp_sender_automaton::send_arg send_effect () {
    m_send_state = SEND_WAIT;
    ioa::const_shared_ptr<std::string> m = m_sendq.front ();
    m_sendq.pop ();
    return ioa::udp_sender_automaton::send_arg (m_address, m);
  }

  V_UP_OUTPUT (udp_sender, send, ioa::udp_sender_automaton::send_arg);

  void send_complete_effect (const int& result) {
    if (result != 0) {
      char buf[256];
#ifdef STRERROR_R_CHAR_P
      std::cerr << "Couldn't send udp_sender_automaton: " << strerror_r (result, buf, 256) << std::endl;
#else
      strerror_r (result, buf, 256);
      std::cerr << "Couldn't send udp_sender_automaton: " << buf << std::endl;
#endif
      exit(EXIT_FAILURE);
    }
    m_send_state = SEND_SEND;
  }
  
  V_UP_INPUT (udp_sender, send_complete, int);
};

int main (int argc,
	  char* argv[]) {
  ioa::inet_address address ("224.0.0.137", 54321);

  uint32_t message_rate = 100; // per second
  uint32_t message_size = 560; // bytes
  uint32_t message_count = 1000;

  if (argc == 2) {
    message_rate = atoi (argv[1]);
  }

  ioa::global_fifo_scheduler sched;
  ioa::run (sched, ioa::make_generator<udp_sender> (address, message_rate, message_size, message_count));

  return 0;
}
