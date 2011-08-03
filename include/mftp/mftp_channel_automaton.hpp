#ifndef __mftp_channel_automaton_hpp__
#define __mftp_channel_automaton_hpp__

#include <ioa/udp_sender_automaton.hpp>
#include <ioa/udp_receiver_automaton.hpp>
#include <mftp/message.hpp>

#include <queue>

namespace mftp {

  class mftp_channel_automaton :
    public ioa::automaton,
    private ioa::observer
  {
  private:
    ioa::handle_manager<mftp_channel_automaton> m_self;
    typedef std::pair<ioa::const_shared_ptr<message_buffer>, ioa::aid_t> message_aid;
    std::list<message_aid > m_outgoing_messages;
    std::set<ioa::aid_t> m_outgoing_set;
    ioa::aid_t m_pending_aid;
    std::set<ioa::aid_t> m_outgoing_completes;
    const ioa::inet_address m_send;
    std::queue<ioa::const_shared_ptr<mftp::message> > m_incoming_messages;

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
    mftp_channel_automaton (const ioa::inet_address& send_address,
			    const ioa::inet_address& local_address,
			    const bool multicast);
  private:
    void schedule () const;
    void observe (ioa::observable* o);
    void purge (const ioa::aid_t aid);

    void send_effect (const ioa::const_shared_ptr<message_buffer>& message,
		      ioa::aid_t aid);
  public:
    V_AP_INPUT (mftp_channel_automaton, send, ioa::const_shared_ptr<message_buffer>);

  private:
    bool send_out_precondition () const;
    ioa::udp_sender_automaton::send_arg send_out_effect ();
    V_UP_OUTPUT (mftp_channel_automaton, send_out, ioa::udp_sender_automaton::send_arg);

    void send_in_complete_effect (const int& result);
    V_UP_INPUT (mftp_channel_automaton, send_in_complete, int);

  private:
    bool send_complete_precondition (ioa::aid_t aid) const;
    void send_complete_effect (ioa::aid_t aid);
  public:
    UV_AP_OUTPUT (mftp_channel_automaton, send_complete);

  private:
    void receive_in_effect (const ioa::udp_receiver_automaton::receive_val& rv);
    V_UP_INPUT (mftp_channel_automaton, receive_in, ioa::udp_receiver_automaton::receive_val);

    bool receive_precondition () const;
    ioa::const_shared_ptr<mftp::message> receive_effect ();
  public:
    V_UP_OUTPUT (mftp_channel_automaton, receive, ioa::const_shared_ptr<mftp::message>);
  };

}

#endif
