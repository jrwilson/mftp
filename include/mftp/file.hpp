#ifndef __file_hpp__
#define __file_hpp__

#include <vector>
#include <mftp/fileid.hpp>
#include <mftp/span.hpp>

namespace mftp {
  
  class file {
  private:
    mfileid m_mfileid;
    uint8_t* m_data;
    std::vector<bool> m_have;
    uint32_t m_have_count;
    uint32_t m_start_idx;

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
    bool have (const uint32_t offset) const;
    bool write_chunk (const uint32_t offset,
		      const void* data);
    span_t get_next_range ();
    uint32_t get_random_index () const;
    uint32_t get_progress () const;
  };
}

#endif
