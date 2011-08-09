#ifndef __file_hpp__
#define __file_hpp__

#include <mftp/interval_set.hpp>
#include <mftp/mfileid.hpp>

namespace mftp {
  
  class file {
  public:
    interval_set<uint32_t> m_dont_have;

  private:
    mfileid m_mfileid;
    uint8_t* m_data;
    uint32_t m_have_count;

  public:
    file (const std::string&,
	  const uint32_t);
    file (const fileid& f);
    file (const file& other);
    file (const void* ptr,
	  uint32_t size,
	  uint32_t type);
    ~file ();
    
    const mfileid& get_mfileid () const;
    void* get_data_ptr ();
    const void* get_data_ptr () const;
    uint32_t have_count () const;
    bool complete () const;
    bool empty () const;
    bool write_chunk (const uint32_t idx,
		      const void* data);
    uint32_t get_first_fragment_index () const;
  };
}

#endif
