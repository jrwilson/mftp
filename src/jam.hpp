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
  
  struct meta_inst_predicate :
    public mftp::match_candidate_predicate
  {
    std::string filename;

    meta_inst_predicate (const std::string& name) :
      filename (name)
    { }

    bool operator() (const mftp::fileid& fid) const {
      if (fid.type == META_TYPE) {
	return fid.length == sizeof (mftp::fileid) + filename.size ();
      }
      else if (fid.type == INSTANCE_TYPE) {
	return fid.length == sizeof (uuid_t) + filename.size ();
      }
      else {
	return false;
      }
    }

    meta_inst_predicate* clone () const {
      return new meta_inst_predicate (*this);
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
      if (f.get_mfileid ().get_fileid ().type == META_TYPE) {
	// We have exactly the right amount of data!!!!!!!
	// Get the size of the name.
	return filename == std::string (static_cast<const char*> (f.get_data_ptr ()) + sizeof (mftp::fileid), filename.size ());
      }      
      else {  //an instance
	//We have the right amount of data.
	return filename == std::string (static_cast<const char*> (f.get_data_ptr ()) + sizeof (uuid_t), filename.size ());
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
    std::string filename;

    query_predicate (const std::string& name) :
      filename (name)
    { }

    bool operator() (const mftp::fileid& fid) const {
      if (fid.type == QUERY_TYPE) {
	return fid.length == filename.size ();
      }
      else {
	return false;
      }
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
      return filename == std::string (static_cast<const char*> (f.get_data_ptr ()), filename.size ());
    }

    query_filename_predicate* clone () const {
      return new query_filename_predicate (*this);
    }
  };

}

#endif
