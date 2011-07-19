#ifndef __mftp_automaton_hpp__
#define	__mftp_automaton_hpp__

#include <mftp/file.hpp>

#include <mftp/mftp_sender_automaton.hpp>
#include <mftp/mftp_receiver_automaton.hpp>
#include <ioa/alarm_automaton.hpp>

#include <config.hpp>
#include <cstdlib>
#include <queue>
#include <set>
#include <vector>
#include <cstring>

// TODO:  We accumulate matches without ever forgetting them.  We should purge matches that we haven't seen in a while.

namespace mftp {
  
  struct match_candidate_predicate {
    virtual ~match_candidate_predicate () { }
    virtual bool operator() (const fileid& fid) const = 0;
    virtual match_candidate_predicate* clone () const = 0;
  };
  
  struct match_predicate {
    virtual ~match_predicate () { }
    virtual bool operator() (const file& f) const = 0;
    virtual match_predicate* clone () const = 0;
  };
  
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

    ioa::handle_manager<mftp_automaton> m_self;
    file m_file;
    const mfileid& m_mfileid;
    const fileid& m_fileid;
    std::vector<bool> m_requests; // Bit vector indicating fragments that have been requested.
    uint32_t m_requests_count; // Number of true elements in m_requests.
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

    ioa::handle_manager<mftp_sender_automaton> m_sender;
    ioa::handle_manager<mftp_receiver_automaton> m_receiver;

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

    void create_bindings () {
      ioa::make_binding_manager (this,
				 &m_self, &mftp_automaton::send,
				 &m_sender, &mftp_sender_automaton::send);
      
      ioa::make_binding_manager (this,
				 &m_sender, &mftp_sender_automaton::send_complete,
				 &m_self, &mftp_automaton::send_complete);
      
      ioa::make_binding_manager (this,
				 &m_receiver, &mftp_receiver_automaton::receive,
				 &m_self, &mftp_automaton::receive);

      ioa::automaton_manager<ioa::alarm_automaton>* request_clock = new ioa::automaton_manager<ioa::alarm_automaton> (this, ioa::make_generator<ioa::alarm_automaton> ());
      ioa::make_binding_manager (this,
			     &m_self,
			     &mftp_automaton::set_request_timer,
			     request_clock,
			     &ioa::alarm_automaton::set);
      ioa::make_binding_manager (this,
			     request_clock,
			     &ioa::alarm_automaton::alarm,
			     &m_self,
			     &mftp_automaton::request_timer_interrupt);
    
      ioa::automaton_manager<ioa::alarm_automaton>* announcement_clock = new ioa::automaton_manager<ioa::alarm_automaton> (this, ioa::make_generator<ioa::alarm_automaton> ());
      ioa::make_binding_manager (this,
			     &m_self,
			     &mftp_automaton::set_announcement_timer,
			     announcement_clock,
			     &ioa::alarm_automaton::set);
      ioa::make_binding_manager (this,
			     announcement_clock,
			     &ioa::alarm_automaton::alarm,
			     &m_self,
			     &mftp_automaton::announcement_timer_interrupt);

      if (m_matching) {
	ioa::automaton_manager<ioa::alarm_automaton>* matching_clock = new ioa::automaton_manager<ioa::alarm_automaton> (this, ioa::make_generator<ioa::alarm_automaton> ());
	ioa::make_binding_manager (this,
				   &m_self,
				   &mftp_automaton::set_match_timer,
				   matching_clock,
				   &ioa::alarm_automaton::set);
	ioa::make_binding_manager (this,
				   matching_clock,
				   &ioa::alarm_automaton::alarm,
				   &m_self,
				   &mftp_automaton::matching_timer_interrupt);
      }

      ioa::automaton_manager<ioa::alarm_automaton>* progress_clock = new ioa::automaton_manager<ioa::alarm_automaton> (this, ioa::make_generator<ioa::alarm_automaton> ());
      ioa::make_binding_manager (this,
				 &m_self,
				 &mftp_automaton::set_progress_timer,
				 progress_clock,
				 &ioa::alarm_automaton::set);
      ioa::make_binding_manager (this,
				 progress_clock,
				 &ioa::alarm_automaton::alarm,
				 &m_self,
				 &mftp_automaton::progress_timer_interrupt);
      
      schedule ();
    }

