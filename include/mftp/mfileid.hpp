#ifndef __mfileid_hpp__
#define __mfileid_hpp__

#include <mftp/fileid.hpp>

namespace mftp {
  const size_t FRAGMENT_SIZE = 512;

  // Memoized fileid.
  class mfileid
  {
  private:
    uint32_t m_fragment_count;
    uint32_t m_padded_length;
    uint32_t m_final_length;
    fileid m_fileid;

    void calculate_lengths () {
      m_padded_length = m_fileid.length;
      // Pad up to a fragment.
      if (m_padded_length % FRAGMENT_SIZE != 0) {
	m_padded_length += (FRAGMENT_SIZE - (m_fileid.length % FRAGMENT_SIZE));
      }
      assert ((m_padded_length % FRAGMENT_SIZE) == 0);
      m_final_length = m_padded_length;
      m_fragment_count = m_final_length / FRAGMENT_SIZE;
    }
    
  public:

    mfileid () :
      m_fragment_count (0),
      m_padded_length (0),
      m_final_length (0)
    { }

    mfileid (const fileid& fileid) :
      m_fileid (fileid)
    {
      calculate_lengths ();
    }

    void set_hash (const char* hash) {
      memcpy (m_fileid.hash, hash, HASH_SIZE);
    }

    void set_type (const uint32_t type) {
      m_fileid.type = type;
    }

    void set_length (const uint32_t length) {
      m_fileid.length = length;
      calculate_lengths ();
    }

    const fileid& get_fileid () const {
      return m_fileid;
    }

    const size_t get_original_length () const {
      return m_fileid.length;
    }

    const size_t get_fragment_count () const {
      return m_fragment_count;
    }

    const size_t get_padded_length () const {
      return m_padded_length;
    }

    const size_t get_final_length () const {
      return m_final_length;
    }

  };

  // // Memoized fileid for validation.
  // class mfileid
  // {
  // private:
  //   uint32_t m_fragment_count;
  //   uint32_t m_padded_length;
  //   uint32_t m_final_length;
  //   fileid m_fileid;

  //   void calculate_lengths () {
  //     // LET'S DO MATH!!

  //     // Calculate the size after we pad with 0s to fall on a HASH_SIZE boundary.
  //     uint32_t padded_length = m_fileid.length;
  //     if (padded_length % HASH_SIZE != 0) {
  // 	padded_length += (HASH_SIZE - (m_fileid.length % HASH_SIZE));
  //     }
  //     assert ((padded_length % HASH_SIZE) == 0);

  //     /*
  // 	Calculate the size after hashing.

  // 	Read pointer is named r.
  // 	Write pointer is named w.
  // 	The fragment index is i.
  // 	The MD5 hash length is M.
  // 	The fragment size is F.
  // 	The padded length is L.

  // 	r = 0 + F * i
  // 	w = L + M * i

  // 	Looking for first i such that r >= w
  // 	F * i >= L + M * i
  // 	F * i - M * i >= L
  // 	i ( F - M) >= L
  // 	i >= L / (F - M)
  // 	i = ceil (L / (F - M) )
  //     */

  //     m_fragment_count = static_cast<uint32_t> (ceil (static_cast<double> (padded_length) / static_cast<double> (FRAGMENT_SIZE - HASH_SIZE)));

  //     uint32_t hashed_length = padded_length + HASH_SIZE * (m_fragment_count - 1);
  //     assert ((hashed_length % HASH_SIZE) == 0);

  //     m_final_length = hashed_length;
  //     if (m_final_length % FRAGMENT_SIZE != 0) {
  // 	m_final_length += (FRAGMENT_SIZE - (hashed_length % FRAGMENT_SIZE));
  //     }
  //     assert ((m_final_length % FRAGMENT_SIZE) == 0);

  //     uint32_t padding = (m_final_length - hashed_length) + (padded_length - m_fileid.length);
  //     assert (padding < FRAGMENT_SIZE);
  //     m_padded_length = m_fileid.length + padding;
  //   }
    
  // public:

  //   mfileid () :
  //     m_fragment_count (0),
  //     m_padded_length (0),
  //     m_final_length (0)
  //   { }

  //   mfileid (const fileid& fileid) :
  //     m_fileid (fileid)
  //   {
  //     calculate_lengths ();
  //   }

  //   void set_hash (const uint8_t* hash) {
  //     memcpy (m_fileid.hash, hash, HASH_SIZE);
  //   }

  //   void set_type (const uint32_t type) {
  //     m_fileid.type = type;
  //   }

  //   void set_length (const uint32_t length) {
  //     m_fileid.length = length;
  //     calculate_lengths ();
  //   }

  //   const fileid& get_fileid () const {
  //     return m_fileid;
  //   }

  //   const size_t get_original_length () const {
  //     return m_fileid.length;
  //   }

  //   const size_t get_fragment_count () const {
  //     return m_fragment_count;
  //   }

  //   const size_t get_padded_length () const {
  //     return m_padded_length;
  //   }

  //   const size_t get_final_length () const {
  //     return m_final_length;
  //   }

  // };

}

#endif
