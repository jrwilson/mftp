#include <mftp/file.hpp>
#include "sha2_256.hpp"

namespace mftp {

  file::file (const fileid& f) :
    m_mfileid (f),
    m_have_count (0)
  {
    m_data.resize (m_mfileid.get_final_length ());
    m_dont_have.insert (std::make_pair (0, m_mfileid.get_fragment_count ()));
  }

  file::file (const file& other) :
    m_dont_have (other.m_dont_have),
    m_mfileid (other.m_mfileid),
    m_data (other.m_data),
    m_have_count (other.m_have_count)
  { }

  file::file (const char* ptr, uint32_t size, uint32_t type)
  {
    m_mfileid.set_length (size);
    m_mfileid.set_type (type);
    m_have_count = m_mfileid.get_fragment_count ();

    m_data.append (ptr, size);
    m_data.resize (m_mfileid.get_final_length ());
  
    // Clear the padding.
    for (uint32_t idx = m_mfileid.get_original_length (); idx < m_mfileid.get_padded_length (); ++idx) {
      m_data[idx] = 0;
    }

    sha2_256 digester;
    digester.update (m_data.data (), m_mfileid.get_final_length ());
    digester.finalize ();
    char samp[HASH_SIZE];
    digester.get (samp);
    m_mfileid.set_hash (samp);
  }

  const mfileid& file::get_mfileid () const {
    return m_mfileid;
  }
  
  const std::string& file::get_data () const {
    return m_data;
  }

  uint32_t file::have_count () const {
    return m_have_count;
  }

  bool file::complete () const {
    return m_have_count == m_mfileid.get_fragment_count ();
  }

  bool file::empty () const {
    return m_have_count == 0;
  }

  bool file::write_chunk (const uint32_t idx,
			  const char* data) {
    assert (idx < m_mfileid.get_fragment_count ());

    const interval_set<uint32_t>::interval_type interval = std::make_pair (idx, idx + 1);
    
    if (m_dont_have.find_first_intersect (interval) != m_dont_have.end ()) {
      // Don't have.
      // Copy it.
      m_data.replace (idx * FRAGMENT_SIZE, FRAGMENT_SIZE, data, FRAGMENT_SIZE);
      // Now we have it.
      m_dont_have.erase (interval);
      ++m_have_count;
      return true;
    }
    else {
      // Have.
      return false;
    }
  }

  uint32_t file::get_first_fragment_index () const {
    const interval_set<uint32_t>::const_iterator pos = m_dont_have.begin ();

    if (pos == m_dont_have.end () ||
	pos->first != 0) {
      // We have the first fragment.
      return 0;
    }
    else {
      return pos->second;
    }
  }

}
