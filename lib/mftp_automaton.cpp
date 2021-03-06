#include <mftp/mftp_automaton.hpp>

namespace mftp {
  const ioa::time mftp_automaton::ALARM_INTERVAL (1, 0); // 1 second
  const ioa::time mftp_automaton::INIT_INTERVAL (1, 0); // 1 second
  const ioa::time mftp_automaton::MAX_INTERVAL (64, 0); // slightly over 1 minute
  const uint32_t mftp_automaton::MAX_FRAGMENT_COUNT (1); // Number of fragments allowed in sendq.
  const uint32_t mftp_automaton::SHUFFLE_LIMIT (REQUEST_SIZE);
  const uint32_t mftp_automaton::REREQUEST_NUMERATOR (9);
  const uint32_t mftp_automaton::REREQUEST_DENOMINATOR (10);

  // Not matching.
  mftp_automaton::mftp_automaton (std::auto_ptr<file> file,
				  const ioa::automaton_handle<mftp_channel_automaton>& channel,
				  const bool suicide,
				  const uint32_t progress_threshold) :
    m_self (ioa::get_aid ()),
    m_file (file.get ()),
    m_mfileid (file->get_mfileid ()),
    m_fileid (m_mfileid.get_fileid ()),
    m_channel (channel),
    m_send_state (SEND_READY),
    m_num_frag_in_sendq (0),
    m_num_req_in_sendq (0),
    m_num_match_in_sendq (0),
    m_inserts_since_shuffle (0),
    m_request_idx (0),
    m_last_request_size (0),
    m_fragments_since_request (0),
    m_alarm_state (SET_READY),
    m_announcement_interval (INIT_INTERVAL),
    m_request_interval (INIT_INTERVAL),
    m_match_interval (INIT_INTERVAL),
    m_fragments_since_report (0),
    m_progress_threshold (progress_threshold),
    m_matching (false),
    m_get_matching_files (false),
    m_suicide_flag (suicide),
    m_reported (m_file->complete ())
  {
    m_file_ptr = file.release ();
    create_bindings ();
  }

  mftp_automaton::mftp_automaton (const ioa::const_shared_ptr<file>& file,
				  const ioa::automaton_handle<mftp_channel_automaton>& channel,
				  const bool suicide,
				  const uint32_t progress_threshold) :
    m_self (ioa::get_aid ()),
    m_file (file),
    m_file_ptr (0),
    m_mfileid (file->get_mfileid ()),
    m_fileid (m_mfileid.get_fileid ()),
    m_channel (channel),
    m_send_state (SEND_READY),
    m_num_frag_in_sendq (0),
    m_num_req_in_sendq (0),
    m_num_match_in_sendq (0),
    m_inserts_since_shuffle (0),
    m_request_idx (0),
    m_last_request_size (0),
    m_fragments_since_request (0),
    m_alarm_state (SET_READY),
    m_announcement_interval (INIT_INTERVAL),
    m_request_interval (INIT_INTERVAL),
    m_match_interval (INIT_INTERVAL),
    m_fragments_since_report (0),
    m_progress_threshold (progress_threshold),
    m_matching (false),
    m_get_matching_files (false),
    m_suicide_flag (suicide),
    m_reported (m_file->complete ())
  {
    create_bindings ();
  }

  // Matching.
  mftp_automaton::mftp_automaton (std::auto_ptr<file> file,
				  const ioa::automaton_handle<mftp_channel_automaton>& channel,
				  const match_candidate_predicate& match_candidate_pred,
				  const match_predicate& match_pred,
				  const bool get_matching_files,
				  const bool suicide,
				  const uint32_t progress_threshold) :
    m_self (ioa::get_aid ()),
    m_file (file.get ()),
    m_mfileid (file->get_mfileid ()),
    m_fileid (m_mfileid.get_fileid ()),
    m_channel (channel),
    m_send_state (SEND_READY),
    m_num_frag_in_sendq (0),
    m_num_req_in_sendq (0),
    m_num_match_in_sendq (0),
    m_inserts_since_shuffle (0),
    m_request_idx (0),
    m_last_request_size (0),
    m_fragments_since_request (0),
    m_alarm_state (SET_READY),
    m_announcement_interval (INIT_INTERVAL),
    m_request_interval (INIT_INTERVAL),
    m_match_interval (INIT_INTERVAL),
    m_fragments_since_report (0),
    m_progress_threshold (progress_threshold),
    m_matching (true),
    m_match_candidate_predicate (match_candidate_pred.clone ()),
    m_match_predicate (match_pred.clone ()),
    m_get_matching_files (get_matching_files),
    m_suicide_flag (suicide),
    m_reported (m_file->complete ())
  {
    m_file_ptr = file.release ();
    create_bindings ();
  }

