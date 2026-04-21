#include "kev/name_tracker.h"
#include <algorithm>

namespace kev {

const std::string NameTracker::empty_;

void NameTracker::set_process(int pid, const std::string& name, int parent_pid) {
    procs_[pid] = {name, parent_pid};
}

void NameTracker::set_thread(int pid, int tid, const std::string& name) {
    threads_[key(pid, tid)] = name;
}

const std::string& NameTracker::process_name(int pid) const {
    auto it = procs_.find(pid);
    return it != procs_.end() ? it->second.name : empty_;
}

const std::string& NameTracker::thread_name(int pid, int tid) const {
    auto it = threads_.find(key(pid, tid));
    return it != threads_.end() ? it->second : empty_;
}

std::vector<NameTracker::ProcessInfo> NameTracker::all_processes() const {
    std::vector<ProcessInfo> result;
    result.reserve(procs_.size());
    for (auto& [pid, entry] : procs_) {
        result.push_back({pid, entry.name, entry.parent_pid});
    }
    std::sort(result.begin(), result.end(),
              [](auto& a, auto& b) { return a.pid < b.pid; });
    return result;
}

std::vector<NameTracker::ThreadInfo> NameTracker::all_threads() const {
    std::vector<ThreadInfo> result;
    result.reserve(threads_.size());
    for (auto& [k, name] : threads_) {
        int pid = static_cast<int>(k >> 32);
        int tid = static_cast<int>(k & 0xFFFFFFFF);
        result.push_back({pid, tid, name});
    }
    std::sort(result.begin(), result.end(),
              [](auto& a, auto& b) {
                  return a.pid != b.pid ? a.pid < b.pid : a.tid < b.tid;
              });
    return result;
}

} // namespace kev
