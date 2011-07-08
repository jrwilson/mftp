#ifndef __mftp_automaton_hpp__
#define	__mftp_automaton_hpp__

#include "file.hpp"
#include "conversion_channel_automaton.hpp"
#include <ioa/alarm_automaton.hpp>
#include <ioa/udp_sender_automaton.hpp>
#include <ioa/udp_receiver_automaton.hpp>

#include <arpa/inet.h>
#include <config.hpp>
#include <cstdlib>
#include <queue>
#include <set>
#include <vector>
#include <cstring>
#include <math.h>


struct interesting_file_predicate {
  virtual bool operator() (const mftp::fileid& fid) = 0;
};

struct matching_file_predicate {
  virtual bool operator() (const mftp::file& f, const char* fname) = 0;
};

namespace mftp {
  class mftp_automaton :
    public ioa::automaton
  {
  private:
    const ioa::time m_fragment_interval;
    const ioa::time m_request_interval;
    const ioa::time m_announcement_interval;
    const ioa::time m_matching_interval;

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
    std::vector<bool> m_their_req;
    uint32_t m_their_req_count;
    std::queue<ioa::const_shared_ptr<message_buffer> > m_sendq;
    send_state_t m_send_state;
    timer_state_t m_fragment_timer_state;
    timer_state_t m_request_timer_state;
    timer_state_t m_announcement_timer_state;
    timer_state_t m_matching_timer_state;
    bool m_reported;
    bool m_matching;
    bool m_returning_files;

    std::set<fileid> matches;
    std::set<fileid> non_matches;

    std::queue<ioa::const_shared_ptr<file> > matching_files;

    ioa::handle_manager<ioa::udp_sender_automaton> m_sender;
    ioa::handle_manager<conversion_channel_automaton> m_converter;

  public:
    mftp_automaton (const file& file, bool matching, bool returning, const ioa::automaton_handle<ioa::udp_sender_automaton>& sender, const ioa::automaton_handle<conversion_channel_automaton>& converter) :
      m_fragment_interval (0, 1000), // 1000 microseconds = 1 millisecond
      m_request_interval (1, 0), // 1 second
      m_announcement_interval (7, 0), // 7 seconds
      m_matching_interval (5, 0), //5 seconds
      m_self (ioa::get_aid ()),
      m_file (file),
      m_mfileid (file.get_mfileid ()),
      m_fileid (m_mfileid.get_fileid ()),
      m_their_req (m_mfileid.get_fragment_count ()),
      m_their_req_count (0),
      m_send_state (SEND_READY),
      m_fragment_timer_state (SET_READY),
      m_request_timer_state (SET_READY),
      m_announcement_timer_state (SET_READY),
      m_matching_timer_state (SET_READY),
      m_reported (m_file.complete ()),
      m_matching (matching),
      m_returning_files (returning),
      m_sender (sender),
      m_converter (converter)
    {
      ioa::make_binding_manager (this,
				 &m_self, &mftp_automaton::send,
				 &m_sender, &ioa::udp_sender_automaton::send);
      
      ioa::make_binding_manager (this,
				 &m_sender, &ioa::udp_sender_automaton::send_complete,
				 &m_self, &mftp_automaton::send_complete);
      
      ioa::make_binding_manager (this,
				 &m_converter, &conversion_channel_automaton::pass_message,
				 &m_self, &mftp_automaton::receive);

      ioa::automaton_manager<ioa::alarm_automaton>* fragment_clock = new ioa::automaton_manager<ioa::alarm_automaton> (this, ioa::make_generator<ioa::alarm_automaton> ());
      ioa::make_binding_manager (this,
			     &m_self,
			     &mftp_automaton::set_fragment_timer,
			     fragment_clock,
			     &ioa::alarm_automaton::set);
      ioa::make_binding_manager (this,
			     fragment_clock,
			     &ioa::alarm_automaton::alarm,
			     &m_self,
			     &mftp_automaton::fragment_timer_interrupt);

    
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
      
      schedule ();
    }

  private:
    void schedule () const {
      if (send_precondition ()) {
	ioa::schedule (&mftp_automaton::send);
      }

      if (set_fragment_timer_precondition ()) {
	ioa::schedule (&mftp_automaton::set_fragment_timer);
      }

      if (set_request_timer_precondition ()) {
	ioa::schedule (&mftp_automaton::set_request_timer);
      }

      if (set_announcement_timer_precondition ()) {
	ioa::schedule (&mftp_automaton::set_announcement_timer);
      }

      if (m_matching){
	if (match_complete_precondition ()){
	  ioa::schedule (&mftp_automaton::match_complete);
	}
	if (set_match_timer_precondition ()) {
	  ioa::schedule (&mftp_automaton::set_match_timer);
	}
      }
      
      if (download_complete_precondition ()) {
	ioa::schedule (&mftp_automaton::download_complete);
      }
    }

    bool send_precondition () const {
      return !m_sendq.empty () && m_send_state == SEND_READY;
    }

    ioa::udp_sender_automaton::send_arg send_effect () {
      std::cout << __func__ << ": sendq size is " << m_sendq.size () << std::endl;
      ioa::inet_address a ("224.0.0.137", 54321);
      ioa::const_shared_ptr<message_buffer> m = m_sendq.front ();
      m_sendq.pop ();
      m_send_state = SEND_COMPLETE_WAIT;
      return ioa::udp_sender_automaton::send_arg (a, m);
    }
    
  public:
    V_UP_OUTPUT (mftp_automaton, send, ioa::udp_sender_automaton::send_arg);

  private:
    void send_complete_effect (const int& result) {
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
	m_send_state = SEND_READY;
      }
    }

