#include <mftp/mftp_automaton.hpp>

namespace mftp {
  const ioa::time mftp_automaton::TIMER_INTERVAL (1, 0); // 1 second
  const ioa::time mftp_automaton::INIT_INTERVAL (1, 0); // 1 second
  const ioa::time mftp_automaton::MAX_INTERVAL (64, 0); // slightly over 1 minute
  const size_t mftp_automaton::MAX_FRAGMENT_COUNT (1); // Number of fragments allowed in sendq.

  // Not matching.
  mftp_automaton::mftp_automaton (const file& file,
				  const ioa::automaton_handle<mftp_channel_automaton>& channel,
				  const bool suicide,
				  const uint32_t progress_threshold) :
    m_self (ioa::get_aid ()),
    m_file (file),
    m_mfileid (file.get_mfileid ()),
    m_fileid (m_mfileid.get_fileid ()),
    m_last_sent_idx (rand () % m_mfileid.get_fragment_count ()),
    m_send_state (SEND_READY),
    m_fragment_count (0),
    m_timer_state (SET_READY),
    m_announcement_interval (INIT_INTERVAL),
    m_request_interval (INIT_INTERVAL),
    m_match_interval (INIT_INTERVAL),
    m_reported (m_file.complete ()),
    m_last_request_size (0),
    m_fragments_since_request (0),
    m_channel (channel),
    m_matching (false),
    m_get_matching_files (false),
    m_suicide_flag (suicide),
    m_fragments_since_report (0),
    m_progress_threshold (progress_threshold)
  {
    create_bindings ();
  }

  // Matching.
  mftp_automaton::mftp_automaton (const file& file,
				  const ioa::automaton_handle<mftp_channel_automaton>& channel,
				  const match_candidate_predicate& match_candidate_pred,
				  const match_predicate& match_pred,
				  const bool get_matching_files,
				  const bool suicide,
				  const uint32_t progress_threshold) :
    m_self (ioa::get_aid ()),
    m_file (file),
    m_mfileid (file.get_mfileid ()),
    m_fileid (m_mfileid.get_fileid ()),
    m_last_sent_idx (rand () % m_mfileid.get_fragment_count ()),
    m_send_state (SEND_READY),
    m_fragment_count (0),
    m_timer_state (SET_READY),
    m_announcement_interval (INIT_INTERVAL),
    m_request_interval (INIT_INTERVAL),
    m_match_interval (INIT_INTERVAL),
    m_reported (m_file.complete ()),
    m_last_request_size (0),
    m_fragments_since_request (0),
    m_channel (channel),
    m_matching (true),
    m_match_candidate_predicate (match_candidate_pred.clone ()),
    m_match_predicate (match_pred.clone ()),
    m_get_matching_files (get_matching_files),
    m_suicide_flag (suicide),
    m_fragments_since_report (0),
    m_progress_threshold (progress_threshold)
  {
    create_bindings ();
  }

  void mftp_automaton::create_bindings () {
    ioa::make_binding_manager (this,
			       &m_self, &mftp_automaton::send,
			       &m_channel, &mftp_channel_automaton::send);
      
    ioa::make_binding_manager (this,
			       &m_channel, &mftp_channel_automaton::send_complete,
			       &m_self, &mftp_automaton::send_complete);
      
    ioa::make_binding_manager (this,
			       &m_channel, &mftp_channel_automaton::receive,
			       &m_self, &mftp_automaton::receive);


    ioa::automaton_manager<ioa::alarm_automaton>* clock = new ioa::automaton_manager<ioa::alarm_automaton> (this, ioa::make_generator<ioa::alarm_automaton> ());
    ioa::make_binding_manager (this,
			       &m_self,
			       &mftp_automaton::set_timer,
			       clock,
			       &ioa::alarm_automaton::set);
    ioa::make_binding_manager (this,
			       clock,
			       &ioa::alarm_automaton::alarm,
			       &m_self,
			       &mftp_automaton::timer_interrupt);

    send_announcement ();
    send_request (false);
    schedule ();
  }

