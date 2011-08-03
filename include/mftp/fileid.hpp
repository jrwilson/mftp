#ifndef __fileid_hpp__
#define __fileid_hpp__

#include <arpa/inet.h>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <stdint.h>
#include <stdio.h>
#include <string>

namespace mftp {
  const size_t HASH_SIZE = 32;

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
      // TODO:  Use streams.
      char buffer[2 * sizeof (fileid) + 1];
      char* ptr = buffer;
      const char* end = ptr + sizeof (buffer);

      ptr += snprintf (ptr, end - ptr, "%08x%08x", type, length);
      
      for (size_t idx = 0; idx < HASH_SIZE; ++idx) {
      	ptr += snprintf (ptr, end - ptr, "%02x", hash[idx]);
      }
      return std::string (buffer, ptr - buffer);
    }
  };

}

#endif
