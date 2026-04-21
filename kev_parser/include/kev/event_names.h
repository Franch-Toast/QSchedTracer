#pragma once

namespace kev {
namespace event_names {

const char* thread_state_name(int state_index);
const char* kercall_name(int call_num);
const char* comm_name(int event_index);
const char* system_name(int event_index);

int bitmask_to_state_index(unsigned bitmask);
int event_to_state_index(unsigned event_value);

} // namespace event_names
} // namespace kev