  public:
    // Not matching.
    mftp_automaton (const file& file,
		    const ioa::automaton_handle<mftp_sender_automaton>& sender,
		    const ioa::automaton_handle<mftp_receiver_automaton>& receiver,
		    const bool suicide) :
      m_self (ioa::get_aid ()),
      m_file (file),
      m_mfileid (file.get_mfileid ()),
      m_fileid (m_mfileid.get_fileid ()),
      m_requests (m_mfileid.get_fragment_count ()),
      m_requests_count (0),
      m_send_state (SEND_READY),
      m_fragment_count (0),
      m_send_request (!m_file.complete ()),
      m_send_announcement (!m_file.empty ()),
      m_send_match (false),
      m_request_timer_state (SET_READY),
      m_announcement_timer_state (SET_READY),
      m_matching_timer_state (SET_READY),
      m_announcement_interval (INIT_ANNOUNCEMENT_INTERVAL),
      m_reported (m_file.complete ()),
      m_progress (false),
      m_sender (sender),
      m_receiver (receiver),
      m_matching (false),
      m_get_matching_files (false),
      m_suicide_flag (suicide)
    {
      create_bindings ();
      std::cout << "Created " << this << std::endl;
    }

    // Matching.
    mftp_automaton (const file& file,
		    const ioa::automaton_handle<mftp_sender_automaton>& sender,
		    const ioa::automaton_handle<mftp_receiver_automaton>& receiver,
		    const match_candidate_predicate& match_candidate_pred,
		    const match_predicate& match_pred,
		    const bool get_matching_files,
		    const bool suicide) :
      m_self (ioa::get_aid ()),
      m_file (file),
      m_mfileid (file.get_mfileid ()),
      m_fileid (m_mfileid.get_fileid ()),
      m_requests (m_mfileid.get_fragment_count ()),
      m_requests_count (0),
      m_send_state (SEND_READY),
      m_fragment_count (0),
      m_send_request (!m_file.complete ()),
      m_send_announcement (!m_file.empty ()),
      m_send_match (false),
      m_request_timer_state (SET_READY),
      m_announcement_timer_state (SET_READY),
      m_matching_timer_state (SET_READY),
      m_announcement_interval (INIT_ANNOUNCEMENT_INTERVAL),
      m_reported (m_file.complete ()),
      m_progress (false),
      m_sender (sender),
      m_receiver (receiver),
      m_matching (true),
      m_match_candidate_predicate (match_candidate_pred.clone ()),
      m_match_predicate (match_pred.clone ()),
      m_get_matching_files (get_matching_files),
      m_suicide_flag (suicide)
    {
      create_bindings ();
      std::cout << "Created " << this << std::endl;
    }

  private:
    void schedule () const {
      if (send_precondition ()) {
	ioa::schedule (&mftp_automaton::send);
      }
      if (send_fragment_precondition ()) {
	ioa::schedule (&mftp_automaton::send_fragment);
      }
      if (send_request_precondition ()) {
	ioa::schedule (&mftp_automaton::send_request);
      }
      if (send_announcement_precondition ()) {
	ioa::schedule (&mftp_automaton::send_announcement);
      }
      if (send_match_precondition ()) {
	ioa::schedule (&mftp_automaton::send_match);
      }
      if (set_request_timer_precondition ()) {
	ioa::schedule (&mftp_automaton::set_request_timer);
      }
      if (set_announcement_timer_precondition ()) {
	ioa::schedule (&mftp_automaton::set_announcement_timer);
      }
      if (set_match_timer_precondition ()) {
	ioa::schedule (&mftp_automaton::set_match_timer);
      }
      if (match_complete_precondition ()){
	ioa::schedule (&mftp_automaton::match_complete);
      }
      if (download_complete_precondition ()) {
	ioa::schedule (&mftp_automaton::download_complete);
      }
      if (suicide_precondition ()) {
	ioa::schedule (&mftp_automaton::suicide);
      }
      if (set_progress_timer_precondition ()){
	ioa::schedule (&mftp_automaton::set_progress_timer);
      }
      if (report_progress_precondition ()) {
	ioa::schedule (&mftp_automaton::report_progress);
      }
    }

