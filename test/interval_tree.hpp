#ifndef __interval_tree_hpp__
#define __interval_tree_hpp__

#include <utility>
#include <iostream>
#include <cassert>

class interval_tree {

private:
  class tree_iterator;

  class interval_node  {
  public:
    std::pair<int, int> span;
    interval_node* left;
    interval_node* right;
    interval_node* parent;

  public:
    interval_node (const std::pair<int,int>& sp) :
      span(sp),
      left(0),
      right(0),
      parent(0)
    {}

    void insert(const std::pair<int,int>& sp, interval_node** dptr) {
      //Span inserted on left with no intersection
      if (sp.second < span.first) {
	interval_node* new_root = new interval_node (std::make_pair (sp.first, span.second));
	new_root->left = new interval_node (sp);
	new_root->left->parent = new_root;
	new_root->right = this;
	new_root->right->parent = new_root;
	*dptr = new_root;
	return;
      }
      //Span inserted on right with no intersection
      else if (sp.first > span.second) {
	interval_node* new_root = new interval_node (std::make_pair (span.first, sp.second));
	new_root->right = new interval_node (sp);
	new_root->right->parent = new_root;
	new_root->left = this;
	new_root->left->parent = new_root;
	*dptr = new_root;
	return;
      }
      //Span inserted on left with left-only intersection
      else if (sp.first < span.first && sp.second >= span.first && sp.second <= span.second) {
	  for (interval_node* ptr = this; ptr != 0; ptr = ptr->left) {
	    ptr->span.first = sp.first;
	  }
	  insert (sp, dptr);
	  return;
	}
	//Span inserted on right with right-only intersection
	else if (sp.second > span.second && sp.first <= span.second && sp.first >= span.first) {
	    for (interval_node* ptr = this; ptr != 0; ptr = ptr->left) {
	      ptr->span.first = sp.first;
	    }
	    insert (sp, dptr);
	    return;
	  }
	//Span is larger than ours
	else if (sp.first < span.first && sp.second > span.second) {
	  if (left != 0) {
	    left->destroy ();
	    left = 0;
	  }
	  if (right != 0) {
	    right->destroy ();
	    right = 0;
	  }
	  span = sp;
	}
	//It is somewhere inside of our span (equivalency possible)
	else {
	  assert (false);
	}


      /*
      if (sp.first < span.first && sp.second > span.second) { //if we intersect on both sides
	span = sp; //become the other span
      }
      else if (sp.first < span.first && sp.second > span.first) { //if we intersect only on the left
	span.first = sp.first; //expand on the left
      }

      else if (sp.first < span.second && sp.second > span.second) { //if we intersect only on the right
	span.second = sp.second; //expand on the right
      }
      //The entire rest of the method only applies to branches.
      //if I have children:
      if (left != 0){
	//if it falls between my children:
	if (sp.first > left->span.second && sp.second < right->span.first){
	  std::pair<int,int> new_span = std::make_pair(left->span.first,sp.second);
	  interval_node sp_child(sp);
	  interval_node new_child(new_span, left, &sp_child);
	  left = &new_child;
	}
	//Otherwise, DATA STRUCTURES GONE WILD.
	else  {
	  left->insert(sp);
	  right->insert(sp);

	  if (left->span.second > right->span.first) {  //if my children overlap
	    if (left->left == 0 && right->left == 0) {  //if both are leaves
	      left->destroy();
	      left = 0;
	      right->destroy();
	      right = 0;
	    }
	    else if (left->left != 0 && right->left == 0) { //if left is a branch and right is a leaf
	      adopt_left ();
	    }
	    else if (left->left == 0 && right->left != 0) { //if left is a leaf and right is a branch
	      adopt_right();
	    }
	    else { //if both left and right are branches
	      if (left->right->span.first < right->left->span.first && left->right->span.second > right->left->span.second) { //if left's right is bigger than right's left
		right->insert (left->right->span);
		adopt_left ();
	      }
	      else { //if right's left is bigger than left's right
		left->insert (right->left->span);
		adopt_right ();
	      }
	    }
	  }
	}
      } else {
	//I don't have children but it's outside of my span
	} */
    }
    
    void adopt_left () {
      left->right->destroy();
      left->right = 0;
      interval_node* temp = left;
      left = left->left;
      temp->left = 0;
      temp->destroy();
    }

    void adopt_right () {
      right->left->destroy();
      right->left = 0;
      interval_node* temp = right;
      right = right->right;
      temp->right = 0;
      temp->destroy();
    }
    
    void destroy() {
      if (left != 0)
	left->destroy();
      if (right != 0)
	right->destroy();

      delete this;
    }

    interval_node* find(const std::pair<int,int>& sp) {
      if (left == 0){
	if (sp.first >= span.first && sp.second <= span.second) {
	  return this;
	}
	else {
	  return 0;
	}
      }
      else {
	interval_node* l = left->find(sp);
	if (l != 0)
	  return l;
	else return right->find(sp);
      }
    }
    

    void erase (std::pair<int,int> sp){
      
    }

