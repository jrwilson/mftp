#include <mftp/interval_set.hpp>
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

static const char* insert_single () {
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

static const char* insert_single_left_gap () {
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

static const char* insert_single_left_intersect () {
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

static const char* insert_single_right_intersect () {
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

static const char* insert_single_right_gap () {
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

static const char* insert_equal () {
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

static const char* insert_smaller () {
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

static const char* insert_bigger () {
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

static const char* insert_double_intersect () {
  std::cout << __func__ << std::endl;
  
  interval_set<int> tr;
  tr.insert (std::make_pair (-5, 0));
  tr.insert (std::make_pair (10, 15));
  tr.insert (std::make_pair (-2, 12));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-5, 15));

  mu_assert (tr.size () == 1);
  mu_assert (std::equal (tr.begin (), tr.end (), ls.begin ()));

  return 0;
}

static const char* insert_triple_intersect () {
  std::cout << __func__ << std::endl;
  
  interval_set<int> tr;
  tr.insert (std::make_pair (-5, 0));
  tr.insert (std::make_pair (10, 15));
  tr.insert (std::make_pair (20,25));
  tr.insert (std::make_pair (-2, 22));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-5, 25));

  mu_assert (tr.size () == 1);
  mu_assert (std::equal (tr.begin (), tr.end (), ls.begin ()));

  return 0;  
}

static const char* insert_triple_overlap () {
  std::cout << __func__ << std::endl;
  
  interval_set<int> tr;
  tr.insert (std::make_pair (-5, 0));
  tr.insert (std::make_pair (10, 15));
  tr.insert (std::make_pair (20,25));
  tr.insert (std::make_pair (-7, 27));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-7, 27));

  mu_assert (tr.size () == 1);
  mu_assert (std::equal (tr.begin (), tr.end (), ls.begin ()));

  return 0;  
}

static const char* insert_triple_intersect12 () {
  std::cout << __func__ << std::endl;
  
  interval_set<int> tr;
  tr.insert (std::make_pair (-5, 0));
  tr.insert (std::make_pair (10, 15));
  tr.insert (std::make_pair (20,25));
  tr.insert (std::make_pair (-2, 17));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-5, 17));
  ls.push_back (std::make_pair (20, 25));

  mu_assert (tr.size () == 2);
  mu_assert (std::equal (tr.begin (), tr.end (), ls.begin ()));

  return 0;  
}

static const char* erase_single_left_gap () {
  std::cout << __func__ << std::endl;
  interval_set<int> tr;
  tr.insert (std::make_pair (0, 20));
  tr.erase (std::make_pair (-10, 0));
  
  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (0, 20));

  mu_assert (tr.size () == ls.size ());
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));

  return 0;
}

static const char* erase_single_left_intersect () {
  std::cout << __func__ << std::endl;
 
  interval_set<int> tr;
  tr.insert (std::make_pair (-7, 20));
  tr.erase (std::make_pair (-10, -6));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-6, 20));

  mu_assert (tr.size () == ls.size ());
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));
  return 0;
}

static const char* erase_single_right_intersect () {
  std::cout << __func__ << std::endl;

  interval_set<int> tr;
  tr.insert (std::make_pair (-10, -7));
  tr.erase (std::make_pair (-8, 20));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-10, -8));

  mu_assert (tr.size () == ls.size ());
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));

  return 0;
}

static const char* erase_single_right_gap () {
  std::cout << __func__ << std::endl;

  interval_set<int> tr;
  tr.insert (std::make_pair (-10, -5));
  tr.erase (std::make_pair (-5, 20));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-10, -5));

  mu_assert (tr.size () == ls.size ());
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));

  return 0;
}

static const char* erase_equal () {
  std::cout << __func__ << std::endl;
  
  interval_set<int> tr;
  tr.insert (std::make_pair (-10, -7));
  tr.erase (std::make_pair (-10, -7));

  mu_assert (tr.empty ());
  
  return 0;
}

