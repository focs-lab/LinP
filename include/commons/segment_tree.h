#pragma once

#include <vector>

namespace linp {

// `O(log n)` range update
// `O(log n)` point query
// `O(1)` minimum query segment tree
// minimum size 1

template <typename value_type>
struct default_segment_tree_zero {
  static constexpr value_type value{};
};

template <typename value_type>
struct default_segment_tree_point_remover {
  inline void operator()(value_type& v) {
    v = std::numeric_limits<value_type>::max();
  }
};

template <typename value_type>
struct default_segment_tree_updater {
  inline void operator()(value_type& a, const value_type& b) { a += b; }
};

template <typename value_type,
          typename zero_allocator = default_segment_tree_zero<value_type>,
          typename updater = default_segment_tree_updater<value_type>,
          typename remover = default_segment_tree_point_remover<value_type>>
struct segment_tree {
 public:
  segment_tree(const size_t& size) : tree(size << 2), size(size) {
    build(1, 0, size - 1);
  }

  template <typename value_ptr>
  segment_tree(const value_ptr& arr, size_t size)
      : tree(size << 2), size(size) {
    build(1, 0, size - 1, arr);
  }

  void update_range(int l, int r, value_type addend) {
    update_range(1, 0, size - 1, l, r, addend);
  }

  void remove_point(int pnt) { remove_point(1, 0, size - 1, pnt); }

  std::pair<value_type, int> query_min() {
    return {tree[1].min_value, tree[1].min_pos};
  }

  std::pair<value_type, int> query_min_range(int l, int r) {
    return query_min_range(1, 0, size - 1, l, r);
  }

 private:
  struct segment_tree_node {
    value_type min_value;
    value_type weight;
    int min_pos;
  };

  void build(int v, int tl, int tr) {
    tree[v] = {zero_allocator::value, zero_allocator::value, tl};
    if (tl != tr) {
      int tm = (tl + tr) >> 1;
      build(v << 1, tl, tm);
      build((v << 1) + 1, tm + 1, tr);
    }
  }

  template <typename value_ptr>
  void build(int v, int tl, int tr, const value_ptr& arr) {
    if (tl == tr) {
      tree[v] = {arr[tl], arr[tl], tl};
    } else {
      int tm = (tl + tr) >> 1;
      build(v << 1, tl, tm, arr);
      build((v << 1) + 1, tm + 1, tr, arr);
      update_node(v);
    }
  }

  void update_node(int par) {
    int left = par << 1;
    auto take = (tree[left].min_value <= tree[left + 1].min_value)
                    ? tree[left]
                    : tree[left + 1];
    tree[par].min_value = take.min_value;
    tree[par].min_pos = take.min_pos;
  }

  void propagate(int v) {
    auto& node = tree[v];
    if (node.weight == zero_allocator::value) return;
    apply(v << 1, node.weight);
    apply((v << 1) + 1, node.weight);
    node.weight = zero_allocator::value;
  }

  void apply(int v, value_type addend) {
    updater()(tree[v].min_value, addend);
    updater()(tree[v].weight, addend);
  }

  void remove(int v) { remover()(tree[v].min_value); }

  void update_range(int v, int tl, int tr, int l, int r, value_type addend) {
    if (l <= tl && tr <= r) {
      apply(v, addend);
      return;
    }
    propagate(v);
    int tm = (tl + tr) >> 1;
    if (l <= tm) update_range(v << 1, tl, tm, l, r, addend);
    if (tm < r) update_range((v << 1) + 1, tm + 1, tr, l, r, addend);
    update_node(v);
  }

  void remove_point(int v, int tl, int tr, int p) {
    if (tl == tr) {
      remove(v);
      return;
    }
    propagate(v);
    int tm = (tl + tr) >> 1;
    if (p <= tm)
      remove_point(v << 1, tl, tm, p);
    else
      remove_point((v << 1) + 1, tm + 1, tr, p);
    update_node(v);
  }

  std::pair<value_type, int> query_min_range(int v, int tl, int tr, int l,
                                             int r) {
    if (l > r) return {std::numeric_limits<int>::max(), -1};
    if (l == tl && r == tr) return {tree[v].min_value, tree[v].min_pos};
    propagate(v);
    int tm = (tl + tr) >> 1;
    auto left_res = query_min_range(v << 1, tl, tm, l, std::min(r, tm));
    auto right_res =
        query_min_range((v << 1) + 1, tm + 1, tr, std::max(l, tm + 1), r);
    return (left_res.first <= right_res.first) ? left_res : right_res;
  }

  // root at `1`, left child at `2*par`, right child at `2*par+1`
  // for odd size ranges, mid belongs to right child
  std::vector<segment_tree_node> tree;
  int size;
};

}  // namespace linp