  mftp_automaton::mftp_automaton (const ioa::const_shared_ptr<file>& file,
				  const ioa::automaton_handle<mftp_channel_automaton>& channel,
				  const match_candidate_predicate& match_candidate_pred,
				  const match_predicate& match_pred,
				  const bool get_matching_files,
				  const bool suicide,
				  const uint32_t progress_threshold) :
    m_self (ioa::get_aid ()),
    m_file (file),
    m_file_ptr (0),
    m_mfileid (file->get_mfileid ()),
    m_fileid (m_mfileid.get_fileid ()),
    m_channel (channel),
    m_send_state (SEND_READY),
    m_num_frag_in_sendq (0),
    m_num_req_in_sendq (0),
    m_num_match_in_sendq (0),
    m_inserts_since_shuffle (0),
    m_request_idx (0),
    m_last_request_size (0),
    m_fragments_since_request (0),
    m_alarm_state (SET_READY),
    m_announcement_interval (INIT_INTERVAL),
    m_request_interval (INIT_INTERVAL),
    m_match_interval (INIT_INTERVAL),
    m_fragments_since_report (0),
    m_progress_threshold (progress_threshold),
    m_matching (true),
    m_match_candidate_predicate (match_candidate_pred.clone ()),
    m_match_predicate (match_pred.clone ()),
    m_get_matching_files (get_matching_files),
    m_suicide_flag (suicide),
    m_reported (m_file->complete ())
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


    ioa::automaton_manager<ioa::alarm_automaton>* alarm = new ioa::automaton_manager<ioa::alarm_automaton> (this, ioa::make_generator<ioa::alarm_automaton> ());
    ioa::make_binding_manager (this,
			       &m_self,
			       &mftp_automaton::set_alarm,
			       alarm,
			       &ioa::alarm_automaton::set);
    ioa::make_binding_manager (this,
			       alarm,
			       &ioa::alarm_automaton::alarm,
			       &m_self,
			       &mftp_automaton::alarm_interrupt);

