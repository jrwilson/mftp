#include "interval_tree.hpp"
#include "minunit.h"

#include <list>
#include <algorithm>
#include <iostream>

static const char* default_ctor () {
  std::cout << __func__ << std::endl;
  interval_tree tr;
  mu_assert (tr.size () == 0);
  mu_assert (tr.empty ());
  mu_assert (tr.begin () == tr.end ());
  mu_assert (tr.rbegin () == tr.rend ());
  return 0;
}

static const char* insert_span () {
  std::cout << __func__ << std::endl;
  interval_tree tr;
  tr.insert (std::make_pair (0, 20));
  
  mu_assert (tr.size () == 1);
  mu_assert (!tr.empty ());

  interval_tree::const_iterator it = tr.begin ();
  mu_assert (it != tr.end ());
  mu_assert (tr.rbegin () != tr.rend ());
  mu_assert (*it == std::make_pair (0, 20));
  mu_assert (it++ == tr.begin ());
  mu_assert (it-- == tr.end ());
  mu_assert (++it == tr.end ());
  mu_assert (--it == tr.begin ());

  return 0;
}

static const char* left_gap () {
  std::cout << __func__ << std::endl;
  interval_tree tr;
  tr.insert (std::make_pair (0, 20));
  tr.insert (std::make_pair (-10, -5));

  std::list<std::pair<int, int> > ls;
  ls.push_back (std::make_pair (-10, -5));
  ls.push_back (std::make_pair (0, 20));

  mu_assert (tr.size () == 2);
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));

  return 0;
}

static const char* left_intersect () {
  std::cout << __func__ << std::endl;
 
  interval_tree tr;
  tr.insert (std::make_pair (-7, 20));
  tr.insert (std::make_pair (-10, -7));

  std::list<std::pair<int, int> > ls;
  ls.push_back (std::make_pair (-10, 20));

  mu_assert (tr.size () == 1);
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));
  return 0;
}

static const char* middle_intersect () {
  std::cout << __func__ << std::endl;
  return 0;
}

static const char* right_intersect () {
  std::cout << __func__ << std::endl;
  return 0;
}

static const char* right_gap () {
  std::cout << __func__ << std::endl;
  return 0;
}

const char* all_tests () {
  mu_run_test (default_ctor);
  mu_run_test (insert_span);
  mu_run_test (left_gap);
  mu_run_test (left_intersect);
  mu_run_test (middle_intersect);
  mu_run_test (right_intersect);
  mu_run_test (right_gap);

  return 0;
}

int main (int argc, char **argv)
{
  const char* result = all_tests();
  if (result != 0) {
    std::cout << result << std::endl;
  }

  return result != 0;
}

/*
int main () {
  interval_tree tree1(std::make_pair(0,10));

  if (tree1.find(std::make_pair(0,10)) == tree1.end())
    std::cout << "failed to find (0,10)" << std::endl;
  if (tree1.find(std::make_pair(2,5)) == tree1.end())
    std::cout << "failed to find (2,5)" << std::endl;
  if (tree1.find(std::make_pair(15,20)) != tree1.end())
    std::cout << "thinks it found (15,20)" << std::endl;
  if (tree1.find(std::make_pair(9,10)) == tree1.end())
    std::cout << "failed to find (9,10)" << std::endl; 
  if (tree1.find(std::make_pair(8,15)) != tree1.end())
    std::cout << "thinks it found (8,15)" << std::endl;   


  if (tree1.empty ()) std::cout << "This should not be empty (but is)." << std::endl;
  if (tree1.size () != 10) std::cout << "failed size test" << std::endl;
  
  for (interval_tree::const_iterator pos = tree1.begin (); pos != tree1.end (); ++pos) {
    std::cout << "[" << pos->first << "," << pos->second << ")" << std::endl;
  }

  for (interval_tree::const_iterator pos = tree1.begin (); pos != tree1.end (); pos++) {
    std::cout << "[" << pos->first << "," << pos->second << ")" << std::endl;
  }
  
  for (interval_tree::const_reverse_iterator pos = tree1.rbegin (); pos != tree1.rend (); ++pos) {
    std::cout << "[" << pos->first << "," << pos->second << ")" << std::endl;
  }

  for (interval_tree::const_reverse_iterator pos = tree1.rbegin (); pos != tree1.rend (); pos++) {
    std::cout << "[" << pos->first << "," << pos->second << ")" << std::endl;
  }

  tree1.insert (std::make_pair (3, 12));
  tree1.insert (std::make_pair (0, 5));
  tree1.insert (std::make_pair (14, 18));
   
  //Hi Adam.  I added a method to interval_tree called "test_insert" that prints check.  Probably easier to use for testing :)

  mu_run_test (default_ctor);

  return 0;
}
*/
