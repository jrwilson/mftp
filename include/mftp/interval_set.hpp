#ifndef __interval_set_hpp__
#define __interval_set_hpp__

#include <iostream>
#include <set>
#include <cassert>

template <typename Key> 
class interval_set {
private:
  typedef std::pair<const Key, const Key> interval_type;

  class interval_lt {
  public:
    bool operator() (const interval_type& x, const interval_type& y) const {
      return x.first < y.first;
    }
    
  };
  
  typedef std::set<interval_type, interval_lt> set_type;
  set_type m_set;

public:
  typedef typename set_type::value_type value_type;
  typedef typename set_type::key_type key_type;
  typedef typename set_type::pointer pointer;
  typedef typename set_type::const_pointer const_pointer;
  typedef typename set_type::reference reference;
  typedef typename set_type::const_reference const_reference;
  typedef typename set_type::size_type size_type;
  typedef typename set_type::difference_type difference_type;
  typedef typename set_type::iterator iterator;
  typedef typename set_type::const_iterator const_iterator;
  typedef typename set_type::reverse_iterator reverse_iterator;
  typedef typename set_type::const_reverse_iterator const_reverse_iterator;

  iterator begin () const {
    return m_set.begin ();
  }

  iterator end () const {
    return m_set.end ();
  }

  reverse_iterator rbegin () const {
    return m_set.rbegin ();
  }

  reverse_iterator rend () const {
    return m_set.rend ();
  }

  size_type size () const {
    return m_set.size ();
  }

  size_type max_size () const {
    return m_set.max_size ();
  }

  bool empty () const {
    return m_set.empty ();
  }

  explicit interval_set () { }
  
  template <class InputIterator>
  interval_set (InputIterator f, 
		InputIterator l) {
    for (; f != l; ++f) {
      insert (*f);
    }
  }

  interval_set (const interval_set& is) :
    m_set (is.m_set)
  { }

  interval_set& operator= (const interval_set& is) {
    if (this != &is) {
      m_set = is.m_set;
    }
    return *this;
  }

  void swap (interval_set& is) {
    if (this != &is) {
      m_set.swap (is.m_set);
    }
  }

private:
  static bool intersect (const interval_type& x, const interval_type& y) {
    return std::max (x.first, y.first) <= std::min (x.second, y.second);
  } 

public:
  std::pair<iterator, bool>
  insert (const value_type& x) {

    if (x.first < x.second) {       
      if (empty ()) {
	return m_set.insert (x);
      }

      iterator pos = m_set.lower_bound (x);
      if (pos == m_set.end ()) {
	--pos;
      }
      if (!intersect (*pos, x)) {
	//nothing intersects x
	return m_set.insert (x);
      }
      if (pos != m_set.begin ()) {
	iterator it = pos;
	--it;

	if (intersect (*it, x)) {
	  pos = it;
	}
      }

      const Key alpha = std::min (x.first, pos->first); 
      Key beta = pos->second;
      while (pos != m_set.end () && intersect (*pos, x)) {
	beta = pos->second;
	m_set.erase (pos++);
      }

      beta = std::max (beta, x.second);
      return m_set.insert (std::make_pair (alpha, beta));
    }
    
    return std::make_pair (m_set.end (), false);
  }

  template <class InputIterator>
  void insert (InputIterator f, InputIterator l) {
    for (; f != l; ++f) {
      insert (*f);
    }
  }

  void erase (iterator pos) {
    m_set.erase (pos);
  }

  size_type erase (const key_type& k) {
    if (k.first < k.second) {       
      if (empty ()) {
	return 0;
      }
      iterator pos = m_set.lower_bound (k);
      if (pos != m_set.begin ()) {
	iterator it = pos;
	--it;
	
	if (intersect (*it, k)) {
	  pos = it;
	}
      }

      if (pos == m_set.end () || !intersect (*pos, k)) {
	//nothing intersects k
	return 0;
      }

      const Key alpha = pos->first; 
      const Key beta = std::min (pos->second, k.first);
      Key delta = pos->second;

      size_type count = 0;
      while (pos != m_set.end () && intersect (*pos, k)) {
	delta = pos->second;
	m_set.erase (pos++);
	++count;
      }
      insert (std::make_pair (alpha, beta));
      insert (std::make_pair (k.second, delta));

      return count;
    }
    return 0;
  }

  void erase (iterator f, iterator l) {
    m_set.erase (f, l);
  }
  
  iterator find (const key_type& k) const {
    return m_set.find (k);
  }

  size_type count (const key_type& k) const {
    return m_set.count (k);
  }

  iterator lower_bound (const key_type& k) const {
    return m_set.lower_bound (k);
  }

  iterator upper_bound (const key_type& k) const {
    return m_set.upper_bound (k);
  }

  std::pair<iterator, iterator>
  equal_range (const key_type& k) const {
    return m_set.equal_range (k);
  }

  
  bool operator== (const interval_set& y) const {
    return m_set == y.m_set;
  }
  
  
  bool operator< (const interval_set& y) const {
    return m_set < y.m_set;
  }
  
};

#endif
