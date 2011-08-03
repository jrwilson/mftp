#ifndef __match_hpp__
#define __match_hpp__

#include <mftp/fileid.hpp>
#include <mftp/file.hpp>

namespace mftp {

  struct match_candidate_predicate {
    virtual ~match_candidate_predicate () { }
    virtual bool operator() (const fileid& fid) const = 0;
    virtual match_candidate_predicate* clone () const = 0;
  };
  
  struct match_predicate {
    virtual ~match_predicate () { }
    virtual bool operator() (const file& f) const = 0;
    virtual match_predicate* clone () const = 0;
  };

}

#endif
