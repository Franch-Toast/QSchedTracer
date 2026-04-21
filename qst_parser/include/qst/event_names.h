#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace qst {

constexpr int kKercallTableSize = 108;
constexpr int kCommCodeCount = 12;

struct CommField {
  const char* f1;
  const char* f2;
};

struct KercallFields {
  const char** fast;
  int fast_count;
  const char** wide;
  int wide_count;
};

struct SystemFieldLayout {
  const char** fast;
  int fast_count;
  const char** wide;
  int wide_count;
};

const char* get_kercall_name(int call_num, int os_version);
const char* get_comm_name(int comm_code);
const char* get_system_name(int sys_code);

CommField get_comm_field(int comm_code);
const char** get_comm_wide_fields(int comm_code, int* count);

const SystemFieldLayout* get_system_fields(int sys_code);

KercallFields get_kercall_enter_fields(int call_num, bool is_64);
KercallFields get_kercall_exit_fields(int call_num, bool is_64);

void format_kercall_args(std::unordered_map<std::string, std::string>& args, int call_num);
void format_general_args(std::unordered_map<std::string, std::string>& args);

}  // namespace qst
