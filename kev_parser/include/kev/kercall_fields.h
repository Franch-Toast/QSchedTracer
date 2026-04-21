#pragma once

namespace kev {

struct KercallFieldDef {
    const char* const* fast;
    int fast_count;
    const char* const* wide;
    int wide_count;
};

const KercallFieldDef& get_kercall_enter_fields(int call_num);
const KercallFieldDef& get_kercall_exit_fields(int call_num);

} // namespace kev
