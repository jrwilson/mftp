#ifndef __span_hpp__
#define __span_hpp__

namespace mftp {

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

}

#endif
