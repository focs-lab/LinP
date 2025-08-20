#pragma once

#include <unordered_map>

#include "linputils.h"

namespace linp {

namespace set {

using add_methods = method_group<Method::INSERT>;
using remove_methods = method_group<Method::REMOVE>;

template <typename value_type>
bool is_linearizable(history_t<value_type>& hist, const value_type& emptyVal) {
  if (hist.empty()) return true;

  if (!extend_dist_history<value_type, add_methods, remove_methods>(hist,
                                                                    emptyVal))
    return false;

  std::unordered_map<value_type, std::pair<time_type, time_type>> minResMaxInv;
  for (const auto& o : hist)
    if (o.method != Method::CONTAINS_FALSE) {
      auto& elem =
          minResMaxInv.try_emplace(o.value, MAX_TIME, MIN_TIME).first->second;
      elem.first = std::min(elem.first, o.endTime);
      elem.second = std::max(elem.second, o.startTime);
    }

  for (const auto& o : hist) {
    auto& elem = minResMaxInv.at(o.value);
    if (o.method != Method::CONTAINS_FALSE) {
      if (o.method == INSERT && o.startTime > elem.first) return false;
      if (o.method == REMOVE && o.endTime < elem.second) return false;
    } else if (elem.first < o.startTime && o.endTime < elem.second)
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

  std::unordered_map<value_type, time_type> minRes;
  for (const auto& o : hist) {
    auto& elem = minRes.try_emplace(o.value, MAX_TIME).first->second;
    elem = std::min(elem, o.endTime);
  }

  for (const auto& o : hist)
    if (o.method == INSERT && o.startTime > minRes.at(o.value)) return false;

  return true;
}

};  // namespace set

}  // namespace linp