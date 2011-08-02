#ifndef __mftp_automaton_hpp__
#define	__mftp_automaton_hpp__

#include <mftp/file.hpp>
#include <mftp/mftp_channel_automaton.hpp>
#include <ioa/alarm_automaton.hpp>
#include <mftp/interval_set.hpp>

#include <cstdlib>
#include <queue>
#include <set>
#include <cstring>

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

    void create_bindings () {
      ioa::make_binding_manager (this,
				 &m_self, &mftp_automaton::send,
				 &m_channel, &mftp_channel_automaton::send);
      
      ioa::make_binding_manager (this,
				 &m_channel, &mftp_channel_automaton::send_complete,
				 &m_self, &mftp_automaton::send_complete);
      
      ioa::make_binding_manager (this,
				 &m_channel, &mftp_channel_automaton::receive,
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
		    const ioa::automaton_handle<mftp_channel_automaton>& channel,
		    const bool suicide) :
      m_self (ioa::get_aid ()),
      m_file (file),
      m_mfileid (file.get_mfileid ()),
      m_fileid (m_mfileid.get_fileid ()),
      m_last_sent_idx (rand () % m_mfileid.get_fragment_count ()),
      m_send_state (SEND_READY),
      m_fragment_count (0),
      m_send_request (!m_file.complete ()),
      m_send_announcement (!m_file.empty ()),
      m_send_match (false),
      m_request_timer_state (SET_READY),
      m_announcement_timer_state (SET_READY),
      m_matching_timer_state (SET_READY),
      m_progress_timer_state (SET_READY),
      m_announcement_interval (INIT_ANNOUNCEMENT_INTERVAL),
      m_reported (m_file.complete ()),
      m_progress (false),
      m_pending_requests (0),
      m_since_received (0),
      m_channel (channel),
      m_matching (false),
      m_get_matching_files (false),
      m_suicide_flag (suicide)
    {
      create_bindings ();
    }

    // Matching.
    mftp_automaton (const file& file,
		    const ioa::automaton_handle<mftp_channel_automaton>& channel,
		    const match_candidate_predicate& match_candidate_pred,
		    const match_predicate& match_pred,
		    const bool get_matching_files,
		    const bool suicide) :
      m_self (ioa::get_aid ()),
      m_file (file),
      m_mfileid (file.get_mfileid ()),
      m_fileid (m_mfileid.get_fileid ()),
      m_last_sent_idx (rand () % m_mfileid.get_fragment_count ()),
      m_send_state (SEND_READY),
      m_fragment_count (0),
      m_send_request (!m_file.complete ()),
      m_send_announcement (!m_file.empty ()),
      m_send_match (false),
      m_request_timer_state (SET_READY),
      m_announcement_timer_state (SET_READY),
      m_matching_timer_state (SET_READY),
      m_progress_timer_state (SET_READY),
      m_announcement_interval (INIT_ANNOUNCEMENT_INTERVAL),
      m_reported (m_file.complete ()),
      m_progress (false),
      m_pending_requests (0),
      m_since_received (0),
      m_channel (channel),
      m_matching (true),
      m_match_candidate_predicate (match_candidate_pred.clone ()),
      m_match_predicate (match_pred.clone ()),
      m_get_matching_files (get_matching_files),
      m_suicide_flag (suicide)
    {
      create_bindings ();
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
	      if (m_file.write_chunk (m->frag.offset, m->frag.data)) {
		++m_since_received;
		//If we have received more than 80% of the fragments we requested...
		if (REQUEST_DENOMINATOR * m_since_received > REQUEST_NUMERATOR * m_pending_requests){
		  m_send_request = true;
		}
	      }
	    }

	    // Remove fragment from requests.
	    uint32_t idx = (m->frag.offset / FRAGMENT_SIZE);
	    m_requests.erase (std::make_pair (idx, idx + 1));
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
	      ioa::automaton_manager<mftp_automaton>* new_file_home = new ioa::automaton_manager<mftp_automaton> (this, ioa::make_generator<mftp_automaton> (f, m_channel.get_handle(), true));
	      
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
	    interval_set<uint32_t> requests;
	    for (uint32_t sp = 0; sp < m->req.span_count; sp++){
	      // Request must be in range.
	      if (m->req.spans[sp].start % FRAGMENT_SIZE == 0 &&
		  m->req.spans[sp].stop % FRAGMENT_SIZE == 0 &&
		  m->req.spans[sp].start < m->req.spans[sp].stop &&
		  m->req.spans[sp].stop <= m_mfileid.get_final_length ()) {
		// We have it.
		requests.insert (std::make_pair (m->req.spans[sp].start / FRAGMENT_SIZE, m->req.spans[sp].stop / FRAGMENT_SIZE));
	      }
	    }
	    if (!requests.empty () && !m_file.m_dont_have.empty ()) {
	      interval_set<uint32_t>::iterator req_pos = requests.begin ();
	      const uint32_t req_limit = (--requests.end ())->second;
	      interval_set<uint32_t>::iterator dh_pos = m_file.m_dont_have.lower_bound (std::make_pair (req_pos->first, req_pos->first + 1));
	      if (dh_pos != m_file.m_dont_have.begin ()) {
		--dh_pos;
	      }
	      for (; dh_pos != m_file.m_dont_have.end () && dh_pos->first < req_limit; ++dh_pos) {
		requests.erase (*dh_pos);
	      }
	    }
	    for (interval_set<uint32_t>::const_iterator pos = requests.begin (); pos != requests.end (); ++pos) {
	      m_requests.insert (*pos);
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
		    ioa::automaton_manager<mftp_automaton>* new_file_home = new ioa::automaton_manager<mftp_automaton> (this, ioa::make_generator<mftp_automaton> (f, m_channel.get_handle(), true));
		    
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
		    ioa::automaton_manager<mftp_automaton>* new_file_home = new ioa::automaton_manager<mftp_automaton> (this, ioa::make_generator<mftp_automaton> (f, m_channel.get_handle(), true));
		    
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
      return !m_requests.empty () && m_fragment_count < MAX_FRAGMENT_COUNT;
    }

    void send_fragment_effect () {
      // Purpose is to produce a randomly selected requested fragment.
      advance_to_next_request_index ();
      m_requests.erase (std::make_pair (m_last_sent_idx, m_last_sent_idx + 1));
      
      // Get the fragment for that index.
      message_buffer* m = get_fragment (m_last_sent_idx);
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
	m_pending_requests = (spans[0].stop-spans[0].start) / FRAGMENT_SIZE;
	bool looking = true;
	while (looking){
	  span_t range = m_file.get_next_range ();
	  if (range.start == spans[0].start || sp_count == SPANS_SIZE){ //when we've come back to the range we started on, or we have as many ranges as we can hold
	    looking = false;
	  }
	  else {
	    spans[sp_count] = range;
	    ++sp_count;
	    m_pending_requests += (spans[sp_count].stop - spans[sp_count].start)/FRAGMENT_SIZE;
	  }
	}

	std::cout << "Sending request" << std::endl;
	message_buffer* m = new message_buffer (request_type (), m_fileid, sp_count, spans);
	m->convert_to_network ();
	m_sendq.push (ioa::const_shared_ptr<message_buffer> (m));

	m_since_received = 0;
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
      if (m_requests.empty () && m_fragment_count < MAX_FRAGMENT_COUNT) {
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
    }

    ioa::time set_progress_timer_effect () {
      m_progress_timer_state = INTERRUPT_WAIT;
      return PROGRESS_INTERVAL;
    }

  public:
    V_UP_OUTPUT (mftp_automaton, set_progress_timer, ioa::time);

  private:
    void progress_timer_interrupt_effect () {
      assert (m_progress_timer_state == INTERRUPT_WAIT);
      m_progress = true;
      m_progress_timer_state = SET_READY;
    }

  public:
    UV_UP_INPUT (mftp_automaton, progress_timer_interrupt);

  private:
    bool report_progress_precondition () const {
      return m_progress;
    }

    uint32_t report_progress_effect () {
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
	if (m_get_matching_files) {
	  m_matching_files.push (ioa::const_shared_ptr<file> (new file (f)));
	}
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
    void advance_to_next_request_index () {
      assert (!m_requests.empty ());
      
      interval_set<uint32_t>::const_iterator pos = m_requests.lower_bound (std::make_pair (m_last_sent_idx, m_last_sent_idx + 1));
      if (pos == m_requests.end ()) {
	// Start over.
	pos = m_requests.begin ();
      }

      m_last_sent_idx = pos->first;
    }

    message_buffer* get_fragment (uint32_t idx) {
      uint32_t offset = idx * FRAGMENT_SIZE;
      return new message_buffer (fragment_type (), m_fileid, offset, static_cast<const char*> (m_file.get_data_ptr ()) + offset);
    }

  };

  const ioa::time mftp_automaton::REQUEST_INTERVAL (1, 0); // 1 second
  const ioa::time mftp_automaton::INIT_ANNOUNCEMENT_INTERVAL (1, 0); //1 second
  const ioa::time mftp_automaton::MAX_ANNOUNCEMENT_INTERVAL (64, 0); //slightly over 1 minute
  const ioa::time mftp_automaton::MATCHING_INTERVAL (4, 0); //4 seconds
  const ioa::time mftp_automaton::PROGRESS_INTERVAL (1,0); //1 second
  const size_t mftp_automaton::MAX_FRAGMENT_COUNT (1); // Number of fragments allowed in sendq.

}

#endif
