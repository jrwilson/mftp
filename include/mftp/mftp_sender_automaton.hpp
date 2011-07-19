#ifndef __mftp_sender_automaton_hpp__
#define __mftp_sender_automaton_hpp__

#include <ioa/udp_sender_automaton.hpp>
#include <mftp/message.hpp>

#include <queue>
#include <iostream>

namespace mftp {

  class mftp_sender_automaton :
    public ioa::automaton,
    private ioa::observer
  {
  private:
    ioa::handle_manager<mftp_sender_automaton> m_self;
    typedef std::pair<ioa::const_shared_ptr<message_buffer>, ioa::aid_t> message_aid;
    std::list<message_aid > m_outgoing_messages;
    std::set<ioa::aid_t> m_outgoing_set;
    ioa::aid_t m_pending_aid;
    std::set<ioa::aid_t> m_outgoing_completes;
    const ioa::inet_address m_send;

    struct message_aid_equal {
      const ioa::aid_t m_aid;
      
      message_aid_equal (const ioa::aid_t aid) :
	m_aid (aid)
      { }
      
      bool operator() (const message_aid& o) const {
	return m_aid == o.second;
      }
    };

  public:
    mftp_sender_automaton (ioa::inet_address send_address) :
      m_self (ioa::get_aid ()),
      m_pending_aid (-1),
      m_send (send_address)
    {
      add_observable (&send);
      add_observable (&send_complete);

      ioa::automaton_manager<ioa::udp_sender_automaton>* sender = new ioa::automaton_manager<ioa::udp_sender_automaton> (this, ioa::make_generator<ioa::udp_sender_automaton> (sizeof (message)));

      ioa::make_binding_manager (this,
				 &m_self, &mftp_sender_automaton::send_out,
				 sender, &ioa::udp_sender_automaton::send);

      ioa::make_binding_manager (this,
				 sender, &ioa::udp_sender_automaton::send_complete,
				 &m_self, &mftp_sender_automaton::send_in_complete);
    }

  private:
    void schedule () const {
      if (send_out_precondition ()) {
	ioa::schedule (&mftp_sender_automaton::send_out);
      }
      for (std::set<ioa::aid_t>::const_iterator pos = m_outgoing_completes.begin ();
	   pos != m_outgoing_completes.end ();
	   ++pos) {
	if (send_complete_precondition (*pos)) {
	  ioa::schedule (&mftp_sender_automaton::send_complete, *pos);
	}
      }
    }

    void observe (ioa::observable* o) {
      if (o == &send && send.recent_op == ioa::UNBOUND) {
	purge (send.recent_parameter);
      }
      else if (o == &send_complete && send_complete.recent_op == ioa::UNBOUND) {
	purge (send_complete.recent_parameter);
      }
    }

    void purge (const ioa::aid_t aid) {
      if (m_outgoing_set.count (aid) != 0) {
	std::list<message_aid>::iterator pos = std::find_if (m_outgoing_messages.begin (),
							     m_outgoing_messages.end (),
							     message_aid_equal (aid));
	assert (pos != m_outgoing_messages.end ());
	m_outgoing_messages.erase (pos);
	m_outgoing_set.erase (aid);
      }

      if (m_pending_aid == aid) {
	m_pending_aid = -1;
      }
      
      m_outgoing_completes.erase (aid);
    }

    void send_effect (const ioa::const_shared_ptr<message_buffer>& message,
		      ioa::aid_t aid) {
      if (m_outgoing_set.count (aid) == 0 &&
	  m_pending_aid != aid &&
	  m_outgoing_completes.count (aid) == 0) {
	m_outgoing_messages.push_back (std::make_pair (message, aid));
	m_outgoing_set.insert (aid);
      }
    }

  public:
    V_AP_INPUT (mftp_sender_automaton, send, ioa::const_shared_ptr<message_buffer>);

  private:
    bool send_out_precondition () const {
      return m_pending_aid == -1 && !m_outgoing_messages.empty () && ioa::binding_count (&mftp_sender_automaton::send_out) != 0;
    }

    ioa::udp_sender_automaton::send_arg send_out_effect () {
      message_aid m = m_outgoing_messages.front ();
      m_outgoing_messages.pop_front ();
      m_pending_aid = m.second;
      m_outgoing_set.erase (m_pending_aid);
      return ioa::udp_sender_automaton::send_arg (m_send, m.first);
    }

    V_UP_OUTPUT (mftp_sender_automaton, send_out, ioa::udp_sender_automaton::send_arg);

    void send_in_complete_effect (const int& result) {
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
      else {
	m_outgoing_completes.insert (m_pending_aid);
	m_pending_aid = -1;
      }
    }

    V_UP_INPUT (mftp_sender_automaton, send_in_complete, int);

  private:
    bool send_complete_precondition (ioa::aid_t aid) const {
      return m_outgoing_completes.count (aid) != 0 && ioa::binding_count (&mftp_sender_automaton::send_complete, aid) != 0;
    }

    void send_complete_effect (ioa::aid_t aid) {
      m_outgoing_completes.erase (aid);
    }

  public:
    UV_AP_OUTPUT (mftp_sender_automaton, send_complete);

  };

}

#endif
