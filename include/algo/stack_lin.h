#pragma once

#include <algorithm>
#include <memory>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "commons/interval_tree.h"
#include "commons/segment_tree.h"
#include "linputils.h"

namespace linp {

namespace stack {

using add_methods = method_group<Method::PUSH>;
using remove_methods = method_group<Method::POP>;

constexpr int PERM_MULTI_LAYERS = -1;
constexpr int PERM_INF_LAYERS = -2;

template <typename value_type>
struct stack_perm_segtree {
  typedef int pos_t;
  typedef std::pair<int, value_type> node_value_t;

  struct stack_segment_tree_node_zero {
    static constexpr node_value_t value{{}, {}};
  };

  struct stack_segment_tree_node_updater {
    inline node_value_t& operator()(node_value_t& a, const node_value_t& b) {
      a.first += b.first;
      a.second += b.second;
      return a;
    }
  };

  struct stack_segment_tree_point_remover {
    inline void operator()(node_value_t& v) {
      // we are only removing values anyways
      v.first = std::numeric_limits<int>::max();
    }
  };

  typedef segment_tree<node_value_t, stack_segment_tree_node_zero,
                       stack_segment_tree_node_updater,
                       stack_segment_tree_point_remover>
      segtree_t;

  stack_perm_segtree(const history_t<value_type>& hist, size_t n) : n{n} {
    std::vector<node_value_t> initializer(n << 1,
                                          stack_segment_tree_node_zero::value);
    for (const operation_t<value_type>& o : hist) {
      if (o.method == PUSH)
        critIntervals[o.value].start = o.endTime;
      else if (o.method == POP)
        critIntervals[o.value].end = o.startTime;
    }
    for (const auto& [value, intvl] : critIntervals)
      if (intvl.start < intvl.end) {
        initializer[intvl.start] = {1, value};
        initializer[intvl.end] = {-1, -value};
      }
    node_value_t prefixSum = stack_segment_tree_node_zero::value;
    for (node_value_t& pr : initializer)
      pr = stack_segment_tree_node_updater()(prefixSum, pr);
    segTree = std::make_unique<segtree_t>(initializer, (n << 1) - 1);
  }

  void remove_subhistory(const value_type& v) {
    auto& [b, e] = critIntervals[v];
    segTree->update_range(b, e - 1, {-1, -v});
    for (const time_type& t : waitingReturns[v]) pendingReturns.push_back(t);
  }

  std::pair<pos_t, std::optional<value_type>> get_permissive() {
    if (!pendingReturns.empty()) {
      pos_t t = pendingReturns.back();
      pendingReturns.pop_back();
      return {t, std::nullopt};
    }

    auto [layers, pos] = segTree->query_min();
    segTree->remove_point(pos);
    if (layers.first == 0) return {pos, std::nullopt};
    if (layers.first == 1) {
      value_type& val = layers.second;
      waitingReturns[val].push_back(pos);
      return {pos, val};
    }
    return {(layers.first <= n) ? PERM_MULTI_LAYERS : PERM_INF_LAYERS,
            std::nullopt};
  }

 private:
  std::unordered_map<value_type, std::vector<time_type>> waitingReturns;
  std::vector<time_type> pendingReturns;
  std::unique_ptr<segtree_t> segTree;
  size_t n;
  std::unordered_map<value_type, interval> critIntervals;
};

template <typename value_type>
bool is_linearizable(history_t<value_type>& hist, const value_type& emptyVal) {
  if (hist.empty()) return true;

  if (!extend_dist_history<value_type, add_methods, remove_methods>(hist,
                                                                    emptyVal))
    return false;

  events_t<value_type> events{get_events(hist)};
  if (!tune_events<value_type, add_methods, remove_methods>(events, emptyVal,
                                                            hist.back().id) ||
      !verify_empty<value_type, add_methods, remove_methods>(events, emptyVal))
    return false;

  time_type maxTime =
      std::get<0>(*std::ranges::max_element(events.begin(), events.end()));
  remove_empty(hist, emptyVal);

  auto mem_alloc =
      std::make_shared<memory_allocator<interval_tree_node>>(hist.size() << 1);
  interval_tree<decltype(mem_alloc)> ops{mem_alloc};
  std::unordered_map<value_type, interval_tree<decltype(mem_alloc)>> opByVal;
  std::vector<value_type> startTimeToVal(maxTime + 1);
  stack_perm_segtree<value_type> sst{hist, static_cast<size_t>(maxTime)};

  for (const auto& o : hist) {
    interval itr{static_cast<int>(o.startTime), static_cast<int>(o.endTime)};
    ops.insert(itr);
    startTimeToVal[o.startTime] = o.value;
    auto [map_iter, _] = opByVal.try_emplace(o.value, mem_alloc);
    map_iter->second.insert(itr);
  }

  while (!ops.empty()) {
    auto [pos, optVal] = sst.get_permissive();
    if (pos == PERM_MULTI_LAYERS) return false;
    if (pos == PERM_INF_LAYERS) return true;

    std::vector<interval> overlaps =
        optVal ? opByVal.at(*optVal).query(pos) : ops.query(pos);
    for (const interval& itr : overlaps) {
      value_type val = startTimeToVal[itr.start];
      opByVal.at(val).remove(itr);
      ops.remove(itr);
      if (opByVal.at(val).empty()) sst.remove_subhistory(val);
    }
  }

  return true;
}

template <typename value_type>
bool is_linearizable_x(history_t<value_type>& hist,
                       const value_type& emptyVal) {
  if (hist.empty()) return true;

  if (!extend_dist_history<value_type, add_methods, remove_methods>(hist,
                                                                    emptyVal))
    return false;

  events_t<value_type> events{get_events(hist)};
  if (!tune_events_x<value_type, add_methods>(events, emptyVal,
                                              hist.back().id) ||
      !verify_empty<value_type, add_methods, remove_methods>(events, emptyVal))
    return false;

  time_type maxTime =
      std::get<0>(*std::ranges::max_element(events.begin(), events.end()));
  remove_empty(hist, emptyVal);

  auto mem_alloc =
      std::make_shared<memory_allocator<interval_tree_node>>(hist.size());
  std::vector<value_type> startTimeToVal(maxTime + 1);
  stack_perm_segtree<value_type> sst{hist, static_cast<size_t>(maxTime)};

  std::vector<interval> intervals;
  intervals.reserve(hist.size());
  for (const auto& o : hist) {
    intervals.emplace_back(static_cast<int>(o.startTime),
                           static_cast<int>(o.endTime));
    startTimeToVal[o.startTime] = o.value;
  }
  interval_tree ops{mem_alloc, std::move(intervals)};

  std::unordered_set<value_type> pending;
  while (!ops.empty()) {
    auto [pos, optVal] = sst.get_permissive();
    if (pos == PERM_MULTI_LAYERS) return false;
    if (pos == PERM_INF_LAYERS) return true;

    if (optVal) continue;

    for (const interval& itr : ops.query(pos)) {
      ops.remove(itr);
      value_type val = startTimeToVal[itr.start];
      if (!pending.insert(val).second) sst.remove_subhistory(val);
    }
  }

  return true;
}

};  // namespace stack

}  // namespace linp