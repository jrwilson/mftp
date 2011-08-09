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
	      const uint32_t type)
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
    m_have_count (0)
  { 
    m_dont_have.insert (std::make_pair (0, m_mfileid.get_fragment_count ()));
  }

  file::file (const file& other) :
    m_dont_have (other.m_dont_have),
    m_mfileid (other.m_mfileid),
    m_have_count (other.m_have_count)
  {
    const uint32_t final_length = m_mfileid.get_final_length ();
    m_data = new unsigned char[final_length];
    memcpy (m_data, other.m_data, final_length);
  }

  file::file (const void* ptr, uint32_t size, uint32_t type)
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

  bool file::write_chunk (const uint32_t idx,
			  const void* data) {
    assert (idx < m_mfileid.get_fragment_count ());

    const interval_set<uint32_t>::interval_type interval = std::make_pair (idx, idx + 1);
    
    if (m_dont_have.find_first_intersect (interval) != m_dont_have.end ()) {
      // Don't have.
      // Copy it.
      memcpy (m_data + idx * FRAGMENT_SIZE, data, FRAGMENT_SIZE);
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
