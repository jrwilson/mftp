#ifndef __mftp_automaton_hpp__
#define	__mftp_automaton_hpp__

#include <mftp/match.hpp>
#include <mftp/mftp_channel_automaton.hpp>
#include <ioa/alarm_automaton.hpp>
#include <mftp/interval_set.hpp>

#include <queue>
#include <set>

namespace mftp {
    
  class mftp_automaton :
    public ioa::automaton
  {
  private:
    static const ioa::time REQUEST_INTERVAL;
    static const ioa::time INIT_ANNOUNCEMENT_INTERVAL;
    static const ioa::time MAX_ANNOUNCEMENT_INTERVAL;
    static const ioa::time MATCHING_INTERVAL;
    static const ioa::time PROGRESS_INTERVAL;
    static const size_t MAX_FRAGMENT_COUNT;

    enum send_state_t {
      SEND_READY,
      SEND_COMPLETE_WAIT
    };

    enum timer_state_t {
      SET_READY,
      INTERRUPT_WAIT,
    };

    #define REQUEST_NUMERATOR 4
    #define REQUEST_DENOMINATOR 5
    
    ioa::handle_manager<mftp_automaton> m_self;
    file m_file;
    const mfileid& m_mfileid;
    const fileid& m_fileid;
    interval_set<uint32_t> m_requests; // Set of intervals indicating fragments that have been requested.
    uint32_t m_last_sent_idx; //Index of last fragment sent
    std::queue<ioa::const_shared_ptr<message_buffer> > m_sendq;
    send_state_t m_send_state;
    size_t m_fragment_count;
    bool m_send_request;
    bool m_send_announcement;
    bool m_send_match;
    timer_state_t m_request_timer_state;
    timer_state_t m_announcement_timer_state;
    timer_state_t m_matching_timer_state;
    timer_state_t m_progress_timer_state;
    ioa::time m_announcement_interval;
    bool m_reported; // True when we have reported a complete download.
    bool m_progress;
    uint32_t m_pending_requests;
    uint32_t m_since_received;

    ioa::handle_manager<mftp_channel_automaton> m_channel;

    const bool m_matching; // Try to find matches for this file.
    std::auto_ptr<match_candidate_predicate> m_match_candidate_predicate;
    std::auto_ptr<match_predicate> m_match_predicate;
    const bool m_get_matching_files; // Always get matching files.

    bool m_suicide_flag;  //Self-destruct when job is done.

    std::set<fileid> m_pending_matches;
    std::set<fileid> m_matches;
    std::set<fileid> m_non_matches;
    std::deque<fileid> m_matchq;

    std::queue<ioa::const_shared_ptr<file> > m_matching_files;

  public:
    // Not matching.
    mftp_automaton (const file& file,
		    const ioa::automaton_handle<mftp_channel_automaton>& channel,
		    const bool suicide);

    // Matching.
    mftp_automaton (const file& file,
		    const ioa::automaton_handle<mftp_channel_automaton>& channel,
		    const match_candidate_predicate& match_candidate_pred,
		    const match_predicate& match_pred,
		    const bool get_matching_files,
		    const bool suicide);

  private:
    void create_bindings ();
    void schedule () const;
    void process_match_candidate (const file& f);
    void advance_to_next_request_index ();
    message_buffer* get_fragment (uint32_t idx);

    bool send_precondition () const;
    ioa::const_shared_ptr<message_buffer> send_effect ();
    V_UP_OUTPUT (mftp_automaton, send, ioa::const_shared_ptr<message_buffer>);

    void send_complete_effect ();
    UV_UP_INPUT (mftp_automaton, send_complete);

    void receive_effect (const ioa::const_shared_ptr<message>& m);
    V_UP_INPUT (mftp_automaton, receive, ioa::const_shared_ptr<message>);

    bool send_fragment_precondition () const;
    void send_fragment_effect ();
    UP_INTERNAL (mftp_automaton, send_fragment);

    bool send_request_precondition () const;
    void send_request_effect ();
    UP_INTERNAL (mftp_automaton, send_request);

    bool send_announcement_precondition () const;
    void send_announcement_effect ();
    UP_INTERNAL (mftp_automaton, send_announcement);

    bool send_match_precondition () const;
    void send_match_effect ();
    UP_INTERNAL (mftp_automaton, send_match);

    bool set_request_timer_precondition () const;
    ioa::time set_request_timer_effect ();
    V_UP_OUTPUT (mftp_automaton, set_request_timer, ioa::time);

    void request_timer_interrupt_effect ();
    UV_UP_INPUT (mftp_automaton, request_timer_interrupt);

    bool set_announcement_timer_precondition () const;
    ioa::time set_announcement_timer_effect ();
    V_UP_OUTPUT (mftp_automaton, set_announcement_timer, ioa::time);
    
    void announcement_timer_interrupt_effect ();
    UV_UP_INPUT (mftp_automaton, announcement_timer_interrupt);

    bool set_match_timer_precondition () const;
    ioa::time set_match_timer_effect ();
    V_UP_OUTPUT (mftp_automaton, set_match_timer, ioa::time);

    void matching_timer_interrupt_effect ();
    UV_UP_INPUT (mftp_automaton, matching_timer_interrupt);

    bool set_progress_timer_precondition () const;
    ioa::time set_progress_timer_effect ();
    V_UP_OUTPUT (mftp_automaton, set_progress_timer, ioa::time);

    void progress_timer_interrupt_effect ();
    UV_UP_INPUT (mftp_automaton, progress_timer_interrupt);

    bool report_progress_precondition () const;
    uint32_t report_progress_effect ();
  public:
    V_UP_OUTPUT (mftp_automaton, report_progress, uint32_t);

  private:
    bool download_complete_precondition () const;
    file download_complete_effect ();
  public:
    V_UP_OUTPUT (mftp_automaton, download_complete, file);

  private: 
    bool suicide_precondition () const;
    void suicide_effect ();
    UP_INTERNAL (mftp_automaton, suicide);

    void match_download_complete_effect (const file& f,
					 ioa::aid_t aid);
    V_AP_INPUT (mftp_automaton, match_download_complete, file);

    bool match_complete_precondition () const;
    ioa::const_shared_ptr<file> match_complete_effect ();
  public:
    V_UP_OUTPUT (mftp_automaton, match_complete, ioa::const_shared_ptr<file>);
  };
}

#endif
