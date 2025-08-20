#pragma once

#include <algorithm>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "linputils.h"

namespace linp {

namespace queue {

using add_methods = method_group<Method::ENQ>;
using remove_methods = method_group<Method::DEQ>;

template <typename value_type>
inline std::unordered_set<value_type> pendingVals;
template <typename value_type>
inline std::unordered_set<value_type> ignoreVals;
template <typename value_type>
inline std::vector<value_type> delayedVals;
template <typename value_type>
std::unordered_map<value_type, size_t> cntByVal;

template <typename value_type>
void upgrade_val(const value_type& val) {
  if (pendingVals<value_type>.erase(val))
    ignoreVals<value_type>.insert(val);
  else
    pendingVals<value_type>.insert(val);
}

template <typename value_type, typename event_iter, Method method_arg>
bool scan(event_iter& start, const event_iter& end) {
  event_iter temp = start;
  while (start != end) {
    const auto& [_, isInv, optr] = *start;
    const value_type& val = optr->value;

    if (ignoreVals<value_type>.count(val) || optr->method != method_arg) {
      ++start;
      continue;
    }
    if (!isInv) break;

    upgrade_val(val);
    ++start;
  }
  return temp != start;
}

template <typename value_type, typename event_iter>
bool scan_front(event_iter& start, const event_iter& end,
                std::optional<value_type>& last) {
  event_iter temp = start;
  while (start != end) {
    const auto& [_, isInv, optr] = *start;
    const value_type& val = optr->value;

    if (ignoreVals<value_type>.count(val) || optr->method == Method::ENQ) {
      ++start;
      continue;
    }

    if (last && ignoreVals<value_type>.count(*last)) last.reset();

    if (!last) {
      for (value_type& val : delayedVals<value_type>) upgrade_val(val);
      delayedVals<value_type>.clear();
    }

    if (isInv) {
      if (optr->method == Method::DEQ) {
        if (last && last != val)
          delayedVals<value_type>.push_back(val);
        else
          upgrade_val(val);
      }
    } else {
      if (!last) last = val;
      // no two existing values can respond and last must wait for confirmation
      if (last != val || optr->method == Method::DEQ) break;
    }

    ++start;
    continue;
  }
  return temp != start;
}

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

  std::sort(events.begin(), events.end());

  cntByVal<value_type>.clear();
  for (const auto& o : hist) ++cntByVal<value_type>[o.value];

  // initializations
  std::optional<value_type> lastFront;
  pendingVals<value_type>.clear();
  ignoreVals<value_type>.clear();
  delayedVals<value_type>.clear();
  auto enqStart = events.begin();
  auto frontStart = events.begin();
  auto end = events.end();

  while (
      scan<value_type, decltype(enqStart), Method::ENQ>(enqStart, end) ||
      scan_front<value_type, decltype(frontStart)>(frontStart, end, lastFront));

  return enqStart == end && frontStart == end;
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

  std::sort(events.begin(), events.end());

  // initializations
  pendingVals<value_type>.clear();
  ignoreVals<value_type>.clear();
  auto enqStart = events.begin();
  auto deqStart = events.begin();
  const auto end = events.end();

  while (scan<value_type, decltype(enqStart), Method::ENQ>(enqStart, end) ||
         scan<value_type, decltype(deqStart), Method::DEQ>(deqStart, end));

  return enqStart == end && deqStart == end;
}

};  // namespace queue

}  // namespace linp