    bool send_precondition () const {
      return !m_sendq.empty () && m_send_state == SEND_READY && ioa::binding_count (&mftp_automaton::send) != 0;
    }

    ioa::const_shared_ptr<message_buffer> send_effect () {
      ioa::const_shared_ptr<message_buffer> m = m_sendq.front ();
      m_sendq.pop ();
      if (m->msg.header.message_type == htonl (FRAGMENT)) {
	--m_fragment_count;
      }

      m_send_state = SEND_COMPLETE_WAIT;
      return m;
    }
    
  public:
    V_UP_OUTPUT (mftp_automaton, send, ioa::const_shared_ptr<message_buffer>);

  private:
    void send_complete_effect () {
      m_send_state = SEND_READY;
    }

  public:
    UV_UP_INPUT (mftp_automaton, send_complete);

  private:
    void receive_effect (const ioa::const_shared_ptr<message>& m) {
      switch (m->header.message_type) {
      case FRAGMENT:
	{
	  // If we are looking for our own file, it must be a fragment from our file and the offset must be correct.
	  if (m->frag.fid == m_fileid &&
	      m->frag.offset < m_mfileid.get_final_length () &&
	      m->frag.offset % FRAGMENT_SIZE == 0) {
	    
	    // Save the fragment.
	    if (!m_file.complete ()){
	      m_file.write_chunk (m->frag.offset, m->frag.data);
	    }
	    uint32_t idx = (m->frag.offset / FRAGMENT_SIZE);
	    if (m_requests[idx]) {
	      // Somebody else wanted it and now has just seen it.
	      m_requests[idx] = false;
	      --m_requests_count;
	    }
	  }

	  // Otherwise, we could be looking for files that might match our file.
	  // If we are matching, we have not already checked this one AND it is interesting:
	  if (m_matching &&
	      m_pending_matches.count (m->frag.fid) == 0 &&
	      m_matches.count (m->frag.fid) == 0 &&
	      m_non_matches.count (m->frag.fid) == 0 &&
	      (*m_match_candidate_predicate) (m->frag.fid)) {

	    m_pending_matches.insert (m->frag.fid);

	    file f (m->frag.fid);
	    f.write_chunk (m->frag.offset, m->frag.data);
	    if (f.complete()) {
	      // We received the whole file.
	      process_match_candidate (f);
	    }
	    else {
	      // Create an mftp_automaton with MATCHING FALSE and PROGRESS FALSE to download other file.
	      // Perform matching when the download is complete.
	      ioa::automaton_manager<mftp_automaton>* new_file_home = new ioa::automaton_manager<mftp_automaton> (this, ioa::make_generator<mftp_automaton> (f, m_sender.get_handle(), m_receiver.get_handle(), true));
	      
	      ioa::make_binding_manager (this,
					 new_file_home, &mftp_automaton::download_complete,
					 &m_self, &mftp_automaton::match_download_complete);
	    }
	  } 
	}
	break;
	
      case REQUEST:
	{
	  //Requests must be for our file.
	  if (m->req.fid == m_fileid){
	    for (uint32_t sp = 0; sp < m->req.span_count; sp++){
	      // Request must be in range.
	      if (m->req.spans[sp].start % FRAGMENT_SIZE == 0 &&
		  m->req.spans[sp].stop % FRAGMENT_SIZE == 0 &&
		  m->req.spans[sp].start < m->req.spans[sp].stop &&
		  m->req.spans[sp].stop <= m_mfileid.get_final_length ()) {
		// We have it.
		for (uint32_t offset = m->req.spans[sp].start;
		     offset < m->req.spans[sp].stop;
		     offset += FRAGMENT_SIZE) {
		  uint32_t idx = offset / FRAGMENT_SIZE;
		  if (m_file.have (offset) && !m_requests[idx]) {
		    m_requests[idx] = true;
		    ++m_requests_count;
		  }
		}
	      }
	    }
	  }
	}
	break;

      case MATCH:
	{
	  if (m_matching) {
	    if (m->mat.fid != m_fileid) {
	      // Search for our fileid in the set of matches.
	      for (uint32_t idx = 0; idx < m->mat.match_count; ++idx) {
		if (m->mat.matches[idx] == m_fileid &&
		    m_pending_matches.count (m->mat.fid) == 0 &&
		    m_matches.count (m->mat.fid) == 0 &&
		    m_non_matches.count (m->mat.fid) == 0 &&
		    (*m_match_candidate_predicate) (m->mat.fid)) {
		  // We have never seen this before.
		  if (m_get_matching_files) {
		    // We need to get the file.
		    m_pending_matches.insert (m->mat.fid);

		    // Create an mftp_automaton with MATCHING FALSE to download other file.
		    // Perform matching when the download is complete.
		    file f (m->mat.fid);
		    ioa::automaton_manager<mftp_automaton>* new_file_home = new ioa::automaton_manager<mftp_automaton> (this, ioa::make_generator<mftp_automaton> (f, m_sender.get_handle(), m_receiver.get_handle(), true));
		    
		    ioa::make_binding_manager (this,
		  			       new_file_home, &mftp_automaton::download_complete,
		  			       &m_self, &mftp_automaton::match_download_complete);
		  }
		  else {
		    // We can just add it to the set.
		    m_matches.insert (m->mat.fid);
		  }
		}
	      }
	    }
	    else {
	      // Add all matches in the set.
	      for (uint32_t idx = 0; idx < m->mat.match_count; ++idx) {
		if (m_pending_matches.count (m->mat.matches[idx]) == 0 &&
		    m_matches.count (m->mat.matches[idx]) == 0 &&
		    m_non_matches.count (m->mat.matches[idx]) == 0 &&
		    (*m_match_candidate_predicate) (m->mat.matches[idx])) {
		  // We have never seen this before.
		  if (m_get_matching_files) {
		    // We need to get the file.
		    m_pending_matches.insert (m->mat.matches[idx]);

		    // Create an mftp_automaton with MATCHING FALSE to download other file.
		    // Perform matching when the download is complete.
		    file f (m->mat.matches[idx]);
		    ioa::automaton_manager<mftp_automaton>* new_file_home = new ioa::automaton_manager<mftp_automaton> (this, ioa::make_generator<mftp_automaton> (f, m_sender.get_handle(), m_receiver.get_handle(), true));
		    
		    ioa::make_binding_manager (this,
		  			       new_file_home, &mftp_automaton::download_complete,
		  			       &m_self, &mftp_automaton::match_download_complete);
		  }
		  else {
		    // We can just add it to the set.
		    m_matches.insert (m->mat.matches[idx]);
		  }
		}
	      }
	    }
	  }
	}
	break;

      default:
	// Unknown message type.
	break;
      }
      
    }
    