  void mftp_automaton::schedule () const {
    if (send_precondition ()) {
      ioa::schedule (&mftp_automaton::send);
    }
    if (send_fragment_precondition ()) {
      ioa::schedule (&mftp_automaton::send_fragment);
    }
    if (set_timer_precondition ()) {
      ioa::schedule (&mftp_automaton::set_timer);
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
    if (fragment_count_precondition ()) {
      ioa::schedule (&mftp_automaton::fragment_count);
    }
  }

  void mftp_automaton::send_announcement () {
    // We need at least one fragment.
    // If there are requests, then fragments are forthcoming so don't do anything.
    // There shouldn't be a fragment in the send queue either.
    if (!m_file.empty () && m_requests.empty () && m_fragment_count < MAX_FRAGMENT_COUNT) {
      ioa::time now = ioa::time::now ();
      if (m_fragment_time + m_announcement_interval <= now) {
	// Send a random fragment.
	message_buffer* m = get_fragment (m_file.get_random_index ());
	m->convert_to_network ();
	m_sendq.push (ioa::const_shared_ptr<message_buffer> (m));
	m_announcement_interval += m_announcement_interval;
	m_announcement_interval = std::min (m_announcement_interval, MAX_INTERVAL);
	++m_fragment_count;
      }
    }
  }
  
  void mftp_automaton::send_request (bool reset) {
    if (!m_file.complete ()) {

      ioa::time now = ioa::time::now ();
      bool time_flag = (m_request_time + m_request_interval <= now);
      bool frag_flag = (REQUEST_DENOMINATOR * m_fragments_since_request > REQUEST_NUMERATOR * m_last_request_size);

      if (time_flag && !frag_flag) {
	// Increase the interval.
	m_request_interval += m_request_interval;
	m_request_interval = std::min (m_request_interval, MAX_INTERVAL);
      }

      // If we have received more than 80% of the fragments we requested
      // Or enough time has elapsed
      if (time_flag || frag_flag) {
	span_t spans[SPANS_SIZE];
	spans[0] = m_file.get_next_range ();
	uint32_t sp_count = 1;
	m_last_request_size = (spans[0].stop-spans[0].start) / FRAGMENT_SIZE;
	bool looking = true;
	while (looking) {
	  span_t range;
	  range = m_file.get_next_range ();
	  if (range.start == spans[0].start || sp_count == SPANS_SIZE){ //when we've come back to the range we started on, or we have as many ranges as we can hold
	    looking = false;
	  }
	  else {
	    spans[sp_count] = range;
	    ++sp_count;
	    m_last_request_size += (spans[sp_count].stop - spans[sp_count].start)/FRAGMENT_SIZE;
	  }

	  std::cout << "Requesting ";
	  for (uint32_t i = 0; i < sp_count; ++i) {
	    std::cout << "[" << spans[i].start / FRAGMENT_SIZE << "," << spans[i].stop / FRAGMENT_SIZE << ") ";
	  }
	  std::cout << std::endl;
	}

	message_buffer* m = new message_buffer (request_type (), m_fileid, sp_count, spans);
	m->convert_to_network ();
	m_sendq.push (ioa::const_shared_ptr<message_buffer> (m));
	m_request_time = now;
	m_fragments_since_request = 0;
      }
    }
  }

  void mftp_automaton::send_match (bool reset) {
    if (!m_matches.empty ()) {

      // Reset if required.
      if (reset) {
	m_match_time = ioa::time ();
	m_match_interval = INIT_INTERVAL;
      }

      // Send matches if ready.
      ioa::time now = ioa::time::now ();
      if (m_match_time + m_match_interval <= now) {
	// TODO:  We assume that all matches can be send in m_match_interval.
	std::set<fileid>::const_iterator pos = m_matches.begin ();
	while (pos != m_matches.end ()) {
	  uint32_t count = 0;
	  fileid matches[MATCHES_SIZE];
	  while (count < MATCHES_SIZE && pos != m_matches.end ()) {
	    matches[count] = *pos;
	    ++pos;
	    ++count;
	  }

	  message_buffer* m = new message_buffer (match_type (), m_fileid, count, matches);
	  m->convert_to_network ();
	  m_sendq.push (ioa::const_shared_ptr<message_buffer> (m));
	}

	m_match_interval += m_match_interval;
	m_match_interval = std::min (m_match_interval, MAX_INTERVAL);
      }
    }
  }

  void mftp_automaton::add_match (const fileid& fid) {
    assert (m_matches.count (fid) == 0);
    m_matches.insert (fid);
    // Reset the interval because we have something new.
    send_match (true);
  }

  bool mftp_automaton::send_precondition () const {
    return !m_sendq.empty () && m_send_state == SEND_READY && ioa::binding_count (&mftp_automaton::send) != 0;
  }

  ioa::const_shared_ptr<message_buffer> mftp_automaton::send_effect () {
    ioa::const_shared_ptr<message_buffer> m = m_sendq.front ();
    m_sendq.pop ();
    if (m->msg.header.message_type == htonl (FRAGMENT)) {
      --m_fragment_count;
    }
    
    m_send_state = SEND_COMPLETE_WAIT;
    return m;
  }

  void mftp_automaton::send_complete_effect () {
    m_send_state = SEND_READY;
  }

  void mftp_automaton::receive_effect (const ioa::const_shared_ptr<message>& m) {
    switch (m->header.message_type) {
    case FRAGMENT:
      {
	// If we are looking for our own file, it must be a fragment from our file and the offset must be correct.
	if (m->frag.fid == m_fileid &&
	    m->frag.offset < m_mfileid.get_final_length () &&
	    m->frag.offset % FRAGMENT_SIZE == 0) {
	  // Record the time.
	  m_fragment_time = ioa::time::now ();

	  // Remove fragment from requests.
	  uint32_t idx = (m->frag.offset / FRAGMENT_SIZE);
	  m_requests.erase (std::make_pair (idx, idx + 1));

	  // Save the fragment.
	  if (!m_file.complete ()) {
	    std::cout << "Writing " << m->frag.offset / FRAGMENT_SIZE << std::endl;
	    if (m_file.write_chunk (m->frag.offset, m->frag.data)) {	      
	      ++m_fragments_since_request;
	      // Reset the request interval.
	      send_request (true);
	      ++m_fragments_since_report;
	    }

	    for (interval_set<uint32_t>::const_iterator pos = m_file.m_dont_have.begin ();
		 pos != m_file.m_dont_have.end ();
		 ++pos) {
	      std::cout << "[" << pos->first << "," << pos->second << ") ";
	    }
	    std::cout << std::endl;
	  }
	}

	// Otherwise, we could be looking for files that might match our file.
	// If we are matching, we have not already checked this one AND it is interesting:
	else if (m_matching &&
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
	    ioa::automaton_manager<mftp_automaton>* new_file_home = new ioa::automaton_manager<mftp_automaton> (this, ioa::make_generator<mftp_automaton> (f, m_channel.get_handle(), true, 0));
	      
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
		  ioa::automaton_manager<mftp_automaton>* new_file_home = new ioa::automaton_manager<mftp_automaton> (this, ioa::make_generator<mftp_automaton> (f, m_channel.get_handle(), true, 0));
		    
		  ioa::make_binding_manager (this,
					     new_file_home, &mftp_automaton::download_complete,
					     &m_self, &mftp_automaton::match_download_complete);
		}
		else {
		  // We can just add it to the set.
		  add_match (m->mat.fid);
		}
	      }
	    }
	  }
	  else {
	    // Record the time.
	    m_match_time = ioa::time::now ();

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
		  ioa::automaton_manager<mftp_automaton>* new_file_home = new ioa::automaton_manager<mftp_automaton> (this, ioa::make_generator<mftp_automaton> (f, m_channel.get_handle(), true, 0));
		    
		  ioa::make_binding_manager (this,
					     new_file_home, &mftp_automaton::download_complete,
					     &m_self, &mftp_automaton::match_download_complete);
		}
		else {
		  // We can just add it to the set.
		  add_match (m->mat.matches[idx]);
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

  bool mftp_automaton::send_fragment_precondition () const {
    return !m_requests.empty () && m_fragment_count < MAX_FRAGMENT_COUNT;
  }

  void mftp_automaton::send_fragment_effect () {
    // Advance to the next fragment.
    interval_set<uint32_t>::const_iterator pos = m_requests.lower_bound (std::make_pair (m_last_sent_idx, m_last_sent_idx + 1));
    if (pos == m_requests.end ()) {
      // Start over.
      pos = m_requests.begin ();
    }
    m_last_sent_idx = pos->first;
    // Remove it from the requests.
    m_requests.erase (std::make_pair (m_last_sent_idx, m_last_sent_idx + 1));
    
    // Get the fragment for that index.
    message_buffer* m = get_fragment (m_last_sent_idx);
    m->convert_to_network ();
    m_sendq.push (ioa::const_shared_ptr<message_buffer> (m));
    ++m_fragment_count;
  }  

  bool mftp_automaton::set_timer_precondition () const {
    return m_timer_state == SET_READY && ioa::binding_count (&mftp_automaton::set_timer) != 0;
  }

  ioa::time mftp_automaton::set_timer_effect () {
    m_timer_state = INTERRUPT_WAIT;
    return TIMER_INTERVAL;
  }

  void mftp_automaton::timer_interrupt_effect () {
    assert (m_timer_state == INTERRUPT_WAIT);
    m_timer_state = SET_READY;

    send_announcement ();
    send_request (false);
    send_match (false);
  }

  bool mftp_automaton::download_complete_precondition () const {
    return m_file.complete () && !m_reported && ioa::binding_count (&mftp_automaton::download_complete) != 0;
  }

  file mftp_automaton::download_complete_effect () {
    m_reported = true;
    return m_file;
  }

  bool mftp_automaton::suicide_precondition () const {
    return m_reported && m_suicide_flag;
  }

  void mftp_automaton::suicide_effect () {
    m_suicide_flag = false;
    self_destruct ();
  }

  void mftp_automaton::match_download_complete_effect (const file& f,
						       ioa::aid_t aid) {
    //For use when the child has reported a download_complete.
    process_match_candidate (f);
  }

  void mftp_automaton::process_match_candidate (const file& f) {
    // Remove from pending.
    fileid fid = f.get_mfileid ().get_fileid ();
    m_pending_matches.erase (fid);

    if ((*m_match_predicate) (f)) {
      add_match (fid);
      if (m_get_matching_files) {
	m_matching_files.push (ioa::const_shared_ptr<file> (new file (f)));
      }
    }
    else {
      m_non_matches.insert (fid);
    }
  }

  bool mftp_automaton::match_complete_precondition () const {
    return !m_matching_files.empty () && ioa::binding_count (&mftp_automaton::match_complete) != 0;
  }

  ioa::const_shared_ptr<file> mftp_automaton::match_complete_effect () {
    ioa::const_shared_ptr<file> f = m_matching_files.front ();
    m_matching_files.pop ();
    return f;
  }

  message_buffer* mftp_automaton::get_fragment (uint32_t idx) {
    uint32_t offset = idx * FRAGMENT_SIZE;
    return new message_buffer (fragment_type (), m_fileid, offset, static_cast<const char*> (m_file.get_data_ptr ()) + offset);
  }

  bool mftp_automaton::fragment_count_precondition () const {
    if (m_progress_threshold != 0) {
      if (!m_file.complete () && m_fragments_since_report >= m_progress_threshold) {
	return true;
      }
      else if (m_file.complete () && m_fragments_since_report != 0) {
	return true;
      }
    }
    return false;
  }

  uint32_t mftp_automaton::fragment_count_effect () {
    m_fragments_since_report = 0;
    return m_file.have_count ();
  }

}
