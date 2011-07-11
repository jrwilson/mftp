#ifndef __mftp_hpp__
#define __mftp_hpp__

#include <stdint.h>
#include <cstring>
#include <cassert>
#include <math.h>
#include <utility>
#include <arpa/inet.h>
#include <ioa/buffer.hpp>
#include <iostream>
#include <stdio.h>

namespace mftp {
  const size_t HASH_SIZE = 32;
  const size_t FRAGMENT_SIZE = 512;

  const uint32_t REQUEST = 0;
  const uint32_t FRAGMENT = 1;
  const uint32_t MATCH = 2;

  const uint32_t SPANS_SIZE = 64;
  const uint32_t MATCHES_SIZE = 12;

  struct fileid
  {
    uint32_t type;
    uint32_t length;
    uint8_t hash[HASH_SIZE];

    bool operator== (const fileid& other) const {
      return type == other.type &&
	length == other.length &&
	memcmp (hash, other.hash, HASH_SIZE) == 0;
    }

    bool operator!= (const fileid& other) const {
      return type != other.type &&
	length != other.length &&
	memcmp (hash, other.hash, HASH_SIZE) != 0;
    }

    bool operator< (const fileid& other) const {
      if (type != other.type) {
	return type < other.type;
      }
      if (length != other.length) {
	return length < other.length;
      }
      return memcmp (hash, other.hash, HASH_SIZE) < 0;
    }

    void convert_to_network () {
      type = htonl (type);
      length = htonl (length);
    }

    void convert_to_host () {
      type = ntohl (type);
      length = ntohl (length);
    }

    std::string to_string () const {
      char temp[2 * sizeof (fileid) + 1];
      for (size_t idx = 0; idx < HASH_SIZE; ++idx) {
      	sprintf (&temp[2 * idx], "%02x", hash[idx]);
      }
      sprintf (temp + 2 * HASH_SIZE, "%08x%08x", type, length);
      return std::string (temp, 2 * sizeof (fileid));
    }
  };

  // Memoized fileid.
  class mfileid
  {
  private:
    uint32_t m_fragment_count;
    uint32_t m_padded_length;
    uint32_t m_final_length;
    fileid m_fileid;

    void calculate_lengths () {
      // LET'S DO MATH!!

      // Calculate the size after we pad with 0s to fall on a HASH_SIZE boundary.
      uint32_t padded_length = m_fileid.length + HASH_SIZE - (m_fileid.length % HASH_SIZE);
      assert ((padded_length % HASH_SIZE) == 0);

      /*
  	Calculate the size after hashing.

  	Read pointer is named r.
  	Write pointer is named w.
  	The fragment index is i.
  	The MD5 hash length is M.
  	The fragment size is F.
  	The padded length is L.

  	r = 0 + F * i
  	w = L + M * i

  	Looking for first i such that r >= w
  	F * i >= L + M * i
  	F * i - M * i >= L
  	i ( F - M) >= L
  	i >= L / (F - M)
  	i = ceil (L / (F - M) )
      */

      m_fragment_count = static_cast<uint32_t> (ceil (static_cast<double> (padded_length) / static_cast<double> (FRAGMENT_SIZE - HASH_SIZE)));
      uint32_t hashed_length = padded_length + HASH_SIZE * (m_fragment_count - 1);
      assert ((hashed_length % HASH_SIZE) == 0);

      m_final_length = hashed_length + FRAGMENT_SIZE - (hashed_length % FRAGMENT_SIZE);
      assert ((m_final_length % FRAGMENT_SIZE) == 0);

      uint32_t padding = (m_final_length - hashed_length) + (padded_length - m_fileid.length);
      assert (padding < FRAGMENT_SIZE);
      m_padded_length = m_fileid.length + padding;
    }
    
  public:

    mfileid () :
      m_fragment_count (0),
      m_padded_length (0),
      m_final_length (0)
    { }

    mfileid (const fileid& fileid) :
      m_fileid (fileid)
    {
      calculate_lengths ();
    }

    void set_hash (const uint8_t* hash) {
      memcpy (m_fileid.hash, hash, HASH_SIZE);
    }

    void set_type (const uint32_t type) {
      m_fileid.type = type;
    }

    void set_length (const uint32_t length) {
      m_fileid.length = length;
      calculate_lengths ();
    }

    const fileid& get_fileid () const {
      return m_fileid;
    }

    const size_t get_original_length () const {
      return m_fileid.length;
    }

    const size_t get_fragment_count () const {
      return m_fragment_count;
    }

    const size_t get_padded_length () const {
      return m_padded_length;
    }

    const size_t get_final_length () const {
      return m_final_length;
    }

  };

  struct fragment
  {
    fileid fid;
    uint32_t offset;
    uint8_t data[FRAGMENT_SIZE];

    void convert_to_network () {
      fid.convert_to_network ();
      offset = htonl (offset);
    }

