#ifndef __jam_hpp__
#define __jam_hpp__

#include "mftp.hpp"
#include "file.hpp"

#include <string.h>

namespace jam {

#define FILE_TYPE 0
#define META_TYPE 1
#define QUERY_TYPE 2    

  class interesting_query_predicate:
    public interesting_file_predicate {
  public:
    virtual bool operator() (const mftp::fileid& fid) {
      return fid.type == META_TYPE;
    }
  };
    
  class matching_query_predicate :
    public matching_file_predicate {
  public:
    virtual bool operator() (const mftp::file& f, const char* fname) {
      uint32_t size = f.get_mfileid ().get_original_length () - sizeof (mftp::fileid);
      char other[size];
      memcpy (other, f.get_data_ptr () + sizeof(mftp::fileid), size);
      return strcmp (other, fname) == 0;
    }
  };
  
  class interesting_meta_predicate : 
    public interesting_file_predicate {
  public:
    virtual bool operator() (const mftp::fileid& fid) {
      return fid.type == QUERY_TYPE;
    }
  };
  
  class matching_meta_predicate :
    public matching_file_predicate {
    virtual bool operator() (const mftp::file& f, const char* fname) {
      uint32_t size = f.get_mfileid ().get_original_length ();
      char other[size];
      memcpy (other, f.get_data_ptr (), size);
      return strcmp (other, fname) == 0;
    }
  };

}

#endif