  public:

    V_UP_INPUT (mftp_automaton, receive, ioa::const_shared_ptr<message>);

  private:
    bool send_fragment_precondition () const {
      return m_requests_count != 0 && m_fragment_count < MAX_FRAGMENT_COUNT;
    }

    void send_fragment_effect () {
      // Purpose is to produce a randomly selected requested fragment.
      // Get a random index.
      uint32_t randy = get_random_request_index ();
      m_requests[randy] = false;
      --m_requests_count;
      
      // Get the fragment for that index.
      message_buffer* m = get_fragment (randy);
      m->convert_to_network ();
      m_sendq.push (ioa::const_shared_ptr<message_buffer> (m));
      ++m_fragment_count;
    }

    UP_INTERNAL (mftp_automaton, send_fragment);

    bool send_request_precondition () const {
      return m_send_request;
    }

    void send_request_effect () {
      
      if (!m_file.complete ()) {
	span_t spans[SPANS_SIZE];
	spans[0] = m_file.get_next_range();
	uint32_t sp_count = 1;
	bool looking = true;
	while (looking){
	  span_t range = m_file.get_next_range ();
	  if (range.start == spans[0].start || sp_count == SPANS_SIZE){ //when we've come back to the range we started on, or we have as many ranges as we can hold
	    looking = false;
	  }
	  else {
	    spans[sp_count] = range;
	    ++sp_count;
	  }
	}
	
	message_buffer* m = new message_buffer (request_type (), m_fileid, sp_count, spans);
	m->convert_to_network ();
	m_sendq.push (ioa::const_shared_ptr<message_buffer> (m));
      }
      m_send_request = false;
    }

