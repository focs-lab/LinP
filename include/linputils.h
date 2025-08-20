#pragma once

#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "definitions.h"

namespace linp {

/**
 * - checks for duplicated adds/removes of the same value
 * - extends history using first remove method
 * - O(n)
 */
template <typename value_type, typename add_group, typename remove_group>
bool extend_dist_history(history_t<value_type>& hist,
                         const value_type& emptyVal) {
  time_type maxTime = MIN_TIME;
  id_type maxId = 0;
  std::unordered_map<value_type, std::pair<int, int>> hasAddRemove;

  for (const auto& o : hist) {
    maxId = std::max(maxId, o.id);
    if (o.value == emptyVal) continue;

    auto [map_iter, _] = hasAddRemove.try_emplace(o.value, 0, 0);
    auto& [hasAdd, hasRemove] = map_iter->second;
    if (add_group::contains(o.method) && hasAdd++) return false;
    if (remove_group::contains(o.method) && hasRemove++) return false;

    maxTime = std::max(maxTime, o.endTime);
  }

  for (const auto& [key, value] : hasAddRemove) {
    auto& [hasAdd, hasRemove] = value;
    if (!hasAdd) return false;
    if (!hasRemove)
      hist.emplace_back(++maxId, remove_group::first, key, maxTime + 1,
                        maxTime + 2);
  }

  return true;
};

/**
 * `O(n)` sorting of events assuming distinct time, and the number of time is
 * not too large.
 */
template <typename value_type>
void counting_sort(events_t<value_type>& events) {
  if (events.empty()) return;  // Empty range check

  // Find the min and max key values
  auto max_it = std::max_element(events.begin(), events.end(),
                                 [&](const auto& a, const auto& b) {
                                   return std::get<0>(a) < std::get<0>(b);
                                 });
  int maxValue = std::get<0>(*max_it);

  std::vector<int> count(maxValue + 1, 0);
  events_t<value_type> output(events.size());

  for (auto it = events.begin(); it != events.end(); ++it)
    ++count[std::get<0>(*it)];

  for (int i = 1; i <= maxValue; ++i) count[i] += count[i - 1];

  for (auto it = events.begin(); it != events.end(); ++it)
    output[count[std::get<0>(*it)] - 1] = *it;

  std::swap(output, events);
}

/**
 * retrieves events, O(n)
 */
template <typename value_type>
events_t<value_type> get_events(history_t<value_type>& hist) {
  events_t<value_type> events;
  events.reserve(hist.size() << 1);
  for (operation_t<value_type>& o : hist) {
    events.emplace_back(o.startTime, true, &o);
    events.emplace_back(o.endTime, false, &o);
  }
  return events;
}

/**
 * tune events so that add responds first and remove invokes last, O(n log n)
 * important: resulting events might not be sorted
 */
template <typename value_type, typename add_group, typename remove_group>
bool tune_events(events_t<value_type>& events, const value_type& emptyVal,
                 const id_type& maxId) {
  std::sort(events.begin(), events.end());

  using oper_ptr = operation_t<value_type>*;
  struct value_event_data {
    oper_ptr add_op = nullptr;
    oper_ptr remove_op = nullptr;
    bool add_ended = false;
    bool remove_ended = false;
    std::deque<oper_ptr> others;
  };
  std::unordered_map<value_type, value_event_data> ongoings_val;
  std::vector<bool> ongoings_op(maxId + 1, false);

  time_type time = MIN_TIME;
  for (const auto& [_, isInv, o] : events) {
    const value_type& value = o->value;
    value_event_data& data = ongoings_val[value];

    if (value == emptyVal) {
      time_type& ts = isInv ? o->startTime : o->endTime;
      ts = ++time;
      continue;
    }

    if (isInv) {
      o->startTime = ++time;
      if (add_group::contains(o->method)) {
        data.add_op = o;
        for (oper_ptr op : data.others) op->startTime = ++time;
        if (data.remove_op) data.remove_op->startTime = ++time;
      } else if (remove_group::contains(o->method)) {
        data.remove_op = o;
      } else {
        ongoings_op[o->id] = true;
        data.others.push_back(o);
        if (data.remove_op) {
          if (data.remove_ended) return false;
          data.remove_op->startTime = ++time;
        }
      }
    } else {
      if (add_group::contains(o->method)) {
        o->endTime = ++time;
        data.add_ended = true;
      } else if (remove_group::contains(o->method)) {
        if (data.add_op == NULL) return false;
        if (!data.add_ended) data.add_op->endTime = ++time;
        while (!data.others.empty()) {
          auto* op = data.others.front();
          if (!ongoings_op[op->id]) continue;
          ongoings_op[op->id] = false;
          op->endTime = ++time;
          data.others.pop_front();
        }
        data.remove_op->endTime = ++time;
        data.remove_ended = true;
      } else {
        if (!ongoings_op[o->id]) continue;
        if (data.add_op == NULL) return false;
        if (!data.add_ended) {
          data.add_op->endTime = ++time;
          data.add_ended = true;
        }
        ongoings_op[o->id] = false;
        o->endTime = ++time;
      }
    }
  }

  for (auto& [t, isInv, o] : events) t = isInv ? o->startTime : o->endTime;
  return true;
}

template <typename value_type, typename add_group>
bool tune_events_x(events_t<value_type>& events, const value_type& emptyVal,
                   const id_type& maxId) {
  std::sort(events.begin(), events.end());

  using oper_ptr = operation_t<value_type>*;
  struct value_event_data {
    oper_ptr add_op = nullptr;
    oper_ptr remove_op = nullptr;
    bool add_ended = false;
  };
  std::unordered_map<value_type, value_event_data> ongoings_val;

  time_type time = MIN_TIME;
  for (const auto& [_, isInv, o] : events) {
    const value_type& value = o->value;
    value_event_data& data = ongoings_val[value];

    if (value == emptyVal) {
      time_type& ts = isInv ? o->startTime : o->endTime;
      ts = ++time;
      continue;
    }

    if (isInv) {
      o->startTime = ++time;
      if (add_group::contains(o->method)) {
        data.add_op = o;
        if (data.remove_op) data.remove_op->startTime = ++time;
      } else {
        data.remove_op = o;
      }
    } else {
      if (add_group::contains(o->method)) {
        o->endTime = ++time;
        data.add_ended = true;
      } else {
        if (data.add_op == NULL) return false;
        if (!data.add_ended) data.add_op->endTime = ++time;
        data.remove_op->endTime = ++time;
      }
    }
  }

  for (auto& [t, isInv, o] : events) t = isInv ? o->startTime : o->endTime;
  return true;
}

// only works on tuned events
// empty operations can be of any method
// O(n log n)
template <typename value_type, typename add_group, typename remove_group>
bool verify_empty(events_t<value_type>& events, const value_type& emptyVal) {
  counting_sort(events);

  std::unordered_set<id_type> runningEmptyOp;
  std::unordered_set<value_type> critVal;
  int critValCnt = 0;

  for (const auto& [_, isInv, op] : events) {
    if (op->value != emptyVal) {
      if (isInv && remove_group::contains(op->method) &&
          !critVal.insert(op->value).second)
        --critValCnt;
      else if (!isInv && add_group::contains(op->method) &&
               critVal.insert(op->value).second)
        ++critValCnt;
    } else {
      if (isInv)
        runningEmptyOp.insert(op->id);
      else if (runningEmptyOp.count(op->id))
        return false;
    }

    if (!critValCnt) runningEmptyOp.clear();
  }

  return true;
}

template <typename value_type>
void remove_empty(history_t<value_type>& hist, events_t<value_type>& events,
                  const value_type& emptyVal) {
  hist.erase(std::remove_if(
                 hist.begin(), hist.end(),
                 [&emptyVal](const auto& o) { return o.value == emptyVal; }),
             hist.end());
  events = get_events(hist);  // pointers can be invalid
}

template <typename value_type>
void remove_empty(history_t<value_type>& hist, const value_type& emptyVal) {
  hist.erase(std::remove_if(
                 hist.begin(), hist.end(),
                 [&emptyVal](const auto& o) { return o.value == emptyVal; }),
             hist.end());
}

}  // namespace linp
