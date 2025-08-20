#pragma once

#include <stdint.h>

#include <memory>
#include <type_traits>
#include <vector>

#include "mem_alloc.h"

namespace linp {

struct interval {
  int start;
  int end;
};

struct interval_tree_node {
  interval intvl;
  int maxEnd;
  int height;
  interval_tree_node* left;
  interval_tree_node* right;

  interval_tree_node(interval i)
      : intvl(i), maxEnd(i.end), height(1), left(nullptr), right(nullptr) {}
};

template <typename T>
concept memory_allocator_ptr =
    std::is_same_v<typename std::pointer_traits<std::decay_t<T>>::element_type,
                   memory_allocator<interval_tree_node>>;

// interval tree efficient O(log n) insert/delete of intervals and `O(m log n)`
// point query
template <memory_allocator_ptr ptr_t>
struct interval_tree {
 public:
  interval_tree(ptr_t ptr) : mem_alloc_ptr(ptr), root(nullptr) {}

  interval_tree(ptr_t ptr, std::vector<interval>&& v) : mem_alloc_ptr(ptr) {
    std::sort(v.begin(), v.end(), [](const interval& a, const interval& b) {
      return a.start < b.start;
    });
    root = v.empty() ? nullptr : build(0, v.size() - 1, v);
  }

  // Newly inserted interval must have unique `start` and `end`
  void insert(interval i) { root = insert(root, i); }

  // `i` must exist in the tree for correctness
  void remove(interval i) { root = remove(root, i); }

  // Retrieves all intervals overlapping `point`. `O(m log n)` time complexity,
  // where `m` is the size of output and `n` is the size of tree
  std::vector<interval> query(int point) {
    std::vector<interval> result;
    query(root, point, result);
    return result;
  }

  bool empty() { return root == nullptr; }

 private:
  interval_tree_node* root;
  ptr_t mem_alloc_ptr;

  interval_tree_node* build(int l, int r, std::vector<interval>& v) {
    int mid = (l + r) >> 1;
    interval_tree_node* node =
        new (mem_alloc_ptr->alloc()) interval_tree_node(v[mid]);
    if (l < mid) node->left = build(l, mid - 1, v);
    if (mid < r) node->right = build(mid + 1, r, v);
    node->height = std::max(height(node->left), height(node->right)) + 1;
    node->maxEnd = std::max(node->intvl.end,
                            std::max(maxEnd(node->left), maxEnd(node->right)));
    return node;
  }

  int height(interval_tree_node* n) { return n ? n->height : 0; }

  int maxEnd(interval_tree_node* n) { return n ? n->maxEnd : INT32_MIN; }

  int getBalance(interval_tree_node* n) {
    return n ? height(n->left) - height(n->right) : 0;
  }

  interval_tree_node* rightRotate(interval_tree_node* y) {
    interval_tree_node* x = y->left;
    interval_tree_node* T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = std::max(height(y->left), height(y->right)) + 1;
    x->height = std::max(height(x->left), height(x->right)) + 1;

    y->maxEnd =
        std::max(y->intvl.end, std::max(maxEnd(y->left), maxEnd(y->right)));
    x->maxEnd =
        std::max(x->intvl.end, std::max(maxEnd(x->left), maxEnd(x->right)));

    return x;
  }

  interval_tree_node* leftRotate(interval_tree_node* x) {
    interval_tree_node* y = x->right;
    interval_tree_node* T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = std::max(height(x->left), height(x->right)) + 1;
    y->height = std::max(height(y->left), height(y->right)) + 1;

    x->maxEnd =
        std::max(x->intvl.end, std::max(maxEnd(x->left), maxEnd(x->right)));
    y->maxEnd =
        std::max(y->intvl.end, std::max(maxEnd(y->left), maxEnd(y->right)));

    return y;
  }

  interval_tree_node* autoBalance(interval_tree_node* node) {
    int balance = getBalance(node);
    if (balance >= 2) {                  // left heavy
      if (getBalance(node->left) == -1)  // left child is right heavy
        node->left = leftRotate(node->left);
      return rightRotate(node);
    }
    if (balance <= -2) {                 // right heavy
      if (getBalance(node->right) == 1)  // right child is left heavy
        node->right = rightRotate(node->right);
      return leftRotate(node);
    }
    return node;
  }

  interval_tree_node* insert(interval_tree_node* node, interval i) {
    if (!node) return new (mem_alloc_ptr->alloc()) interval_tree_node(i);

    if (i.start < node->intvl.start)
      node->left = insert(node->left, i);
    else
      node->right = insert(node->right, i);

    node->height = std::max(height(node->left), height(node->right)) + 1;
    node->maxEnd = std::max(i.end, node->maxEnd);

    return autoBalance(node);
  }

  interval_tree_node* minValueNode(interval_tree_node* node) {
    while (node->left != nullptr) node = node->left;
    return node;
  }

  interval_tree_node* remove(interval_tree_node* node, interval i) {
    if (!node) return node;

    if (i.start < node->intvl.start) {
      node->left = remove(node->left, i);
    } else if (i.start > node->intvl.start) {
      node->right = remove(node->right, i);
    } else if (!node->left || !node->right) {
      interval_tree_node* temp = node->left ? node->left : node->right;
      mem_alloc_ptr->free(node);
      node = temp;
    } else {
      interval_tree_node* temp = minValueNode(node->right);
      node->intvl = temp->intvl;
      node->right = remove(node->right, temp->intvl);
    }

    if (!node) return node;

    node->height = std::max(height(node->left), height(node->right)) + 1;
    node->maxEnd = std::max(node->intvl.end,
                            std::max(maxEnd(node->left), maxEnd(node->right)));

    return autoBalance(node);
  }

  void query(interval_tree_node* node, int point,
             std::vector<interval>& result) {
    if (!node) return;

    if (node->intvl.start <= point && point < node->intvl.end)
      result.push_back(node->intvl);

    if (node->left && node->left->maxEnd > point)
      query(node->left, point, result);

    if (node->right && node->intvl.start < point)
      query(node->right, point, result);
  }
};

}  // namespace linp