#ifndef __conversion_channel_automaton_hpp__
#define __conversion_channel_automaton_hpp__

#include "mftp.hpp"
#include <ioa/ioa.hpp>

#include <queue>

class conversion_channel_automaton :
  public ioa::automaton
{
private:
  std::queue<ioa::const_shared_ptr<mftp::message> > q;
  
public:
  conversion_channel_automaton ()
  {
    schedule ();
  }

private:
  void schedule () const {
    if (pass_message_precondition ()) {
      ioa::schedule (&conversion_channel_automaton::pass_message);
    }
  }

  void receive_buffer_effect (const ioa::udp_receiver_automaton::receive_val& rv) {
    if (rv.buffer.get () != 0 && rv.buffer->size () == sizeof (mftp::message)) {
      std::auto_ptr<mftp::message> m (new mftp::message);
      memcpy (m.get (), rv.buffer->data (), rv.buffer->size ());
      if (m.get ()->convert_to_host ()) {
	q.push (ioa::const_shared_ptr<mftp::message> (m.release ()));
      }
    }
  }

public:
  V_UP_INPUT (conversion_channel_automaton, receive_buffer, ioa::udp_receiver_automaton::receive_val);

private:
  bool pass_message_precondition () const {
    return !q.empty ()  && ioa::binding_count (&conversion_channel_automaton::pass_message) != 0;
  }

  ioa::const_shared_ptr<mftp::message> pass_message_effect () {
    ioa::const_shared_ptr<mftp::message> m = q.front ();
    q.pop ();
    return m;
  }

public:
  V_UP_OUTPUT (conversion_channel_automaton, pass_message, ioa::const_shared_ptr<mftp::message>);
};

#endif