    bool check () {
      if (left == 0 && right == 0) {  //I am a leaf
	return true;
      }
      else {
	return (left != 0 && right != 0) &&  //I have two children
	  (span.first == left->span.first) && //My first is my left child's first
	  (span.second == right->span.second) && //My second is my right child's second
	  (left->span.second < right->span.first) //My children do not overlap
	  && left->check() && right->check(); //The rest of the tree is ok
      }
    }

    const std::pair<int,int>* span_ptr () const {
      return &span;
    }
    
    std::pair<int,int>& span_ref () {
      return span;
    }

    interval_node* first_leaf () {
      if (left == 0)
	return this;
      else
	return left->first_leaf();
    }

    interval_node* last_leaf () {
      if (right == 0) {
	return this;
      }
      else {
	return right->last_leaf ();
      }
    }

    interval_node* prev_leaf () {
      assert (left == 0 && right == 0);
      if (parent == 0) {
	return 0;
      }
      return parent->prev_leaf (this);
    }

    interval_node* next_leaf () {
      assert (left == 0 && right == 0);
      if (parent == 0) {
	return 0;
      }
      return parent->next_leaf (this);
    }
    
  private:
    interval_node* prev_leaf (interval_node* child) {
      assert (left != 0 && right != 0);
      if (child == right) {
	return left->last_leaf ();
      }
      else if (child == left) {
	if (parent == 0) {
	  return 0;
	}
	return parent->prev_leaf (this);
      }
      else {
	assert (false);
      }
    }

    interval_node* next_leaf (interval_node* child) {
      assert (left != 0 && right != 0);
      if (child == left) {
	return right->first_leaf ();
      }
      else if (child == right) {
	if (parent == 0) {
	  return 0;
	}
	return parent->next_leaf (this);
      }
      else {
	assert (false);
      }
    }
  };

  class tree_iterator :
    public std::iterator <std::bidirectional_iterator_tag, std::pair<int,int> > {
  private:
    interval_node* root;
    interval_node* ptr;

  public:
    tree_iterator () :
      root (0),
      ptr (0)
    {

    }
    tree_iterator (interval_node* r, interval_node* p = 0) :
      root (r),
      ptr (p)
    {}

    bool operator== (const tree_iterator& other) const {
      return ptr == other.ptr;
    }

    bool operator!= (const tree_iterator& other) const {
      return ptr != other.ptr;
    }

    std::pair<int,int>& operator* () const {
      return ptr->span_ref ();
    }

    const std::pair<int,int>* operator-> () const {
      return ptr->span_ptr ();
    }

    tree_iterator& operator++ () {
      ptr = ptr->next_leaf ();
      return *this;
    }

    tree_iterator operator++ (int /* */) {
      tree_iterator retval = *this;
      ptr = ptr->next_leaf ();
      return retval;
    }

    tree_iterator& operator-- () {
      if (root != 0) {
	if (ptr != 0) {
	  ptr = ptr->prev_leaf ();
	}
	else {
	  ptr = root->last_leaf ();
	}
      }
      return *this;
    }

    tree_iterator operator-- (int /* */) {
      tree_iterator retval = *this;
      if (root != 0) {
	if (ptr != 0) {
	  ptr = ptr->prev_leaf ();
	} 
	else {
	  ptr = root->last_leaf ();
	}
      }
      return retval;
    }
  };

  interval_node* root;

public:
  typedef std::pair<int,int> value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;

  typedef ptrdiff_t difference_type;
  typedef size_t size_type;
  typedef tree_iterator iterator;
  typedef tree_iterator const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  iterator begin () {
    if (root != 0)  {
      return tree_iterator (root, root->first_leaf());
    }
    else { 
      return tree_iterator ();
    }
  }
  iterator end () {
    return tree_iterator (root);
  }
  const_iterator begin() const {
    if (root != 0) {
      return tree_iterator (root, root->first_leaf ());
    }
    else {
      return tree_iterator ();
    }
  }
  const_iterator end () const {
    return tree_iterator (root);
  }
  reverse_iterator rbegin() {
    return reverse_iterator (end ());
  }
  reverse_iterator rend() {
    return reverse_iterator (begin ());
  }
  const_reverse_iterator rbegin() const {
    return reverse_iterator (end ());
  }
  const_reverse_iterator rend() const {
    return reverse_iterator (begin ());
  }

  size_type size () const {
    //TODO: this requires iterating over the whole tree.
    size_type retval = 0;
    for (const_iterator pos = begin(); pos != end(); ++pos, ++retval) { }

    return retval;
  }

  bool empty () const {
    return root == 0; 
  }  

  interval_tree() :
    root(0)
  {}

  void insert(const std::pair<int,int>& sp) {
    if (sp.first < sp.second) {
      if (root != 0) 
	root->insert(sp, &root);
      else
	root = new interval_node(sp);
      assert (root->check ());
    }
  }

  tree_iterator find(const std::pair<int,int>& sp) {
    if (root != 0)
      return tree_iterator(root, root->find(sp));
    else
      return tree_iterator(root);
  }

  void erase(const std::pair<int,int>& sp) {
    if (root != 0)
      root->erase(sp);
  }

};

bool operator== (const interval_tree&, const interval_tree&) { return false; } //TODO: fix me
bool operator< (const interval_tree&, const interval_tree&) { return false; } //TODO: fix me


#endif
