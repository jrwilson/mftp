#ifndef __mftp_receiver_automaton_hpp__
#define __mftp_receiver_automaton_hpp__

#include <mftp/mftp.hpp>
#include <ioa/ioa.hpp>
#include <ioa/udp_receiver_automaton.hpp>

#include <queue>

namespace mftp {

  class mftp_receiver_automaton :
    public ioa::automaton
  {
  private:
    ioa::handle_manager<mftp_receiver_automaton> m_self;
    std::queue<ioa::const_shared_ptr<mftp::message> > q;
  
  public:
    mftp_receiver_automaton (ioa::inet_address local_address, ioa::inet_address multicast_address) :
      m_self (ioa::get_aid ())
    {
      ioa::automaton_manager<ioa::udp_receiver_automaton>* receiver = new ioa::automaton_manager<ioa::udp_receiver_automaton> (this, ioa::make_generator<ioa::udp_receiver_automaton> (multicast_address, local_address));

      ioa::make_binding_manager (this,
      				 receiver, &ioa::udp_receiver_automaton::receive,
      				 &m_self, &mftp_receiver_automaton::receive_in);


      schedule ();
    }

    mftp_receiver_automaton (ioa::inet_address local_address) :
      m_self (ioa::get_aid ())
    {
      ioa::automaton_manager<ioa::udp_receiver_automaton>* receiver = new ioa::automaton_manager<ioa::udp_receiver_automaton> (this, ioa::make_generator<ioa::udp_receiver_automaton> (local_address));

      ioa::make_binding_manager (this,
      				 receiver, &ioa::udp_receiver_automaton::receive,
      				 &m_self, &mftp_receiver_automaton::receive_in);


      schedule ();
    }

  private:
    void schedule () const {
      if (receive_precondition ()) {
	ioa::schedule (&mftp_receiver_automaton::receive);
      }
    }

    void receive_in_effect (const ioa::udp_receiver_automaton::receive_val& rv) {
      if (rv.buffer.get () != 0 && rv.buffer->size () == sizeof (mftp::message)) {
	std::auto_ptr<mftp::message> m (new mftp::message);
	memcpy (m.get (), rv.buffer->data (), rv.buffer->size ());
	if (m.get ()->convert_to_host ()) {
	  q.push (ioa::const_shared_ptr<mftp::message> (m.release ()));
	}
      }
    }

    V_UP_INPUT (mftp_receiver_automaton, receive_in, ioa::udp_receiver_automaton::receive_val);

    bool receive_precondition () const {
      return !q.empty ()  && ioa::binding_count (&mftp_receiver_automaton::receive) != 0;
    }

    ioa::const_shared_ptr<mftp::message> receive_effect () {
      ioa::const_shared_ptr<mftp::message> m = q.front ();
      q.pop ();
      return m;
    }

  public:
    V_UP_OUTPUT (mftp_receiver_automaton, receive, ioa::const_shared_ptr<mftp::message>);
  };

}
#endif