    UP_INTERNAL (mftp_automaton, send_request);

    bool send_announcement_precondition () const {
      return m_send_announcement;
    }

    void send_announcement_effect () {
      assert (!m_file.empty ());

      // If we have outstanding requests, we don't need to announce.
      if (m_requests_count == 0 && m_fragment_count < MAX_FRAGMENT_COUNT) {
	message_buffer* m = get_fragment (m_file.get_random_index ());
	m->convert_to_network ();
	m_sendq.push (ioa::const_shared_ptr<message_buffer> (m));
	if (m_announcement_interval < MAX_ANNOUNCEMENT_INTERVAL) {
	  m_announcement_interval += m_announcement_interval;
	}
	++m_fragment_count;
      }

      m_send_announcement = false;
    }

    UP_INTERNAL (mftp_automaton, send_announcement);

    bool send_match_precondition () const {
      return m_send_match;
    }

    void send_match_effect () {
      assert (!m_matches.empty ());

      if (m_matchq.empty ()) {
	// The match queue is empty.
	// Resize and add all of the matches to it.
	m_matchq.resize (m_matches.size ());
	std::copy (m_matches.begin (), m_matches.end (), m_matchq.begin ());
      }
	  
      uint32_t count = 0;
      fileid matches[MATCHES_SIZE];
      while (count < MATCHES_SIZE && !m_matchq.empty ()) {
	matches[count] = m_matchq.front ();
	m_matchq.pop_front ();
	++count;
      }
	  
      message_buffer* m = new message_buffer (match_type (), m_fileid, count, matches);
      m->convert_to_network ();
      m_sendq.push (ioa::const_shared_ptr<message_buffer> (m));

      m_send_match = false;
    }

    UP_INTERNAL (mftp_automaton, send_match);

    bool set_request_timer_precondition () const {
      return m_request_timer_state == SET_READY && !m_file.complete () && ioa::binding_count (&mftp_automaton::set_request_timer) != 0;
    }

    ioa::time set_request_timer_effect () {
      m_request_timer_state = INTERRUPT_WAIT;
      return REQUEST_INTERVAL;
    }

    V_UP_OUTPUT (mftp_automaton, set_request_timer, ioa::time);

    void request_timer_interrupt_effect () {
      assert (m_request_timer_state == INTERRUPT_WAIT);
      m_send_request = true;
      m_request_timer_state = SET_READY;
    }

    UV_UP_INPUT (mftp_automaton, request_timer_interrupt);

    bool set_announcement_timer_precondition () const {
      return m_announcement_timer_state == SET_READY && !m_file.empty () && ioa::binding_count (&mftp_automaton::set_announcement_timer) != 0;
    }

    ioa::time set_announcement_timer_effect () {
      m_announcement_timer_state = INTERRUPT_WAIT;
      return m_announcement_interval;
    }

    V_UP_OUTPUT (mftp_automaton, set_announcement_timer, ioa::time);
    
    void announcement_timer_interrupt_effect () {
      assert (m_announcement_timer_state == INTERRUPT_WAIT);
      m_send_announcement = true;
      m_announcement_timer_state = SET_READY;
    }

    UV_UP_INPUT (mftp_automaton, announcement_timer_interrupt);

    bool set_match_timer_precondition () const {
      return m_matching_timer_state == SET_READY && !m_matches.empty () && ioa::binding_count (&mftp_automaton::set_match_timer) != 0;
    }

    ioa::time set_match_timer_effect () {
      m_matching_timer_state = INTERRUPT_WAIT;
      return MATCHING_INTERVAL;
    }

    V_UP_OUTPUT (mftp_automaton, set_match_timer, ioa::time);

    void matching_timer_interrupt_effect () {
      assert (m_matching_timer_state == INTERRUPT_WAIT);
      m_send_match = true;
      m_matching_timer_state = SET_READY;
    }

    UV_UP_INPUT (mftp_automaton, matching_timer_interrupt);

