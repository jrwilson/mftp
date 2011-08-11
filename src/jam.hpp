#ifndef __jam_hpp__
#define __jam_hpp__

#include <mftp/mftp.hpp>
#include <uuid/uuid.h>

namespace jam {
  
#define FILE_TYPE 0
#define META_TYPE 1
#define QUERY_TYPE 2 
  
  const ioa::inet_address SEND_ADDR ("224.0.0.137", 54321);
  const ioa::inet_address LOCAL_ADDR ("0.0.0.0", 54321);
  
  struct meta_predicate :
    public mftp::match_candidate_predicate
  {
    std::string filename;

    meta_predicate (const std::string& name) :
      filename (name)
    { }

    bool operator() (const mftp::fileid& fid) const {
      return (fid.type == META_TYPE) && (fid.length == sizeof (mftp::fileid) + filename.size ());
    }

    meta_predicate* clone () const {
      return new meta_predicate (*this);
    }
  };
    
  struct meta_filename_predicate :
    public mftp::match_predicate
  {
    std::string filename;
    
    meta_filename_predicate (const std::string& fname) :
      filename (fname)
    { }

    bool operator() (const mftp::file& f) const {
      return filename == f.get_data ().substr (sizeof (mftp::fileid), filename.size ());
    }

    meta_filename_predicate* clone () const {
      return new meta_filename_predicate (*this);
    }

  };
  
  struct query_predicate :
    public mftp::match_candidate_predicate
  {
    std::string filename;

    query_predicate (const std::string& name) :
      filename (name)
    { }

    bool operator() (const mftp::fileid& fid) const {
      return (fid.type == QUERY_TYPE) && (fid.length == sizeof (uuid_t) + filename.size ());
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
      return filename == f.get_data ().substr (sizeof (uuid_t), filename.size ());
    }

    query_filename_predicate* clone () const {
      return new query_filename_predicate (*this);
    }
  };

}

#endif
