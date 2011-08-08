#ifndef __jam_hpp__
#define __jam_hpp__

#include <mftp/mftp.hpp>
#include <uuid/uuid.h>

namespace jam {
  
#define FILE_TYPE 0
#define META_TYPE 1
#define QUERY_TYPE 2 
#define INSTANCE_TYPE 3   
  
  const ioa::inet_address SEND_ADDR ("224.0.0.137", 54321);
  const ioa::inet_address LOCAL_ADDR ("0.0.0.0", 54321);
  
  struct meta_predicate :
    public mftp::match_candidate_predicate
  {
    bool operator() (const mftp::fileid& fid) const {
      return fid.type == META_TYPE;
    }

    meta_predicate* clone () const {
      return new meta_predicate (*this);
    }
  };
    
  struct meta_inst_filename_predicate :
    public mftp::match_predicate
  {
    std::string filename;
    
    meta_inst_filename_predicate (const std::string& fname) :
      filename (fname)
    { }

    bool operator() (const mftp::file& f) const {
      assert ((f.get_mfileid ().get_fileid ().type == META_TYPE) || (f.get_mfileid ().get_fileid ().type == INSTANCE_TYPE));
      if (f.get_mfileid ().get_fileid ().type == META_TYPE) {
	if (f.get_mfileid ().get_original_length () >= sizeof (mftp::fileid)) {
	  // We have enough data.
	  // Get the size of the name.
	  const size_t size = f.get_mfileid ().get_original_length () - sizeof (mftp::fileid);
	  return memcmp (filename.c_str (), static_cast<const char*> (f.get_data_ptr ()) + sizeof (mftp::fileid), std::min (filename.size (), size)) == 0;
	}
      }
      else {  //an instance
	if (f.get_mfileid ().get_original_length () >= sizeof (uuid_t)) {
	  //We have enough data.
	  const size_t size = f.get_mfileid ().get_original_length () - sizeof (uuid_t);
	  return memcmp (filename.c_str (), static_cast<const char*> (f.get_data_ptr ()) + sizeof (mftp::fileid), std::min (filename.size (), size)) == 0;
	}

      }
      return false;
    }

    meta_inst_filename_predicate* clone () const {
      return new meta_inst_filename_predicate (*this);
    }

  };
  
  struct query_predicate :
    public mftp::match_candidate_predicate
  {
    bool operator() (const mftp::fileid& fid) const {
      return fid.type == QUERY_TYPE;
    }

    query_predicate* clone () const {
      return new query_predicate (*this);
    }
  };
  
  struct query_filename_predicate :
    public mftp::match_predicate
  {
    std::string filename;

    query_filename_predicate (const std::string& fname) :
      filename (fname)
    { }

    bool operator() (const mftp::file& f) const {
      assert (f.get_mfileid ().get_fileid ().type == QUERY_TYPE);
      return memcmp (filename.c_str (), f.get_data_ptr (), std::min (filename.size (), f.get_mfileid ().get_original_length ())) == 0;
    }

    query_filename_predicate* clone () const {
      return new query_filename_predicate (*this);
    }
  };

}

#endif
