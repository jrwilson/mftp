#ifndef __sha2_256_hpp__
#define __sha2_256_hpp__

/*
  Implementation of the SHA2-256 digest algorithm.
  Based on pseuedo-code found at http://en.wikipedia.org/wiki/SHA-2
 */

#include <algorithm>
#include <arpa/inet.h>
#include <cassert>
#include <cstring>

class sha2_256 {
private:
  static const unsigned int h_init[8];
  static const unsigned int k[64];
  unsigned int h[8];
  unsigned int total_length;
  unsigned int buf_length;
  unsigned char buf[64];

  static unsigned int right_rotate (unsigned int x,
				    unsigned int s);
  void process ();

public:
  sha2_256 ();
  sha2_256 (const unsigned int length,
	    const unsigned char* hash);
  void finalize ();
  void get (unsigned char* hash) const;
  void update (const unsigned char* data,
	       size_t length);
};

#endif
