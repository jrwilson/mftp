#include <mftp/file.hpp>
#include "sha2_256.hpp"

#include <math.h>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <cassert>
#include <unistd.h>
#include <stdio.h>

#include <iostream>

namespace mftp {

  file::file (const std::string& name,
	      const uint32_t type) :
    m_start_idx (0)
  {
    int fd = open (name.c_str (), O_RDONLY);
    assert (fd != -1);

    struct stat stats;

    if (fstat (fd, &stats) >= 0) {
      m_mfileid.set_length (stats.st_size);
    }
    else {
      perror ("fstat");
      exit (EXIT_FAILURE);
    }

    m_mfileid.set_type (type);

    // We have all of the file.
    m_have_count = m_mfileid.get_fragment_count ();

    /*
      Layout of the file will be
      +-------------------------+-------------+
      |  DATA (original_length) | 0 (padding) |
      +-------------------------+-------------+
    */

    // Allocate storage for the data and read it.
    m_data = new unsigned char[m_mfileid.get_final_length ()];
    if (read (fd, m_data, m_mfileid.get_original_length ()) != static_cast<ssize_t> (m_mfileid.get_original_length ())) {
      perror ("read");
      exit (EXIT_FAILURE);
    }
    close (fd);

    // Clear the padding.
    for (uint32_t idx = m_mfileid.get_original_length (); idx < m_mfileid.get_padded_length (); ++idx) {
      m_data[idx] = 0;
    }

    sha2_256 digester;
    digester.update (m_data, m_mfileid.get_final_length ());
    digester.finalize ();
    unsigned char samp[HASH_SIZE];
    digester.get (samp);
    m_mfileid.set_hash (samp);
  }

  file::file (const fileid& f) :
    m_mfileid (f),
    m_data (new unsigned char[m_mfileid.get_final_length ()]),
    m_have_count (0),
    m_start_idx (0)
  { 
    m_dont_have.insert (std::make_pair (0, m_mfileid.get_fragment_count ()));
  }

  file::file (const file& other) :
    m_dont_have (other.m_dont_have),
    m_mfileid (other.m_mfileid),
    m_have_count (other.m_have_count),
    m_start_idx (0)
  {
    const uint32_t final_length = m_mfileid.get_final_length ();
    m_data = new unsigned char[final_length];
    memcpy (m_data, other.m_data, final_length);
  }

  file::file (const void* ptr, uint32_t size, uint32_t type) :
    m_start_idx (0)
  {
    m_mfileid.set_length (size);
    m_mfileid.set_type (type);
    m_have_count = m_mfileid.get_fragment_count ();
    
    m_data = new unsigned char[m_mfileid.get_final_length ()];
    memcpy (m_data, ptr, size);
  
    // Clear the padding.
    for (uint32_t idx = m_mfileid.get_original_length (); idx < m_mfileid.get_padded_length (); ++idx) {
      m_data[idx] = 0;
    }

    sha2_256 digester;
    digester.update (m_data, m_mfileid.get_final_length ());
    digester.finalize ();
    unsigned char samp[HASH_SIZE];
    digester.get (samp);
    m_mfileid.set_hash (samp);
  }

  file::~file () {
    delete [] m_data;
  }

  const mfileid& file::get_mfileid () const {
    return m_mfileid;
  }
  
  void* file::get_data_ptr () {
    return m_data;
  }

  const void* file::get_data_ptr () const {
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

  // bool file::have (const uint32_t offset) const {
  //   assert (offset % FRAGMENT_SIZE == 0);
  //   assert (offset < m_mfileid.get_final_length ());
  //   const uint32_t idx = offset / FRAGMENT_SIZE;
  //   interval_set<uint32_t>::const_iterator pos = m_dont_have.lower_bound (std::make_pair (idx, idx + 1));
  //   if (pos != m_dont_have.end () && pos->first <= offset) {
  //     return false;
  //   }
  //   else {
  //     return true;
  //   }
  // }

  bool file::write_chunk (const uint32_t offset,
			  const void* data) {
    assert (offset % FRAGMENT_SIZE == 0);
    assert (offset < m_mfileid.get_final_length ());

    const uint32_t idx = offset / FRAGMENT_SIZE;
    interval_set<uint32_t>::const_iterator pos = m_dont_have.lower_bound (std::make_pair (idx, idx + 1));
    if (pos != m_dont_have.end () && pos->first <= idx) {
      // Don't have.
      // Copy it.
      memcpy (m_data + offset, data, FRAGMENT_SIZE);
      // Now we have it.
      m_dont_have.erase (std::make_pair (idx, idx + 1));
      ++m_have_count;
      return true;
    }
    else {
      // Have.
      return false;
    }
  }

  span_t file::get_next_range () {
    // We shouldn't have all the fragments.
    assert (m_have_count != m_mfileid.get_fragment_count ());

    interval_set<uint32_t>::iterator pos = m_dont_have.lower_bound (std::make_pair (m_start_idx, m_start_idx + 1));

    if (pos == m_dont_have.end ()) {
      pos = m_dont_have.begin ();
    }
      
    // Request range [m_start_idx, end_idx).
    span_t retval;
    retval.start = pos->first * FRAGMENT_SIZE;
    retval.stop = pos->second * FRAGMENT_SIZE;
    m_start_idx = pos->second;

    return retval;
  }

  uint32_t file::get_random_index () const {
    assert (m_have_count != 0);

    uint32_t rf = 0;
    for (interval_set<uint32_t>::const_iterator pos = m_dont_have.begin ();
	 pos != m_dont_have.end () && pos->first == rf;
	 ++pos, rf = pos->second)
      ;;

    return rf;  
  }

  uint32_t file::get_progress () const {
    return m_have_count;
  }
}