    bool convert_to_host () {
      fid.convert_to_host ();
      offset = ntohl (offset);

      mfileid mid (fid);
      return ((offset % FRAGMENT_SIZE) == 0) && (offset < mid.get_final_length ());
    }
  };
  
  struct span_t
  {
    uint32_t start;
    uint32_t stop;
    
    void convert_to_network () {
      start = htonl (start);
      stop = htonl (stop);
    }
    
    void convert_to_host () {
      start = ntohl (start);
      stop = ntohl (stop);
    }
  };
  
  struct request
  {
    fileid fid;
    uint32_t span_count;
    span_t spans[SPANS_SIZE];
    
    void convert_to_network () {
      fid.convert_to_network ();
      for (uint32_t i = 0; i < span_count; ++i) {
	spans[i].convert_to_network ();
      }
      span_count = htonl (span_count);
    }
    
    bool convert_to_host () {
      fid.convert_to_host ();
      span_count = ntohl (span_count);
      
      if (span_count == 0 || span_count > SPANS_SIZE) {
	return false;
      }
      
      mfileid mid (fid);
      for (uint32_t i = 0; i < span_count; ++i) {
	spans[i].convert_to_host ();
	
	if (!((spans[i].start < spans[i].stop) &&
	      (spans[i].stop <= mid.get_final_length () ) &&
	      (spans[i].start % FRAGMENT_SIZE == 0) &&
	      (spans[i].stop % FRAGMENT_SIZE == 0) )) {
	  return false;
	}
      }
      return true;
    }
  };

  struct match
  {
    fileid fid;
    uint32_t match_count;
    fileid matches[MATCHES_SIZE];

    void convert_to_network () {
      fid.convert_to_network ();
      for (uint32_t i = 0; i < match_count; ++i) {
	matches[i].convert_to_network ();
      }
      match_count = htonl (match_count);
    }

    bool convert_to_host () {
      fid.convert_to_host ();
      match_count = ntohl (match_count);
      
      if (match_count == 0 || match_count > MATCHES_SIZE) {
	return false;
      }
      
      mfileid mid (fid);
      for (uint32_t i = 0; i < match_count; ++i) {
	matches[i].convert_to_host ();
      }
      return true;
    }
    
  };
  
  struct message_header
  {
    uint32_t message_type;
    
    void convert_to_network () {
      message_type = htonl (message_type);
    }
    
    void convert_to_host () {
      message_type = ntohl (message_type);
    }
  };

  struct fragment_type { };
  struct request_type { };
  struct match_type { };

  struct message
  {
    message_header header;
    union {
      fragment frag;
      request req;
      match mat;
    };

    message () { }

    message (fragment_type /* */,
	     const fileid& fileid,
	     uint32_t offset,
	     const void* data)
    {
      header.message_type = FRAGMENT;
      frag.fid = fileid;
      frag.offset = offset;
      memcpy (frag.data, data, FRAGMENT_SIZE);
    }

    message (request_type /* */,
	     const fileid& fileid,
	     uint32_t span_count,
	     const span_t * spans)
    {
      header.message_type = REQUEST;
      req.fid = fileid;
      req.span_count = span_count;
      for (uint32_t i = 0; i<span_count; i++){
	req.spans[i] = spans[i];
      }
    }

    message (match_type /* */,
	     const fileid& fid,
	     uint32_t match_count,
	     const fileid * matches)
    {
      header.message_type = MATCH;
      mat.fid = fid;
      mat.match_count = match_count;
      for (uint32_t i = 0; i < match_count; i++) {
	mat.matches[i] = matches[i];
      }

    }

    void convert_to_network () {
      switch (header.message_type) {
      case FRAGMENT:
        frag.convert_to_network ();
	break;
      case REQUEST:
	req.convert_to_network ();
	break;
      case MATCH:
	mat.convert_to_network ();
	break;
      }
      header.convert_to_network ();
    }

    bool convert_to_host () {
      header.convert_to_host ();
      switch (header.message_type) {
      case FRAGMENT:
        return frag.convert_to_host ();
      case REQUEST:
	return req.convert_to_host ();
      case MATCH:
	return mat.convert_to_host ();
      default:
	return false;
      }
    }
  };

  struct message_buffer :
    public ioa::buffer_interface
  {
    message msg;

    message_buffer (fragment_type type,
		    const fileid& fileid,
		    uint32_t offset,
		    const void* data) :
      msg (type, fileid, offset, data)
    { }

    message_buffer (request_type type,
		    const fileid& fileid,
		    uint32_t span_count,
		    const span_t * spans) :
      msg (type, fileid, span_count, spans)
    { }

    message_buffer (match_type type,
		    const fileid& fid,
		    uint32_t match_count,
		    const fileid * matches) :
      msg (type, fid, match_count, matches)
    { }

    const void* data () const {
      return &msg;
    }

    size_t size () const {
      return sizeof (message);
    }

    void convert_to_network () {
      msg.convert_to_network ();
    }

    void convert_to_host () {
      msg.convert_to_host ();
    }
  };
}

#endif