    bool set_progress_timer_precondition () const {
      return m_progress_timer_state == SET_READY && ioa::binding_count (&mftp_automaton::set_progress_timer) != 0;
      //std::cout << __func__ << " : " << b << std::endl;
      //return b;
    }

    ioa::time set_progress_timer_effect () {
      m_progress_timer_state = INTERRUPT_WAIT;
      return PROGRESS_INTERVAL;
    }

  public:
    V_UP_OUTPUT (mftp_automaton, set_progress_timer, ioa::time);

  private:
    void progress_timer_interrupt_effect () {
      std::cout << __func__ << std::endl;
      m_progress = true;
      m_progress_timer_state = SET_READY;
    }

  public:
    UV_UP_INPUT (mftp_automaton, progress_timer_interrupt);

  private:
    bool report_progress_precondition () const {
      //std::cout << __func__ << " : " << m_progress << std::endl;
      return m_progress;
    }

    uint32_t report_progress_effect () {
      std::cout << __func__ << std::endl;
      m_progress = false;
      return m_file.get_progress ();
    }

  public:
    V_UP_OUTPUT (mftp_automaton, report_progress, uint32_t);

  private:
    
    bool download_complete_precondition () const {
      return m_file.complete () && !m_reported && ioa::binding_count (&mftp_automaton::download_complete) != 0;
    }

    file download_complete_effect () {
      m_reported = true;
      return m_file;
    }

  public:
    V_UP_OUTPUT (mftp_automaton, download_complete, file);

  private: 
    bool suicide_precondition () const {
      return m_reported && m_suicide_flag;
    }

    void suicide_effect () {
      m_suicide_flag = false;
      self_destruct ();
    }

  public:
    UP_INTERNAL (mftp_automaton, suicide);

  private:
    void match_download_complete_effect (const file& f,
					 ioa::aid_t aid) {
      //For use when the child has reported a download_complete.
      process_match_candidate (f);
      // TODO:  Destroy the mftp_automaton that provided us with the file.
    }

    V_AP_INPUT (mftp_automaton, match_download_complete, file);

    void process_match_candidate (const file& f) {
      // Remove from pending.
      fileid fid = f.get_mfileid ().get_fileid ();
      m_pending_matches.erase (fid);

      if ((*m_match_predicate) (f)) {
	m_matches.insert (fid);
	m_matchq.push_front (fid);
	m_send_match = true;
	m_matching_files.push (ioa::const_shared_ptr<file> (new file (f)));
      }
      else {
	m_non_matches.insert (fid);
      }
    }

    bool match_complete_precondition () const {
      return !m_matching_files.empty () && ioa::binding_count (&mftp_automaton::match_complete) != 0;
    }

    ioa::const_shared_ptr<file> match_complete_effect () {
      ioa::const_shared_ptr<file> f = m_matching_files.front ();
      m_matching_files.pop ();
      return f;
    }

  public:
    V_UP_OUTPUT (mftp_automaton, match_complete, ioa::const_shared_ptr<file>);

  private:
    uint32_t get_random_request_index () {
      assert (m_requests_count != 0);
      uint32_t rf = rand () % m_requests.size();
      for (; !m_requests[rf]; rf = (rf + 1) % m_requests.size ()) { }
      return rf;
    }

    message_buffer* get_fragment (uint32_t idx) {
      uint32_t offset = idx * FRAGMENT_SIZE;
      return new message_buffer (fragment_type (), m_fileid, offset, static_cast<const char*> (m_file.get_data_ptr ()) + offset);
    }


    ~mftp_automaton (){
      std::cout << "destroyed" << this << std::endl;
    }

  };

  const ioa::time mftp_automaton::REQUEST_INTERVAL (1, 0); // 1 second
  const ioa::time mftp_automaton::INIT_ANNOUNCEMENT_INTERVAL (1, 0); //1 second
  const ioa::time mftp_automaton::MAX_ANNOUNCEMENT_INTERVAL (64, 0); //slightly over 1 minute
  const ioa::time mftp_automaton::MATCHING_INTERVAL (4, 0); //4 seconds
  const ioa::time mftp_automaton::PROGRESS_INTERVAL (0,10000); //ms
  const size_t mftp_automaton::MAX_FRAGMENT_COUNT (1); // Number of fragments allowed in sendq.

}

#endif
