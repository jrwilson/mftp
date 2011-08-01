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
    std::string specification;
    
    full_description_predicate (const std::string& spec) :
      specification (spec)
    { }

    bool operator() (const mftp::file& f) const {
      assert (f.get_mfileid ().get_fileid ().type == DESCRIPTION);
      std::string description (static_cast<const char*> (f.get_data_ptr ()), f.get_mfileid ().get_original_length ());
        if (description.find (specification) != std::string::npos) {
	return true;
      }
      return false;
    }

    full_description_predicate* clone () const {
      return new full_description_predicate (*this);
    }

  };
  
  struct specification_predicate :
    public mftp::match_candidate_predicate
  {
    bool operator() (const mftp::fileid& fid) const {
      return fid.type == SPECIFICATION;
    }

    specification_predicate* clone () const {
      return new specification_predicate (*this);
    }
  };
  
  struct full_specification_predicate :
    public mftp::match_predicate
  {
    std::string description;

    full_specification_predicate (const std::string& desc) :
      description (desc)
    { }

    bool operator() (const mftp::file& f) const {
      assert (f.get_mfileid ().get_fileid ().type == SPECIFICATION);
      std::string specification (static_cast<const char*> (f.get_data_ptr ()), f.get_mfileid ().get_original_length ());
        if (description.find (specification) != std::string::npos) {
	return true;
      }
      return false;
    }

    full_specification_predicate* clone () const {
      return new full_specification_predicate (*this);
    }
  };

}

#endif
