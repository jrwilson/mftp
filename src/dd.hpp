#ifndef __dd_hpp__
#define __dd_hpp__

#include <mftp/mftp.hpp>

#include <string.h>
#include <stdio.h>

namespace dd {
  
#define DESCRIPTION 0
#define SPECIFICATION 1
  
  const ioa::inet_address SEND_ADDR ("224.0.0.137", 54321);
  const ioa::inet_address LOCAL_ADDR ("0.0.0.0", 54321);
  
  struct description_predicate :
    public mftp::match_candidate_predicate
  {
    bool operator() (const mftp::fileid& fid) const {
      return fid.type == DESCRIPTION;
    }

    description_predicate* clone () const {
      return new description_predicate (*this);
    }
  };
    
  struct full_description_predicate :
    public mftp::match_predicate
  {
    std::string spec;
    
    description_filename_predicate (const std::string& sp) :
      spec (sp)
    { }

    bool operator() (const mftp::file& f) const {
      assert (f.get_mfileid ().get_fileid ().type == DESCRIPTION);
      if (f.get_mfileid ().get_original_length () >= strlen (spec)) {
	// We have enough data.
	std::string data;
	sprintf (
	return memcmp (filename.c_str (), static_cast<const char*> (f.get_data_ptr ()) + sizeof (mftp::fileid), std::min (filename.size (), size)) == 0;
      }
      return false;
    }

    description_filename_predicate* clone () const {
      return new description_filename_predicate (*this);
    }

  };
  
  struct specification_predicate :
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
