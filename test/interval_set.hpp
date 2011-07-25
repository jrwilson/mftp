#ifndef __interval_set_hpp__
#define __interval_set_hpp__

#include <iostream>
#include <set>
#include <cassert>

template <typename Key, 
	  typename Compare = std::less<Key>, 
	  typename Allocator = std::allocator<std::pair<const Key, const Key> > >
class interval_set {
private:
  typedef std::pair<const Key, const Key> interval_type;

  class interval_lt {
  public:
    bool operator() (const interval_type& x, const interval_type& y) const {
      //TODO: use the compare object
      return x.first < y.first;
    }
    
  };
  
  typedef std::set<interval_type, interval_lt, Allocator> set_type;
  set_type m_set;

public:
  typedef typename set_type::value_type value_type;
  typedef typename set_type::key_type key_type;
  typedef typename set_type::key_compare key_compare;
  typedef typename set_type::value_compare value_compare;
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
  typedef typename set_type::allocator_type allocator_type;

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

  key_compare key_comp () const { //TODO: this needs to be changed
    return m_set.key_comp ();
  }

  value_compare value_comp () const { //TODO: this needs to be changed
    return m_set.value_comp ();
  }
 
  explicit interval_set (const key_compare& comp = key_compare (),
			 const Allocator& A = Allocator ()) :
    m_set (comp, A)
  {

  }
  
  template <class InputIterator>
  interval_set (InputIterator f, 
	    InputIterator l, 
	    const key_compare& comp = key_compare (), 
	    const Allocator& A = Allocator ()) {
    assert (false);
  }

  interval_set (const interval_set& is) {
    assert (false);
  }

  interval_set& operator= (const interval_set& is) {
    assert (false);
  }

  allocator_type get_allocator () const {
    assert (false);
  }

  void swap (interval_set& ) {
    assert (false);
  }

private:
  static bool intersect (const interval_type& x, const interval_type& y) {
    return std::max (x.first, y.first) <= std::min (x.second, y.second); //TODO: use compare object
  } 

public:
  std::pair<iterator, bool>
  insert (const value_type& x) {
    /*
      Find the first interval that intersects it
      alpha = min (x, interval we found)
      Remove the interval we found
      Remove all intersecting intervals
      After we're done, we make our new right bound
      beta = max (x, last we saw)
      insert (alpha, beta)
     */
    if (x.first < x.second) { //TODO: use compare object
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

      const Key alpha = std::min (x.first, pos->first); // TODO: use compare object
      Key beta = pos->second;
      while (pos != m_set.end () && intersect (*pos, x)) {
	beta = pos->second;
	m_set.erase (pos++);
      }

      beta = std::max (beta, x.second); //TODO: use compare object

      return m_set.insert (std::make_pair (alpha, beta));
    }
    
    return std::make_pair (m_set.end (), false);
  }

  iterator insert (iterator pos, const value_type& x) {
    assert (false);
  }
  
  template <class InputIterator>
  void insert (InputIterator f, InputIterator l) {
    assert (false);
  }

  void erase (iterator pos) {
    assert (false);
  }

  size_type erase (const key_type& k) {
    assert (false);
  }

  void erase (iterator f, iterator l) {
    assert (false);
  }
  
  iterator find (const key_type& k) const {
    assert (false);
  }

  size_type count (const key_type& k) const {
    assert (false);
  }

  iterator lower_bound (const key_type& k) const {
    assert (false);
  }

  iterator upper_bound (const key_type& k) const {
    assert (false);
  }

  std::pair<iterator, iterator>
  equal_range (const key_type& k) const {
    assert (false);
  }

  
  bool operator== (const interval_set& y) const {
    assert (false);
  }
  
  
  bool operator< (const interval_set& y) const {
    assert (false);
  }
  
};

#endif