  public:
    V_UP_INPUT (mftp_automaton, send_complete, int);

  private:
    void receive_effect (const ioa::const_shared_ptr<message>& m) {
      switch (m->header.message_type) {
      case FRAGMENT:
	{
	  // If we are looking for our own file, it must be a fragment from our file and the offset must be correct.
	  if (!m_file.complete () &&
	      m->frag.fid == m_fileid &&
	      m->frag.offset < m_mfileid.get_final_length () &&
	      m->frag.offset % FRAGMENT_SIZE == 0) {
	    
	    // Save the fragment.
	    m_file.write_chunk (m->frag.offset, m->frag.data);
	    
	    uint32_t idx = (m->frag.offset / FRAGMENT_SIZE);
	    if (m_their_req[idx]) {
	      // Somebody else wanted it and now has just seen it.
	      m_their_req[idx] = false;
	      --m_their_req_count;
	    }
	  }

	  //Otherwise, we could be looking for files that might match our file.

	  //if ((m_fileid.type == META_TYPE && m.fid.type == QUERY_TYPE) && (m_fileid.type == QUERY_TYPE && m.fid.type == META_TYPE)) {       FIXME:  this if statement should be part of the interesting functor	    
	    //If we are matching, we have not already checked this one AND it is interesting:
	  if (m_matching &&
	      matches.count (m->frag.fid) == 0 &&
	      non_matches.count (m->frag.fid) == 0 &&
	      
	      /*it is interesting*/ true) {

	    //We have probably already received the whole file.
	    file f (m->frag.fid);
	    f.write_chunk (m->frag.offset, m->frag.data);
	    if (f.complete()) {
	      
	      process_matching_file (f);
	    }
	    //Create mftp_automaton with MATCHING FALSE to download other file, when download completes, perform matching
	    else {
	      ioa::automaton_manager<mftp::mftp_automaton>* new_file_home = new ioa::automaton_manager<mftp::mftp_automaton> (this, ioa::make_generator<mftp::mftp_automaton> (mftp::file (m->frag.fid), false, false, m_sender.get_handle(), m_converter.get_handle()));
	      
	      ioa::make_binding_manager (this,
					 new_file_home, &mftp_automaton::download_complete,
					 &m_self, &mftp_automaton::match_download_complete);
	    }
	  } 
	}
	break;
	
      case REQUEST:
	{
	  std::cout << "Received a request." << std::endl;
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
		  if (m_file.have (offset) && !m_their_req[idx]) {
		    m_their_req[idx] = true;
		    ++m_their_req_count;
		  }
		}
	      }
	    }
	  }
	}
	break;

      case MATCH:
	{
	  //TODO:  learning matches from others.


	}
	break;

      default:
	// Unkown message type.
	break;
      }
      
    }
    
  public:

    V_UP_INPUT (mftp_automaton, receive, ioa::const_shared_ptr<message>);

  private:
    bool set_fragment_timer_precondition () const {
      return m_fragment_timer_state == SET_READY && m_their_req_count != 0 && ioa::binding_count (&mftp_automaton::set_fragment_timer) != 0;
    }

    ioa::time set_fragment_timer_effect () {
      m_fragment_timer_state = INTERRUPT_WAIT;
      return m_fragment_interval;
    }

    V_UP_OUTPUT (mftp_automaton, set_fragment_timer, ioa::time);

    void fragment_timer_interrupt_effect () {
      std::cout << __func__ << std::endl;
      // Purpose is to produce a randomly selected requested fragment.
      std::cout << m_their_req_count << std::endl;
      if (m_their_req_count != 0 && m_sendq.empty()) {
	std::cout << "decided to send fragment" << std::endl;
	// Get a random index.
	uint32_t randy = get_random_request_index ();
	m_their_req[randy] = false;
	--m_their_req_count;

	// Get the fragment for that index.
	message_buffer* m = get_fragment (randy);
	m->convert_to_network ();
	m_sendq.push (ioa::const_shared_ptr<message_buffer> (m));
      }

      m_fragment_timer_state = SET_READY;
    }

    UV_UP_INPUT (mftp_automaton, fragment_timer_interrupt);

    bool set_request_timer_precondition () const {
      return m_request_timer_state == SET_READY && !m_file.complete () && ioa::binding_count (&mftp_automaton::set_request_timer) != 0;
    }

    ioa::time set_request_timer_effect () {
      m_request_timer_state = INTERRUPT_WAIT;
      return m_request_interval;
    }

    V_UP_OUTPUT (mftp_automaton, set_request_timer, ioa::time);

    void request_timer_interrupt_effect () {
      std::cout << __func__ << std::endl;
      if (m_sendq.empty () && !m_file.complete ()) {
	span_t spans[64];
	spans[0] = m_file.get_next_range();
	uint32_t sp_count = 1;
	bool looking = true;
	while (looking){
	  span_t range = m_file.get_next_range ();
	  if (range.start == spans[0].start || sp_count == 64){ //when we've come back to the range we started on, or we have as many ranges as we can hold
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
      m_request_timer_state = SET_READY;
    }

    UV_UP_INPUT (mftp_automaton, request_timer_interrupt);

    bool set_announcement_timer_precondition () const {
      return m_announcement_timer_state == SET_READY && ioa::binding_count (&mftp_automaton::set_announcement_timer) != 0;
    }

    ioa::time set_announcement_timer_effect () {
      m_announcement_timer_state = INTERRUPT_WAIT;
      return m_announcement_interval;
    }

    V_UP_OUTPUT (mftp_automaton, set_announcement_timer, ioa::time);
    
    void announcement_timer_interrupt_effect () {
      std::cout << __func__ << std::endl;
      if (!m_file.empty () && m_sendq.empty()) {
	message_buffer* m = get_fragment (m_file.get_random_index ());
	m->convert_to_network ();
	m_sendq.push (ioa::const_shared_ptr<message_buffer> (m));
      }
      m_announcement_timer_state = SET_READY;
    }

    UV_UP_INPUT (mftp_automaton, announcement_timer_interrupt);

    bool set_match_timer_precondition () const {
      return m_matching_timer_state == SET_READY && ioa::binding_count (&mftp_automaton::set_match_timer) != 0;
    }

    ioa::time set_match_timer_effect () {
      m_matching_timer_state = INTERRUPT_WAIT;
      return m_matching_interval;
    }

    V_UP_OUTPUT (mftp_automaton, set_match_timer, ioa::time);


    void matching_timer_interrupt_effect () {
      std::cout << __func__ << std::endl;
      if (!matches.empty () && m_sendq.empty()){
	size_t count = matches.size();
	fileid mats[12];
	if (count > 12){
	  count = 12;
	  srand ((unsigned) time (0));
	  uint32_t idx = rand () % matches.size();
	  
	  std::set<fileid>::iterator it = matches.begin ();
	  for (uint32_t i = 0; i < idx; i++) { ++it; }

	  for (uint32_t j = 0; j < count; ++j) {
	    if (it == matches.end ()) {
	      mats[j] = *it;
	      it = matches.begin ();
	    } 
	    else {
	      mats[j] = *it;
	      ++it;
	    }
	  }
	}
	else {
	  uint32_t i = 0;
	  for (std::set<fileid>::iterator it = matches.begin ();
	       it != matches.end (); 
	       ++it) {
	    mats[i] = *it;
	    ++i;
	  }
	}
	message_buffer* m = new message_buffer (match_type (), m_fileid, static_cast<uint32_t>(count), mats);

	m->convert_to_network ();
	m_sendq.push (ioa::const_shared_ptr<message_buffer> (m));
      }
      m_matching_timer_state = SET_READY;
    }

    UV_UP_INPUT (mftp_automaton, matching_timer_interrupt);
    

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
    void match_download_complete_effect (const file& f) {      //For use when the child has reported a download_complete.
      process_matching_file (f);
    }

    V_UP_INPUT (mftp_automaton, match_download_complete, file);


    void process_matching_file (const file& f) {
      if (/*MATCHING FUNCTOR APPLIED TO f*/ true){
	matches.insert(f.get_mfileid ().get_fileid ());
	matching_files.push (ioa::const_shared_ptr<file> (new file (f)));
      }
      else{
	non_matches.insert(f.get_mfileid ().get_fileid ());
      }
    }


    bool match_complete_precondition () const {
      return !matching_files.empty () && ioa::binding_count (&mftp_automaton::match_complete) != 0;
    }

    ioa::const_shared_ptr<file> match_complete_effect () {    //For use when a match has been found and the Client needs to know.
      std::cout << __func__ << std::endl;
      ioa::const_shared_ptr<file> f = matching_files.front ();
      matching_files.pop ();
      return f;
    }

  public:
    V_UP_OUTPUT (mftp_automaton, match_complete, ioa::const_shared_ptr<file>);

  private:
    uint32_t get_random_request_index () {
      assert (m_their_req_count != 0);
      uint32_t rf = rand () % m_their_req.size();
      for (; !m_their_req[rf]; rf = (rf + 1) % m_their_req.size ()) { }
      return rf;
    }

    message_buffer* get_fragment (uint32_t idx) {
      uint32_t offset = idx * FRAGMENT_SIZE;
      return new message_buffer (fragment_type (), m_fileid, offset, m_file.get_data_ptr() + offset);
    }

  };

}

#endif
