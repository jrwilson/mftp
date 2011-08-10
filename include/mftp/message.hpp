#ifndef __message_hpp__
#define __message_hpp__

#include <mftp/mfileid.hpp>

namespace mftp {
  const uint32_t FRAGMENT = 0;
  const uint32_t REQUEST = 1;
  const uint32_t MATCH = 2;

  const uint32_t REQUEST_SIZE = 129;
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

  struct request
  {
    fileid fid;
    uint32_t fragments[REQUEST_SIZE];
    
    void convert_to_network () {
      fid.convert_to_network ();
      for (uint32_t i = 0; i < REQUEST_SIZE; ++i) {
	fragments[i] = htonl (fragments[i]);
      }
    }
    
    bool convert_to_host () {
      fid.convert_to_host ();
      
      mfileid mid (fid);
      for (uint32_t i = 0; i < REQUEST_SIZE; ++i) {
	fragments[i] = ntohl (fragments[i]);
	
	if (fragments[i] >= mid.get_fragment_count ()) {
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
	     const fileid& fileid)
    {
      header.message_type = REQUEST;
      req.fid = fileid;
    }

    message (match_type /* */,
	     const fileid& fid)
    {
      header.message_type = MATCH;
      mat.fid = fid;
      mat.match_count = 0;
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

}

#endif