static const char* erase_smaller () {
  std::cout << __func__ << std::endl;

  interval_set<int> tr;
  tr.insert (std::make_pair (-10, 20));
  tr.erase (std::make_pair (-7, 13));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-10, -7));
  ls.push_back (std::make_pair (13, 20));

  mu_assert (tr.size () == ls.size ());
  mu_assert (!tr.empty ());
  
  mu_assert (std::equal (tr.begin (), tr.end(), ls.begin ()));

  return 0;
}

static const char* erase_bigger () {
  std::cout << __func__ << std::endl;
  
  interval_set<int> tr;
  tr.insert (std::make_pair (-7, 13));
  tr.erase (std::make_pair (-10, 20));

  mu_assert (tr.empty ());
  
  return 0;
}

static const char* erase_double_intersect () {
  std::cout << __func__ << std::endl;
  
  interval_set<int> tr;
  tr.insert (std::make_pair (-5, 0));
  tr.insert (std::make_pair (10, 15));
  tr.erase (std::make_pair (-2, 12));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-5, -2));
  ls.push_back (std::make_pair (12,15));

  mu_assert (tr.size () == ls.size ());
  mu_assert (std::equal (tr.begin (), tr.end (), ls.begin ()));

  return 0;
}

static const char* erase_triple_intersect () {
  std::cout << __func__ << std::endl;
  
  interval_set<int> tr;
  tr.insert (std::make_pair (-5, 0));
  tr.insert (std::make_pair (10, 15));
  tr.insert (std::make_pair (20,25));
  tr.erase (std::make_pair (-2, 22));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-5, -2));
  ls.push_back (std::make_pair (22, 25));

  mu_assert (tr.size () == ls.size ());
  mu_assert (std::equal (tr.begin (), tr.end (), ls.begin ()));

  return 0;  
}

static const char* erase_triple_overlap () {
  std::cout << __func__ << std::endl;
  
  interval_set<int> tr;
  tr.insert (std::make_pair (-5, 0));
  tr.insert (std::make_pair (10, 15));
  tr.insert (std::make_pair (20,25));
  tr.erase (std::make_pair (-7, 27));

  mu_assert (tr.empty ());

  return 0;  
}

static const char* erase_triple_intersect12 () {
  std::cout << __func__ << std::endl;
  
  interval_set<int> tr;
  tr.insert (std::make_pair (-5, 0));
  tr.insert (std::make_pair (10, 15));
  tr.insert (std::make_pair (20,25));
  tr.erase (std::make_pair (-2, 17));

  std::list<std::pair<const int, const int> > ls;
  ls.push_back (std::make_pair (-5, -2));
  ls.push_back (std::make_pair (20, 25));

  mu_assert (tr.size () == ls.size ());
  mu_assert (std::equal (tr.begin (), tr.end (), ls.begin ()));

  return 0;  
}

const char* all_tests () {
  mu_run_test (default_ctor);
  mu_run_test (insert_single);
  mu_run_test (insert_single_left_gap);
  mu_run_test (insert_single_left_intersect);
  mu_run_test (insert_single_right_intersect);
  mu_run_test (insert_single_right_gap);
  mu_run_test (insert_equal);
  mu_run_test (insert_smaller);
  mu_run_test (insert_bigger);

  mu_run_test (insert_double_intersect);
  mu_run_test (insert_triple_intersect);
  mu_run_test (insert_triple_overlap);
  mu_run_test (insert_triple_intersect12);

  mu_run_test (erase_single_left_gap);
  mu_run_test (erase_single_left_intersect);
  mu_run_test (erase_single_right_intersect);
  mu_run_test (erase_single_right_gap);
  mu_run_test (erase_equal);
  mu_run_test (erase_smaller);
  mu_run_test (erase_bigger);

  mu_run_test (erase_double_intersect);
  mu_run_test (erase_triple_intersect);
  mu_run_test (erase_triple_overlap);
  mu_run_test (erase_triple_intersect12);

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
