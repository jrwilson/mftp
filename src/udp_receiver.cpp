#include <ioa/ioa.hpp>
#include <ioa/udp_receiver_automaton.hpp>
#include <ioa/global_fifo_scheduler.hpp>

#include <config.hpp>

#include <iostream>

class udp_receiver :
  public ioa::automaton
{
private:
  ioa::handle_manager<udp_receiver> m_self;
  ioa::inet_address m_group;
  ioa::inet_address m_address;

public:
  udp_receiver (const ioa::inet_address& group,
		const ioa::inet_address& address) :
    m_self (ioa::get_aid ()),
    m_group (group),
    m_address (address)
  {
    ioa::automaton_manager<ioa::udp_receiver_automaton>* receiver = new ioa::automaton_manager<ioa::udp_receiver_automaton> (this, ioa::make_generator<ioa::udp_receiver_automaton> (m_group, m_address));

    ioa::make_binding_manager (this, receiver, &ioa::udp_receiver_automaton::receive, &m_self, &udp_receiver::receive);
  }

private:
  void schedule () const { }

  void receive_effect (const ioa::udp_receiver_automaton::receive_val& v) {
    if (v.err != 0) {
      char buf[256];
#ifdef STRERROR_R_CHAR_P
      std::cerr << "Couldn't receive udp_receiver_automaton: " << strerror_r (v.err, buf, 256) << std::endl;
#else
      strerror_r (v.err, buf, 256);
      std::cerr << "Couldn't receive udp_receiver_automaton: " << buf << std::endl;
#endif
      self_destruct ();
    }
    else {
      assert (v.buffer.get () != 0);
      uint32_t idx;
      memcpy (&idx, v.buffer->data (), sizeof (idx));
      idx = ntohl (idx);
      std::cout << idx << std::endl;
    }
  }
  
  V_UP_INPUT (udp_receiver, receive, ioa::udp_receiver_automaton::receive_val);

};

int
main (int argc, char* argv[]) {
  ioa::inet_address group ("224.0.0.137");
  ioa::inet_address address ("0.0.0.0", 54321);

  ioa::global_fifo_scheduler ss;
  ioa::run (ss, ioa::make_generator<udp_receiver> (group, address));

  return 0; 
}

