#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace kev {

class NameTracker {
public:
    void set_process(int pid, const std::string& name, int parent_pid = 0);
    void set_thread(int pid, int tid, const std::string& name);

    const std::string& process_name(int pid) const;
    const std::string& thread_name(int pid, int tid) const;

    struct ProcessInfo { int pid; std::string name; int parent_pid; };
    struct ThreadInfo  { int pid; int tid; std::string name; };

    std::vector<ProcessInfo> all_processes() const;
    std::vector<ThreadInfo>  all_threads() const;

private:
    struct ProcEntry { std::string name; int parent_pid = 0; };
    std::unordered_map<int, ProcEntry> procs_;
    std::unordered_map<uint64_t, std::string> threads_;

    static const std::string empty_;
    static uint64_t key(int pid, int tid) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(pid)) << 32)
             | static_cast<uint32_t>(tid);
    }
};

} // namespace kev
