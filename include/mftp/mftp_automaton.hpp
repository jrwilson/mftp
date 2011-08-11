#ifndef __mftp_automaton_hpp__
#define	__mftp_automaton_hpp__

#include <mftp/match.hpp>
#include <mftp/mftp_channel_automaton.hpp>
#include <ioa/alarm_automaton.hpp>

#include <queue>
#include <set>

namespace mftp {
    
  class mftp_automaton :
    public ioa::automaton
  {
  private:
    enum send_state_t {
      SEND_READY,
      SEND_COMPLETE_WAIT
    };

    enum alarm_state_t {
      SET_READY,
      INTERRUPT_WAIT,
    };

    static const ioa::time ALARM_INTERVAL;
    static const ioa::time INIT_INTERVAL;
    static const ioa::time MAX_INTERVAL;
    static const uint32_t MAX_FRAGMENT_COUNT;
    static const uint32_t SHUFFLE_LIMIT;
    static const uint32_t REREQUEST_NUMERATOR;
    static const uint32_t REREQUEST_DENOMINATOR;

    ioa::handle_manager<mftp_automaton> m_self;
    ioa::const_shared_ptr<file> m_file;
    file* m_file_ptr;
    const mfileid& m_mfileid;
    const fileid& m_fileid;

    ioa::handle_manager<mftp_channel_automaton> m_channel; // The channel for sending/receiving.

    // Sending.
    std::queue<ioa::const_shared_ptr<std::string> > m_sendq; // Send queue.
    send_state_t m_send_state; // State of send state machine.
    uint32_t m_num_frag_in_sendq; // Number of fragments in the send queue.
    uint32_t m_num_req_in_sendq; // Number of requests in the send queue.
    uint32_t m_num_match_in_sendq; // Number of matches in the send queue.

    // Answering requests.
    std::set<uint32_t> m_requests_set; // Set of fragments that have been requested.
    std::deque<uint32_t> m_requests_deque; // Superset of set organized as deque.
    uint32_t m_inserts_since_shuffle; // Number of inserts since shuffle.

    // Making requests.
    uint32_t m_request_idx; // Index for requests.
    uint32_t m_last_request_size; // Number of fragments in last request (<= REQUEST_SIZE).
    uint32_t m_fragments_since_request; // Number of fragments received since request.

    // Timestamps for certain events.
    ioa::time m_frag_recv_time; // Time when this automaton last received a fragment (of this file).
    ioa::time m_request_timeout_start; // Time when this automaton last sent a request or received a fragment (of this file).
    ioa::time m_match_time; // Time when this automaton last received a match (of this file).

    // Periodic acitivities.
    alarm_state_t m_alarm_state; // State of alarm state machine.
    ioa::time m_announcement_interval; // Must wait this amount of time after m_fragment_time to send an announcement.
    ioa::time m_request_interval; // Must wait this amount of time after m_request_time to send a request for timeout.
    ioa::time m_match_interval; // Must wait this amount of time after m_match_time to send a match.

    // Reporting progress.
    uint32_t m_fragments_since_report; // Number of fragments received since last progress report.
    uint32_t m_progress_threshold; // Accumulate this many fragments before reporting progress.

    // Matching.
    const bool m_matching; // Try to find matches for this file.
    std::auto_ptr<match_candidate_predicate> m_match_candidate_predicate;
    std::auto_ptr<match_predicate> m_match_predicate;
    const bool m_get_matching_files; // Always get matching files.
    std::set<fileid> m_pending_matches; // Fileids that are pending a download for matching.
    std::set<fileid> m_matches; // Fileids that match.
    std::set<fileid> m_non_matches; // Fileids that don't match.
    std::queue<ioa::const_shared_ptr<file> > m_matching_files; // Queue of matching files.

    // Termination.
    bool m_suicide_flag;  // Self-destruct when job is done.
    bool m_reported; // True when we have reported a complete download.

  public:
    // Not matching.
    mftp_automaton (std::auto_ptr<file> file,
		    const ioa::automaton_handle<mftp_channel_automaton>& channel,
		    const bool suicide,
		    const uint32_t progress_threshold);

    mftp_automaton (const ioa::const_shared_ptr<file>& file,
		    const ioa::automaton_handle<mftp_channel_automaton>& channel,
		    const bool suicide,
		    const uint32_t progress_threshold);

    // Matching.
    mftp_automaton (std::auto_ptr<file> file,
		    const ioa::automaton_handle<mftp_channel_automaton>& channel,
		    const match_candidate_predicate& match_candidate_pred,
		    const match_predicate& match_pred,
		    const bool get_matching_files,
		    const bool suicide,
		    const uint32_t progress_threshold);

    mftp_automaton (const ioa::const_shared_ptr<file>& file,
		    const ioa::automaton_handle<mftp_channel_automaton>& channel,
		    const match_candidate_predicate& match_candidate_pred,
		    const match_predicate& match_pred,
		    const bool get_matching_files,
		    const bool suicide,
		    const uint32_t progress_threshold);

  private:
    void create_bindings ();
    void schedule () const;
    void process_match_candidate (const ioa::const_shared_ptr<file>& f);
    std::string* get_fragment (uint32_t idx);
    void send_announcement ();
    void send_request ();
    void send_match (bool reset);
    void add_match (const fileid& fid);

    bool send_precondition () const;
    ioa::const_shared_ptr<std::string> send_effect ();
    V_UP_OUTPUT (mftp_automaton, send, ioa::const_shared_ptr<std::string>);

    void send_complete_effect ();
    UV_UP_INPUT (mftp_automaton, send_complete);

    void receive_effect (const ioa::const_shared_ptr<message>& m);
    V_UP_INPUT (mftp_automaton, receive, ioa::const_shared_ptr<message>);

    bool set_alarm_precondition () const;
    ioa::time set_alarm_effect ();
    V_UP_OUTPUT (mftp_automaton, set_alarm, ioa::time);

    void alarm_interrupt_effect ();
    UV_UP_INPUT (mftp_automaton, alarm_interrupt);

    bool send_fragment_precondition () const;
    void send_fragment_effect ();
    UP_INTERNAL (mftp_automaton, send_fragment);

    bool suicide_precondition () const;
    void suicide_effect ();
    UP_INTERNAL (mftp_automaton, suicide);

    void match_download_complete_effect (const ioa::const_shared_ptr<file>& f,
					 ioa::aid_t aid);
    V_AP_INPUT (mftp_automaton, match_download_complete, ioa::const_shared_ptr<file>);

  private:
    bool fragment_count_precondition () const;
    uint32_t fragment_count_effect ();
  public:
    V_UP_OUTPUT (mftp_automaton, fragment_count, uint32_t);

  private:
    bool download_complete_precondition () const;
    ioa::const_shared_ptr<file> download_complete_effect ();
  public:
    V_UP_OUTPUT (mftp_automaton, download_complete, ioa::const_shared_ptr<file>);

  private: 
    bool match_complete_precondition () const;
    ioa::const_shared_ptr<file> match_complete_effect ();
  public:
    V_UP_OUTPUT (mftp_automaton, match_complete, ioa::const_shared_ptr<file>);
  };
}

#endif