    send_announcement ();
    send_request ();
    schedule ();
  }

  void mftp_automaton::schedule () const {
    if (send_precondition ()) {
      ioa::schedule (&mftp_automaton::send);
    }
    if (set_alarm_precondition ()) {
      ioa::schedule (&mftp_automaton::set_alarm);
    }
    if (send_fragment_precondition ()) {
      ioa::schedule (&mftp_automaton::send_fragment);
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
    // There is room in the sendq for another fragment.
    if (!m_file->empty () && m_requests_set.empty () && m_num_frag_in_sendq < MAX_FRAGMENT_COUNT) {
      const ioa::time now = ioa::time::now ();
      if (m_frag_recv_time + m_announcement_interval <= now) {
	// Send a fragment.
	m_sendq.push (ioa::const_shared_ptr<std::string> (get_fragment (m_file->get_first_fragment_index ())));
	++m_num_frag_in_sendq;
	m_announcement_interval += m_announcement_interval;
	m_announcement_interval = std::min (m_announcement_interval, MAX_INTERVAL);
      }
    }
  }
  
  void mftp_automaton::send_request () {
    // We have requests to send.
    // There are no requests in the sendq.
    // Enough time has elapsed since we sent a request or received a new fragment.
    // We have received some fraction of the fragments that we last requested.
    if (!m_file->complete () &&
	m_num_req_in_sendq == 0) {

      const ioa::time now = ioa::time::now ();
      const bool timeout = m_request_timeout_start + m_request_interval <= now;
      const bool percent = REREQUEST_DENOMINATOR * m_fragments_since_request >= REREQUEST_NUMERATOR * m_last_request_size;

      if (timeout) {
	// Increase the interval.
	m_request_interval += m_request_interval;
	m_request_interval = std::min (m_request_interval, MAX_INTERVAL);
      }

      if (timeout || percent) {
	interval_set<uint32_t>::const_iterator pos = m_file->m_dont_have.lower_bound (std::make_pair (m_request_idx, m_request_idx + 1));
	if (pos == m_file->m_dont_have.end ()) {
	  pos = m_file->m_dont_have.begin ();
	}

	std::set<uint32_t> frags;
	message m (request_type (), m_fileid);

	uint32_t idx = 0;
	while (idx < REQUEST_SIZE) {
	  if (interval_set<uint32_t>::intersect (*pos, std::make_pair (m_request_idx, m_request_idx + 1))) {
	    frags.insert (m_request_idx);
	    m.req.fragments[idx++] = m_request_idx++;
	  }
	  else {
	    ++pos;
	    if (pos == m_file->m_dont_have.end ()) {
	      pos = m_file->m_dont_have.begin ();
	    }
	    m_request_idx = pos->first;
	  }
	}

	m_last_request_size = frags.size ();

	m.convert_to_network ();
	m_sendq.push (ioa::const_shared_ptr<std::string> (new std::string (reinterpret_cast<char *> (&m), sizeof (m))));
	++m_num_req_in_sendq;

	// Reset.
	m_request_timeout_start = now;
	m_fragments_since_request = 0;
	m_last_request_size = 0;
      }
    }
  }

  void mftp_automaton::send_match (bool reset) {
    // Reset if required.
    if (reset) {
      m_match_time = ioa::time ();
      m_match_interval = INIT_INTERVAL;
    }

    // We have matches to send.
    // There are no matches in the sendq.
    if (!m_matches.empty () && m_num_match_in_sendq == 0) {

      // Enough time has elapsed.
      const ioa::time now = ioa::time::now ();
      if (m_match_time + m_match_interval <= now) {

	// Update the interval.
	m_match_interval += m_match_interval;
	m_match_interval = std::min (m_match_interval, MAX_INTERVAL);

	// Create match messages.
	std::set<fileid>::const_iterator pos = m_matches.begin ();
	while (pos != m_matches.end ()) {
	  message m (match_type (), m_fileid);
	  while (m.mat.match_count < MATCHES_SIZE && pos != m_matches.end ()) {
	    m.mat.matches[m.mat.match_count++] = *pos;
	    ++pos;
	  }

	  m.convert_to_network ();
	  m_sendq.push (ioa::const_shared_ptr<std::string> (new std::string (reinterpret_cast<char *> (&m), sizeof (m))));
	  ++m_num_match_in_sendq;
	}
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

  ioa::const_shared_ptr<std::string> mftp_automaton::send_effect () {
    ioa::const_shared_ptr<std::string> m = m_sendq.front ();
    m_sendq.pop ();
    const message* msg = reinterpret_cast<const message*> (m->data ());
    switch (ntohl (msg->header.message_type)) {
    case FRAGMENT:
      --m_num_frag_in_sendq;
      break;
    case REQUEST:
      --m_num_req_in_sendq;
      break;
    case MATCH:
      --m_num_match_in_sendq;
      break;
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
	if (m->frag.idx < m_mfileid.get_fragment_count ()) {

	  // If we are looking for our own file, it must be a fragment from our file and the offset must be correct.
	  if (m->frag.fid == m_fileid) {
	    // Record the time.
	    m_frag_recv_time = ioa::time::now ();

	    // Remove fragment from requests.
	    m_requests_set.erase (m->frag.idx);

	    // Save the fragment.
	    if (!m_file->complete ()) {
	      if (m_file_ptr->write_chunk (m->frag.idx, m->frag.data)) {
		// Just received an new fragment.  Push the time to send a request.
		m_request_timeout_start = ioa::time::now ();
		++m_fragments_since_report;
	      }
	    }

	    // Send a request, possibly.
	    ++m_fragments_since_request;
	    send_request ();
	  }

	  // Otherwise, we could be looking for files that might match our file.
	  // If we are matching, we have not already checked this one AND it is interesting:
	  else if (m_matching &&
		   m_pending_matches.count (m->frag.fid) == 0 &&
		   m_matches.count (m->frag.fid) == 0 &&
		   m_non_matches.count (m->frag.fid) == 0 &&
		   (*m_match_candidate_predicate) (m->frag.fid)) {
	  
	    m_pending_matches.insert (m->frag.fid);

	    std::auto_ptr<file> f (new file (m->frag.fid));
	    f->write_chunk (m->frag.idx, m->frag.data);

	    if (f->complete()) {
	      // We received the whole file.
	      process_match_candidate (ioa::const_shared_ptr<file> (f.release ()));
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
      }
      break;
	
    case REQUEST:
      {
	// Requests must be for our file.
	if (m->req.fid == m_fileid) {

	  // TODO:  Do somethign with the rate.
	  //std::cout << "Rate: " << m->req.fragment_rate << std::endl;

	  // Add the requests to the current set of requests.
	  for (uint32_t idx = 0; idx < REQUEST_SIZE; ++idx) {
	    // If we have the fragment.
	    if (m_file->m_dont_have.find_first_intersect (std::make_pair (m->req.fragments[idx], m->req.fragments[idx] + 1)) == m_file->m_dont_have.end ()) {
	      std::pair<std::set<uint32_t>::iterator, bool> p = m_requests_set.insert (m->req.fragments[idx]);
	    // If the fragment was not already requested.
	    if (p.second) {
	      // Add to the set of requested fragments.
	      m_requests_deque.push_back (m->req.fragments[idx]);
	      ++m_inserts_since_shuffle;
	      if (m_inserts_since_shuffle >= SHUFFLE_LIMIT) {
		std::random_shuffle (m_requests_deque.begin (), m_requests_deque.end ());
		m_inserts_since_shuffle = 0;
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
		  std::auto_ptr<file> f (new file (m->mat.fid));
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
		  std::auto_ptr<file> f (new file (m->mat.matches[idx]));
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

  bool mftp_automaton::set_alarm_precondition () const {
    return m_alarm_state == SET_READY && ioa::binding_count (&mftp_automaton::set_alarm) != 0;
  }

  ioa::time mftp_automaton::set_alarm_effect () {
    m_alarm_state = INTERRUPT_WAIT;
    return ALARM_INTERVAL;
  }

  void mftp_automaton::alarm_interrupt_effect () {
    assert (m_alarm_state == INTERRUPT_WAIT);
    m_alarm_state = SET_READY;

    send_announcement ();
    send_request ();
    send_match (false);
  }

  bool mftp_automaton::send_fragment_precondition () const {
    return !m_requests_deque.empty () && m_num_frag_in_sendq < MAX_FRAGMENT_COUNT;
  }

  void mftp_automaton::send_fragment_effect () {
    uint32_t idx = m_requests_deque.front ();
    m_requests_deque.pop_front ();
    
    std::set<uint32_t>::const_iterator pos = m_requests_set.find (idx);
    if (pos != m_requests_set.end ()) {
      // Remove it from the requests.
      m_requests_set.erase (pos);
      
      // Get the fragment for that index.
      m_sendq.push (ioa::const_shared_ptr<std::string> (get_fragment (idx)));
      ++m_num_frag_in_sendq;
    }
  }

  bool mftp_automaton::download_complete_precondition () const {
    return m_file->complete () && !m_reported && ioa::binding_count (&mftp_automaton::download_complete) != 0;
  }

  ioa::const_shared_ptr<file> mftp_automaton::download_complete_effect () {
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

  void mftp_automaton::match_download_complete_effect (const ioa::const_shared_ptr<file>& f,
						       ioa::aid_t aid) {
    //For use when the child has reported a download_complete.
    process_match_candidate (f);
  }

  void mftp_automaton::process_match_candidate (const ioa::const_shared_ptr<file>& f) {
    // Remove from pending.
    fileid fid = f->get_mfileid ().get_fileid ();
    m_pending_matches.erase (fid);

    if ((*m_match_predicate) (*f)) {
      add_match (fid);
      if (m_get_matching_files) {
	m_matching_files.push (f);
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

  std::string* mftp_automaton::get_fragment (uint32_t idx) {
    message m (fragment_type (), m_fileid, idx, m_file->get_data ().data () + idx * FRAGMENT_SIZE);
    m.convert_to_network ();
    return new std::string (reinterpret_cast<char*> (&m), sizeof (m));
  }

  bool mftp_automaton::fragment_count_precondition () const {
    if (m_progress_threshold != 0) {
      if (!m_file->complete () && m_fragments_since_report >= m_progress_threshold) {
	return true;
      }
      else if (m_file->complete () && m_fragments_since_report != 0) {
	return true;
      }
    }
    return false;
  }

  uint32_t mftp_automaton::fragment_count_effect () {
    m_fragments_since_report = 0;
    return m_file->have_count ();
  }

}
