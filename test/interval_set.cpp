#include "interval_set.hpp"
#include "minunit.h"

#include <list>
#include <algorithm>
#include <iostream>

static const char* default_ctor () {
  std::cout << __func__ << std::endl;
  interval_set<int> tr;
  mu_assert (tr.size () == 0);
  mu_assert (tr.empty ());
  mu_assert (tr.begin () == tr.end ());
  mu_assert (tr.rbegin () == tr.rend ());
  return 0;
}

static const char* insert_span () {
  std::cout << __func__ << std::endl;
  interval_set<int> tr;
  tr.insert (std::make_pair (0, 20));
  
  mu_assert (tr.size () == 1);
  mu_assert (!tr.empty ());

  interval_set<int>::const_iterator it = tr.begin ();
  mu_assert (it != tr.end ());
  mu_assert (tr.rbegin () != tr.rend ());
  
  std::pair<const int, const int> temp (0, 20); 
  mu_assert (*it == temp);
  mu_assert (it++ == tr.begin ());
  mu_assert (it-- == tr.end ());
  mu_assert (++it == tr.end ());
  mu_assert (--it == tr.begin ());

  return 0;
}

static const char* left_gap () {
  std::cout << __func__ << std::endl;
  interval_set<int> tr;
  tr.insert (std::make_pair (0, 20));
  tr.insert (std::make_pair (-10, -5));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-10, -5));
  ls.push_back (std::make_pair (0, 20));

  mu_assert (tr.size () == 2);
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));

  return 0;
}

static const char* left_intersect () {
  std::cout << __func__ << std::endl;
 
  interval_set<int> tr;
  tr.insert (std::make_pair (-7, 20));
  tr.insert (std::make_pair (-10, -7));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-10, 20));

  mu_assert (tr.size () == 1);
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));
  return 0;
}

static const char* right_intersect () {
  std::cout << __func__ << std::endl;

  interval_set<int> tr;
  tr.insert (std::make_pair (-10, -7));
  tr.insert (std::make_pair (-7, 20));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-10, 20));

  mu_assert (tr.size () == 1);
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));

  return 0;
}

static const char* right_gap () {
  std::cout << __func__ << std::endl;

  interval_set<int> tr;
  tr.insert (std::make_pair (-10, -5));
  tr.insert (std::make_pair (0, 20));
  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-10, -5));
  ls.push_back (std::make_pair (0, 20));

  mu_assert (tr.size () == 2);
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));

  return 0;
}

static const char* equals () {
  std::cout << __func__ << std::endl;
  
  interval_set<int> tr;
  tr.insert (std::make_pair (-10, -7));
  tr.insert (std::make_pair (-10, -7));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-10, -7));

  mu_assert (tr.size () == 1);
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));
  
  return 0;
}

static const char* smaller () {
  std::cout << __func__ << std::endl;

  interval_set<int> tr;
  tr.insert (std::make_pair (-10, 20));
  tr.insert (std::make_pair (-7, 13));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-10, 20));

  mu_assert (tr.size () == 1);
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));

  return 0;
}

static const char* bigger () {
  std::cout << __func__ << std::endl;
  
  interval_set<int> tr;
  tr.insert (std::make_pair (-7, 13));
  tr.insert (std::make_pair (-10, 20));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-10, 20));

  mu_assert (tr.size () == 1);
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));

  return 0;
}

const char* all_tests () {
  mu_run_test (default_ctor);
  mu_run_test (insert_span);
  mu_run_test (left_gap);
  mu_run_test (left_intersect);
  mu_run_test (right_intersect);
  mu_run_test (right_gap);
  mu_run_test (equals);
  mu_run_test (smaller);
  mu_run_test (bigger);
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
