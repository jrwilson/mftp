#ifndef __message_hpp__
#define __message_hpp__

#include <mftp/mfileid.hpp>
#include <ioa/buffer.hpp>

namespace mftp {
  const uint32_t FRAGMENT = 0;
  const uint32_t REQUEST = 1;
  const uint32_t MATCH = 2;

  const uint32_t SPANS_SIZE = 63;
  const uint32_t MATCHES_SIZE = 12;

  struct fragment
  {
    fileid fid;
    uint32_t idx;
    uint8_t data[FRAGMENT_SIZE];

    void convert_to_network () {
      fid.convert_to_network ();
      idx = htonl (idx);
    }

    bool convert_to_host () {
      fid.convert_to_host ();
      idx = ntohl (idx);

      mfileid mid (fid);
      return idx < mid.get_fragment_count ();
    }
  };

  struct span_t
  {
    uint32_t start;
    uint32_t stop;

    span_t& operator= (const std::pair<uint32_t, uint32_t>& p) {
      start = p.first;
      stop = p.second;
      return *this;
    }

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
    uint32_t frags_per_second;
    uint32_t span_count;
    span_t spans[SPANS_SIZE];
    
    void convert_to_network () {
      fid.convert_to_network ();
      frags_per_second = htonl (frags_per_second);
      for (uint32_t i = 0; i < span_count; ++i) {
	spans[i].convert_to_network ();
      }
      span_count = htonl (span_count);
    }
    
    bool convert_to_host () {
      fid.convert_to_host ();
      frags_per_second = ntohl (frags_per_second);
      span_count = ntohl (span_count);
      
      if (span_count == 0 || span_count > SPANS_SIZE) {
	return false;
      }
      
      mfileid mid (fid);
      for (uint32_t i = 0; i < span_count; ++i) {
	spans[i].convert_to_host ();
	
	if (!((spans[i].start < spans[i].stop) &&
	      (spans[i].stop <= mid.get_fragment_count ()))) {
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
	     uint32_t idx,
	     const void* data)
    {
      header.message_type = FRAGMENT;
      frag.fid = fileid;
      frag.idx = idx;
      memcpy (frag.data, data, FRAGMENT_SIZE);
    }

    message (request_type /* */,
	     const fileid& fileid,
	     uint32_t frags_per_second,
	     uint32_t span_count,
	     const span_t * spans)
    {
      header.message_type = REQUEST;
      req.fid = fileid;
      req.frags_per_second = frags_per_second;
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
		    uint32_t idx,
		    const void* data) :
      msg (type, fileid, idx, data)
    { }

    message_buffer (request_type type,
		    const fileid& fileid,
		    uint32_t frags_per_second,
		    uint32_t span_count,
		    const span_t * spans) :
      msg (type, fileid, frags_per_second, span_count, spans)
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
