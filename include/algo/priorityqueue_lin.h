#pragma once

#include <algorithm>

#include "commons/segment_tree.h"
#include "linputils.h"

namespace linp {

namespace priorityqueue {

using add_methods = method_group<Method::INSERT>;
using remove_methods = method_group<Method::POLL>;

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

  remove_empty(hist, events, emptyVal);

  time_type maxTime =
      std::get<0>(*std::ranges::max_element(events.begin(), events.end()));
  segment_tree<value_type> segTree{maxTime};
  std::sort(hist.begin(), hist.end(), [](const auto& a, const auto& b) {
    return a.value > b.value || (a.value == b.value && a.id < b.id);
  });

  value_type currVal = emptyVal;
  time_type minRes, maxInv;
  for (const auto& op : hist) {
    if (currVal != op.value) {
      if (currVal != emptyVal && minRes < maxInv)
        segTree.update_range(minRes, maxInv - 1, 1);
      currVal = op.value;
      minRes = op.endTime;
      maxInv = op.startTime;
    } else {
      minRes = std::min(minRes, op.endTime);
      maxInv = std::max(maxInv, op.startTime);
    }

    if (op.method != Method::INSERT &&
        segTree.query_min_range(op.startTime, op.endTime - 1).first > 0)
      return false;
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

  remove_empty(hist, events, emptyVal);

  time_type maxTime =
      std::get<0>(*std::ranges::max_element(events.begin(), events.end()));
  segment_tree<value_type> segTree{maxTime};
  std::sort(hist.begin(), hist.end(), [](const auto& a, const auto& b) {
    return a.value > b.value ||
           // insert to be processed before poll
           (a.value == b.value && a.method == Method::INSERT);
  });

  time_type insertRes;
  for (const auto& op : hist) {
    if (op.method == Method::INSERT)
      insertRes = op.endTime;
    else {
      if (segTree.query_min_range(op.startTime, op.endTime - 1).first > 0)
        return false;
      if (insertRes < op.startTime)
        segTree.update_range(insertRes, op.startTime - 1, 1);
    }
  }

  return true;
}

};  // namespace priorityqueue

}  // namespace linp