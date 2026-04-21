#include "qst/event_names.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace qst {

static const char* COMM_NAMES_DATA[kCommCodeCount] = {
  "SMSG",
  "SPULSE",
  "RMSG",
  "RPULSE",
  "SPULSE_EXE",
  "SPULSE_DIS",
  "SPULSE_DEA",
  "SPULSE_UN",
  "SPULSE_QUN",
  "SIGNAL",
  "REPLY",
  "ERROR",
};

static const char* SYSTEM_NAMES_DATA[32] = {
  nullptr,
  "SYS_RESERVED",
  "PATHMGR",
  "APS_NAME",
  "APS_BUDGETS",
  "APS_BANKRUPTCY",
  "MMAP",
  "MUNMAP",
  "MAPNAME",
  "ADDRESS",
  "FUNC_ENTER",
  "FUNC_EXIT",
  "SLOG",
  "DEFRAG_START",
  "RUNSTATE",
  "POWER",
  "IPI",
  "PAGEWAIT",
  "TIMER",
  "DEFRAG_END",
  "PROFILE",
  "MAPNAME_64",
  "APS_PSTATS",
  "APS_OSTATS",
  "APS_INFO",
  "APS_JOIN",
  "APS_THREAD",
  "APS_PROCESS",
  "SCHED_CONF",
  "IST_ATTACH",
  "IST_DETACH",
  "PCTRACE",
};

static const CommField COMM_FIELDS_DATA[kCommCodeCount] = {
  { "target_rcvid", "target_pid" },
  { "target_scoid", "target_pid" },
  { "target_rcvid", "target_pid" },
  { "target_scoid", "target_pid" },
  { "target_scoid", "target_pid" },
  { "target_scoid", "target_pid" },
  { "target_scoid", "target_pid" },
  { "target_scoid", "target_pid" },
  { "target_scoid", "target_pid" },
  { "si_signo", "si_code" },
  { "target_tid", "target_pid" },
  { "target_tid", "target_pid" },
};

static const char* COMM_WIDE_0[] = {
  "target_rcvid",
  "target_pid",
};
static const int COMM_WIDE_0_COUNT = 2;

static const char* COMM_WIDE_1[] = {
  "target_scoid",
  "target_pid",
};
static const int COMM_WIDE_1_COUNT = 2;

static const char* COMM_WIDE_2[] = {
  "target_rcvid",
  "target_pid",
};
static const int COMM_WIDE_2_COUNT = 2;

static const char* COMM_WIDE_3[] = {
  "target_scoid",
  "target_pid",
};
static const int COMM_WIDE_3_COUNT = 2;

static const char* COMM_WIDE_4[] = {
  "target_scoid",
  "target_pid",
};
static const int COMM_WIDE_4_COUNT = 2;

static const char* COMM_WIDE_5[] = {
  "target_scoid",
  "target_pid",
};
static const int COMM_WIDE_5_COUNT = 2;

static const char* COMM_WIDE_6[] = {
  "target_scoid",
  "target_pid",
};
static const int COMM_WIDE_6_COUNT = 2;

static const char* COMM_WIDE_7[] = {
  "target_scoid",
  "target_pid",
};
static const int COMM_WIDE_7_COUNT = 2;

static const char* COMM_WIDE_8[] = {
  "target_scoid",
  "target_pid",
};
static const int COMM_WIDE_8_COUNT = 2;

static const char* COMM_WIDE_9[] = {
  "si_signo",
  "si_code",
  "si_errno",
  "pad[0]",
  "pad[1]",
  "pad[2]",
  "pad[3]",
  "pad[4]",
  "pad[5]",
};
static const int COMM_WIDE_9_COUNT = 9;

static const char* COMM_WIDE_10[] = {
  "target_tid",
  "target_pid",
};
static const int COMM_WIDE_10_COUNT = 2;

static const char* COMM_WIDE_11[] = {
  "target_tid",
  "target_pid",
};
static const int COMM_WIDE_11_COUNT = 2;

static const char* SYS_FAST_0x02[] = {
  "pid",
  "tid",
};
static const int SYS_FAST_0x02_COUNT = 2;
static const char* SYS_WIDE_0x02[] = {
  "pid",
  "tid",
};
static const int SYS_WIDE_0x02_COUNT = 2;

static const char* SYS_FAST_0x03[] = {
  "partition_id",
};
static const int SYS_FAST_0x03_COUNT = 1;
static const char* SYS_WIDE_0x03[] = {
  "partition_id",
};
static const int SYS_WIDE_0x03_COUNT = 1;

static const char* SYS_FAST_0x04[] = {
  "partition_id",
  "cpu_budget_pct",
  "critical_budget_ms",
  "max_cpu_budget",
  "critical_priority",
  "budget_pct_scale",
};
static const int SYS_FAST_0x04_COUNT = 6;
static const char* SYS_WIDE_0x04[] = {
  "partition_id",
  "cpu_budget_pct",
  "critical_budget_ms",
  "max_cpu_budget",
  "critical_priority",
  "budget_pct_scale",
};
static const int SYS_WIDE_0x04_COUNT = 6;

static const char* SYS_FAST_0x05[] = {
  "suspect_pid",
  "suspect_tid",
  "partition_id",
};
static const int SYS_FAST_0x05_COUNT = 3;
static const char* SYS_WIDE_0x05[] = {
  "suspect_pid",
  "suspect_tid",
  "partition_id",
};
static const int SYS_WIDE_0x05_COUNT = 3;

static const char* SYS_FAST_0x06[] = {
  "pid",
  "addr_lo",
  "addr_hi",
  "len_lo",
  "len_hi",
  "flags",
};
static const int SYS_FAST_0x06_COUNT = 6;
static const char* SYS_WIDE_0x06[] = {
  "pid",
  "addr_lo",
  "addr_hi",
  "len_lo",
  "len_hi",
  "flags",
  "prot",
  "fd",
  "align_lo",
  "align_hi",
  "offset_lo",
  "offset_hi",
};
static const int SYS_WIDE_0x06_COUNT = 12;

static const char* SYS_FAST_0x07[] = {
  "pid",
  "addr_lo",
  "addr_hi",
  "len_lo",
  "len_hi",
};
static const int SYS_FAST_0x07_COUNT = 5;
static const char* SYS_WIDE_0x07[] = {
  "pid",
  "addr_lo",
  "addr_hi",
  "len_lo",
  "len_hi",
};
static const int SYS_WIDE_0x07_COUNT = 5;

static const char* SYS_FAST_0x08[] = {
  "pid",
  "addr",
  "len",
};
static const int SYS_FAST_0x08_COUNT = 3;
static const char* SYS_WIDE_0x08[] = {
  "pid",
  "addr",
  "len",
};
static const int SYS_WIDE_0x08_COUNT = 3;

static const char* SYS_FAST_0x09[] = {
  "addr",
  "empty",
};
static const int SYS_FAST_0x09_COUNT = 2;
static const char* SYS_WIDE_0x09[] = {
  "addr",
  "empty",
};
static const int SYS_WIDE_0x09_COUNT = 2;

static const char* SYS_FAST_0x0a[] = {
  "thisfn",
  "call_site",
};
static const int SYS_FAST_0x0a_COUNT = 2;
static const char* SYS_WIDE_0x0a[] = {
  "thisfn",
  "call_site",
};
static const int SYS_WIDE_0x0a_COUNT = 2;

static const char* SYS_FAST_0x0b[] = {
  "thisfn",
  "call_site",
};
static const int SYS_FAST_0x0b_COUNT = 2;
static const char* SYS_WIDE_0x0b[] = {
  "thisfn",
  "call_site",
};
static const int SYS_WIDE_0x0b_COUNT = 2;

static const char* SYS_FAST_0x0c[] = {
  "opcode",
  "severity",
};
static const int SYS_FAST_0x0c_COUNT = 2;
static const char* SYS_WIDE_0x0c[] = {
  "opcode",
  "severity",
};
static const int SYS_WIDE_0x0c_COUNT = 2;

static const char* SYS_FAST_0x0d[] = {
  "d1",
  "d2",
};
static const int SYS_FAST_0x0d_COUNT = 2;
static const char* SYS_WIDE_0x0d[] = {
  "d1",
  "d2",
};
static const int SYS_WIDE_0x0d_COUNT = 2;

static const char* SYS_FAST_0x0e[] = {
  "bitset",
};
static const int SYS_FAST_0x0e_COUNT = 1;
static const char* SYS_WIDE_0x0e[] = {
  "bitset",
};
static const int SYS_WIDE_0x0e_COUNT = 1;

static const char* SYS_FAST_0x0f[] = {
  "bitset",
  "mode",
};
static const int SYS_FAST_0x0f_COUNT = 2;
static const char* SYS_WIDE_0x0f[] = {
  "bitset",
  "mode",
};
static const int SYS_WIDE_0x0f_COUNT = 2;

static const char* SYS_FAST_0x10[] = {
  "ipicmd",
  "pad",
  "ip_lo",
  "ip_hi",
  "tid",
  "pid",
};
static const int SYS_FAST_0x10_COUNT = 6;
static const char* SYS_WIDE_0x10[] = {
  "ipicmd",
  "pad",
  "ip_lo",
  "ip_hi",
  "tid",
  "pid",
};
static const int SYS_WIDE_0x10_COUNT = 6;

static const char* SYS_FAST_0x11[] = {
  "pid",
  "tid",
  "ip",
  "vaddr",
};
static const int SYS_FAST_0x11_COUNT = 4;
static const char* SYS_WIDE_0x11[] = {
  "pid",
  "tid",
  "ip",
  "vaddr",
  "fault_type",
  "mmap_flags",
  "obj_offset_lo",
  "obj_offset_hi",
};
static const int SYS_WIDE_0x11_COUNT = 8;

static const char* SYS_FAST_0x12[] = {
  "pid",
  "tid",
  "timer_id",
  "flags",
};
static const int SYS_FAST_0x12_COUNT = 4;
static const char* SYS_WIDE_0x12[] = {
  "pid",
  "tid",
  "timer_id",
  "flags",
};
static const int SYS_WIDE_0x12_COUNT = 4;

static const char* SYS_FAST_0x13[] = {
  "rc",
  "freemem",
  "maxblock",
};
static const int SYS_FAST_0x13_COUNT = 3;
static const char* SYS_WIDE_0x13[] = {
  "rc",
  "freemem",
  "maxblock",
};
static const int SYS_WIDE_0x13_COUNT = 3;

static const char* SYS_FAST_0x14[] = {
  "ip",
  "tid",
  "pid",
};
static const int SYS_FAST_0x14_COUNT = 3;
static const char* SYS_WIDE_0x14[] = {
  "ip",
  "tid",
  "pid",
};
static const int SYS_WIDE_0x14_COUNT = 3;

static const char* SYS_FAST_0x15[] = {
  "pid",
  "addr_lo",
  "addr_hi",
  "len_lo",
  "len_hi",
};
static const int SYS_FAST_0x15_COUNT = 5;
static const char* SYS_WIDE_0x15[] = {
  "pid",
  "addr_lo",
  "addr_hi",
  "len_lo",
  "len_hi",
};
static const int SYS_WIDE_0x15_COUNT = 5;

static const char* SYS_FAST_0x16[] = {
  "partition_id",
  "flags",
};
static const int SYS_FAST_0x16_COUNT = 2;
static const char* SYS_WIDE_0x16[] = {
  "partition_id",
  "flags",
};
static const int SYS_WIDE_0x16_COUNT = 2;

static const char* SYS_FAST_0x17[] = {
  "bnkr_pid",
  "bnkr_tid",
  "bnkr_partition_id",
};
static const int SYS_FAST_0x17_COUNT = 3;
static const char* SYS_WIDE_0x17[] = {
  "bnkr_pid",
  "bnkr_tid",
  "bnkr_partition_id",
};
static const int SYS_WIDE_0x17_COUNT = 3;

static const char* SYS_FAST_0x18[] = {
  "window_size_ms",
  "sched_flags",
  "sec_flags",
  "bnkr_flags",
  "num_partitions",
  "max_partitions",
};
static const int SYS_FAST_0x18_COUNT = 6;
static const char* SYS_WIDE_0x18[] = {
  "window_size_ms",
  "sched_flags",
  "sec_flags",
  "bnkr_flags",
  "num_partitions",
  "max_partitions",
};
static const int SYS_WIDE_0x18_COUNT = 6;

static const char* SYS_FAST_0x19[] = {
  "partition_id",
  "pid",
  "tid",
  "aid",
};
static const int SYS_FAST_0x19_COUNT = 4;
static const char* SYS_WIDE_0x19[] = {
  "partition_id",
  "pid",
  "tid",
  "aid",
};
static const int SYS_WIDE_0x19_COUNT = 4;

static const char* SYS_FAST_0x1a[] = {
  "pid",
  "tid",
  "orig_partition",
  "inherited_partition",
  "flags",
};
static const int SYS_FAST_0x1a_COUNT = 5;
static const char* SYS_WIDE_0x1a[] = {
  "pid",
  "tid",
  "orig_partition",
  "inherited_partition",
  "flags",
};
static const int SYS_WIDE_0x1a_COUNT = 5;

static const char* SYS_FAST_0x1b[] = {
  "pid",
  "partition_id",
};
static const int SYS_FAST_0x1b_COUNT = 2;
static const char* SYS_WIDE_0x1b[] = {
  "pid",
  "partition_id",
};
static const int SYS_WIDE_0x1b_COUNT = 2;

static const char* SYS_FAST_0x1c[] = {
  "old_low_latency",
  "old_migrate",
  "new_low_latency",
  "new_migrate",
};
static const int SYS_FAST_0x1c_COUNT = 4;
static const char* SYS_WIDE_0x1c[] = {
  "old_low_latency",
  "old_migrate",
  "new_low_latency",
  "new_migrate",
};
static const int SYS_WIDE_0x1c_COUNT = 4;

static const char* SYS_FAST_0x1d[] = {
  "pid",
  "tid",
  "intr_id",
  "intr_cpu",
  "flags",
  "ist_prio",
};
static const int SYS_FAST_0x1d_COUNT = 6;
static const char* SYS_WIDE_0x1d[] = {
  "pid",
  "tid",
  "intr_id",
  "intr_cpu",
  "flags",
  "ist_prio",
};
static const int SYS_WIDE_0x1d_COUNT = 6;

static const char* SYS_FAST_0x1e[] = {
  "pid",
  "tid",
  "intr_id",
  "intr_cpu",
  "flags",
  "ist_prio",
};
static const int SYS_FAST_0x1e_COUNT = 6;
static const char* SYS_WIDE_0x1e[] = {
  "pid",
  "tid",
  "intr_id",
  "intr_cpu",
  "flags",
  "ist_prio",
};
static const int SYS_WIDE_0x1e_COUNT = 6;


static const SystemFieldLayout SYSTEM_FIELDS_LOOKUP[32] = {
  { nullptr, 0, nullptr, 0 },
  { nullptr, 0, nullptr, 0 },
  { SYS_FAST_0x02, SYS_FAST_0x02_COUNT, SYS_WIDE_0x02, SYS_WIDE_0x02_COUNT },
  { SYS_FAST_0x03, SYS_FAST_0x03_COUNT, SYS_WIDE_0x03, SYS_WIDE_0x03_COUNT },
  { SYS_FAST_0x04, SYS_FAST_0x04_COUNT, SYS_WIDE_0x04, SYS_WIDE_0x04_COUNT },
  { SYS_FAST_0x05, SYS_FAST_0x05_COUNT, SYS_WIDE_0x05, SYS_WIDE_0x05_COUNT },
  { SYS_FAST_0x06, SYS_FAST_0x06_COUNT, SYS_WIDE_0x06, SYS_WIDE_0x06_COUNT },
  { SYS_FAST_0x07, SYS_FAST_0x07_COUNT, SYS_WIDE_0x07, SYS_WIDE_0x07_COUNT },
  { SYS_FAST_0x08, SYS_FAST_0x08_COUNT, SYS_WIDE_0x08, SYS_WIDE_0x08_COUNT },
  { SYS_FAST_0x09, SYS_FAST_0x09_COUNT, SYS_WIDE_0x09, SYS_WIDE_0x09_COUNT },
  { SYS_FAST_0x0a, SYS_FAST_0x0a_COUNT, SYS_WIDE_0x0a, SYS_WIDE_0x0a_COUNT },
  { SYS_FAST_0x0b, SYS_FAST_0x0b_COUNT, SYS_WIDE_0x0b, SYS_WIDE_0x0b_COUNT },
  { SYS_FAST_0x0c, SYS_FAST_0x0c_COUNT, SYS_WIDE_0x0c, SYS_WIDE_0x0c_COUNT },
  { SYS_FAST_0x0d, SYS_FAST_0x0d_COUNT, SYS_WIDE_0x0d, SYS_WIDE_0x0d_COUNT },
  { SYS_FAST_0x0e, SYS_FAST_0x0e_COUNT, SYS_WIDE_0x0e, SYS_WIDE_0x0e_COUNT },
  { SYS_FAST_0x0f, SYS_FAST_0x0f_COUNT, SYS_WIDE_0x0f, SYS_WIDE_0x0f_COUNT },
  { SYS_FAST_0x10, SYS_FAST_0x10_COUNT, SYS_WIDE_0x10, SYS_WIDE_0x10_COUNT },
  { SYS_FAST_0x11, SYS_FAST_0x11_COUNT, SYS_WIDE_0x11, SYS_WIDE_0x11_COUNT },
  { SYS_FAST_0x12, SYS_FAST_0x12_COUNT, SYS_WIDE_0x12, SYS_WIDE_0x12_COUNT },
  { SYS_FAST_0x13, SYS_FAST_0x13_COUNT, SYS_WIDE_0x13, SYS_WIDE_0x13_COUNT },
  { SYS_FAST_0x14, SYS_FAST_0x14_COUNT, SYS_WIDE_0x14, SYS_WIDE_0x14_COUNT },
  { SYS_FAST_0x15, SYS_FAST_0x15_COUNT, SYS_WIDE_0x15, SYS_WIDE_0x15_COUNT },
  { SYS_FAST_0x16, SYS_FAST_0x16_COUNT, SYS_WIDE_0x16, SYS_WIDE_0x16_COUNT },
  { SYS_FAST_0x17, SYS_FAST_0x17_COUNT, SYS_WIDE_0x17, SYS_WIDE_0x17_COUNT },
  { SYS_FAST_0x18, SYS_FAST_0x18_COUNT, SYS_WIDE_0x18, SYS_WIDE_0x18_COUNT },
  { SYS_FAST_0x19, SYS_FAST_0x19_COUNT, SYS_WIDE_0x19, SYS_WIDE_0x19_COUNT },
  { SYS_FAST_0x1a, SYS_FAST_0x1a_COUNT, SYS_WIDE_0x1a, SYS_WIDE_0x1a_COUNT },
  { SYS_FAST_0x1b, SYS_FAST_0x1b_COUNT, SYS_WIDE_0x1b, SYS_WIDE_0x1b_COUNT },
  { SYS_FAST_0x1c, SYS_FAST_0x1c_COUNT, SYS_WIDE_0x1c, SYS_WIDE_0x1c_COUNT },
  { SYS_FAST_0x1d, SYS_FAST_0x1d_COUNT, SYS_WIDE_0x1d, SYS_WIDE_0x1d_COUNT },
  { SYS_FAST_0x1e, SYS_FAST_0x1e_COUNT, SYS_WIDE_0x1e, SYS_WIDE_0x1e_COUNT },
  { nullptr, 0, nullptr, 0 },
};

static const char** COMM_WIDE_PTR[kCommCodeCount] = {
  COMM_WIDE_0,
  COMM_WIDE_1,
  COMM_WIDE_2,
  COMM_WIDE_3,
  COMM_WIDE_4,
  COMM_WIDE_5,
  COMM_WIDE_6,
  COMM_WIDE_7,
  COMM_WIDE_8,
  COMM_WIDE_9,
  COMM_WIDE_10,
  COMM_WIDE_11,
};
static const int COMM_WIDE_COUNT[kCommCodeCount] = {
  COMM_WIDE_0_COUNT,
  COMM_WIDE_1_COUNT,
  COMM_WIDE_2_COUNT,
  COMM_WIDE_3_COUNT,
  COMM_WIDE_4_COUNT,
  COMM_WIDE_5_COUNT,
  COMM_WIDE_6_COUNT,
  COMM_WIDE_7_COUNT,
  COMM_WIDE_8_COUNT,
  COMM_WIDE_9_COUNT,
  COMM_WIDE_10_COUNT,
  COMM_WIDE_11_COUNT,
};

const char* get_comm_name(int comm_code) {
  if (comm_code < 0 || comm_code >= kCommCodeCount) return nullptr;
  return COMM_NAMES_DATA[comm_code];
}

const char* get_system_name(int sys_code) {
  if (sys_code < 0 || sys_code >= 32) return nullptr;
  return SYSTEM_NAMES_DATA[sys_code];
}

CommField get_comm_field(int comm_code) {
  if (comm_code < 0 || comm_code >= kCommCodeCount) return { "", "" };
  return COMM_FIELDS_DATA[comm_code];
}

const char** get_comm_wide_fields(int comm_code, int* count) {
  if (comm_code < 0 || comm_code >= kCommCodeCount) {
    if (count) *count = 0;
    return nullptr;
  }
  if (count) *count = COMM_WIDE_COUNT[comm_code];
  return COMM_WIDE_PTR[comm_code];
}

const SystemFieldLayout* get_system_fields(int sys_code) {
  if (sys_code < 0 || sys_code >= 32) return nullptr;
  const SystemFieldLayout* p = &SYSTEM_FIELDS_LOOKUP[sys_code];
  if (!p->fast || p->fast_count == 0) return nullptr;
  return p;
}

namespace {

bool parse_int64(const std::string& s, std::int64_t* out) {
  if (s.empty()) return false;
  char* end = nullptr;
  errno = 0;
  long long v = std::strtoll(s.c_str(), &end, 0);
  if (errno != 0 || end == s.c_str()) return false;
  if (*end != '\0') return false;
  *out = static_cast<std::int64_t>(v);
  return true;
}

void format_hex_int64(std::string* s) {
  std::int64_t v = 0;
  if (!parse_int64(*s, &v)) return;
  std::ostringstream oss;
  oss << "0x" << std::hex << static_cast<std::uint64_t>(v);
  *s = oss.str();
}

bool hex_field_exact(const std::string& key) {
  static const char* kExact[] = {
    "nd",
    "coid",
    "rcvid",
    "scoid",
    "target_rcvid",
    "target_scoid",
    "flags",
    "sa_flags",
    "sa_mask_0",
    "sa_mask_1",
    "sig_blocked_0",
    "sig_blocked_1",
    "sig_wait_0",
    "sig_wait_1",
    "masks",
    "bits",
    "new_attr",
    "code",
    "value",
    "status",
    "offset",
    "signal_p",
  };
  for (const char* e : kExact) {
    if (key == e) return true;
  }
  return false;
}

bool should_hex_field(std::string_view key_sv) {
  const std::string key(key_sv);
  if (hex_field_exact(key)) return true;
  if (key.size() >= 4 && key.compare(key.size() - 4, 4, "_ptr") == 0) return true;
  std::string kl;
  kl.reserve(key.size());
  for (unsigned char c : key) kl.push_back(static_cast<char>(std::tolower(c)));
  static const char* kPrefixes[] = {
    "sync_p",
    "mutex_p",
    "handler_p",
    "func_p",
    "data_p",
    "area_p",
    "addr",
    "sigstub_p",
    "signal_p",
    "status_p",
    "canstub_p",
    "stackaddr_p",
    "exitfunc_p",
    "eventp",
    "rmsg_p",
  };
  for (const char* p : kPrefixes) {
    const size_t plen = std::strlen(p);
    if (kl.size() >= plen && kl.compare(0, plen, p) == 0) return true;
  }
  if (kl.size() >= 4 && kl.compare(0, 4, "msg[") == 0) return true;
  if (kl.size() >= 5 && kl.compare(0, 5, "rmsg[") == 0) return true;
  if (kl.size() >= 5 && kl.compare(0, 5, "smsg[") == 0) return true;
  return false;
}

void combine_lo_hi(std::unordered_map<std::string, std::string>& args) {
  std::vector<std::string> lo_keys;
  for (const auto& kv : args) {
    const std::string& k = kv.first;
    if (k.size() > 3 && k.compare(k.size() - 3, 3, "_lo") == 0) lo_keys.push_back(k);
  }
  for (const std::string& lo_key : lo_keys) {
    const std::string hi_key = lo_key.substr(0, lo_key.size() - 3) + "_hi";
    auto it_lo = args.find(lo_key);
    auto it_hi = args.find(hi_key);
    if (it_lo == args.end() || it_hi == args.end()) continue;
    std::int64_t lo = 0, hi = 0;
    if (!parse_int64(it_lo->second, &lo) || !parse_int64(it_hi->second, &hi)) continue;
    std::string base = lo_key.substr(0, lo_key.size() - 3);
    if (base.size() >= 2 && base.compare(base.size() - 2, 2, "_p") == 0) {
      base = base.substr(0, base.size() - 2) + "_ptr";
    }
    const auto combined = (static_cast<std::uint64_t>(hi) << 32) |
                          (static_cast<std::uint64_t>(lo) & 0xFFFFFFFFu);
    args.erase(lo_key);
    args.erase(hi_key);
    args[base] = std::to_string(static_cast<std::int64_t>(combined));
  }
}

static const int kSyncKercallNums[] = { 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 77 };

bool is_sync_kercall(int call_num) {
  for (int n : kSyncKercallNums) {
    if (n == call_num) return true;
  }
  return false;
}

std::string format_sync_owner(std::uint32_t val) {
  switch (val) {
    case 0x00000000u: return "MUTEX_FREE";
    case 0xFFFFFFFFu: return "INITIALIZER";
    case 0xFFFFFFFEu: return "DESTROYED";
    case 0xFFFFFFFDu: return "NAMED_SEM";
    case 0xFFFFFFFCu: return "SEM";
    case 0xFFFFFFFBu: return "COND";
    case 0xFFFFFFFAu: return "SPIN";
    case 0xFFFFFFF9u: return "JOB";
    case 0xFFFFFFF8u: return "BARRIER";
    case 0xFFFFFFF7u: return "RWLOCK";
    case 0xFFFFFF00u: return "OWNERDEAD";
    default: break;
  }
  constexpr std::uint32_t kWaiting = 0x80000000u;
  constexpr std::uint32_t kOwnerMask = 0x7FFFFFFFu;
  const bool waiting = (val & kWaiting) != 0;
  const std::uint32_t vtid = val & kOwnerMask;
  std::ostringstream oss;
  if (waiting) oss << "WAITING|0x" << std::hex << vtid;
  else oss << "0x" << std::hex << static_cast<std::uint32_t>(val);
  return oss.str();
}

std::string format_sync_count(std::uint32_t val) {
  constexpr std::uint32_t kCountMask = 0x00FFFFFFu;
  const std::uint32_t count_n = val & kCountMask;
  struct { std::uint32_t mask; const char* name; } flags[] = {
    { 0x80000000u, "NONRECURSIVE" }, { 0x40000000u, "SHARED" }, { 0x20000000u, "PRIOCEILING" },
    { 0x10000000u, "PRIONONE" }, { 0x08000000u, "WAKEUP" }, { 0x04000000u, "KFORCE" },
    { 0x02000000u, "ROBUST" }, { 0x01000000u, "NOERRORCHECK" },
  };
  std::vector<const char*> parts;
  for (const auto& f : flags) {
    if (val & f.mask) parts.push_back(f.name);
  }
  if (parts.empty()) return std::to_string(count_n);
  std::ostringstream oss;
  oss << "0x" << std::hex << static_cast<std::uint32_t>(val) << " (count_n=" << std::dec << count_n << " ";
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i) oss << '|';
    oss << parts[i];
  }
  oss << ")";
  return oss.str();
}

}  // namespace

void format_kercall_args(std::unordered_map<std::string, std::string>& args, int call_num) {
  combine_lo_hi(args);
  const bool sync = is_sync_kercall(call_num);
  for (auto& kv : args) {
    std::int64_t iv = 0;
    if (!parse_int64(kv.second, &iv)) continue;
    const std::string& key = kv.first;
    if (sync && (key == "count" || key == "mutex_count" || key == "sync_count")) {
      kv.second = format_sync_count(static_cast<std::uint32_t>(iv));
      continue;
    }
    if (sync && (key == "owner" || key == "mutex_owner" || key == "sync_owner")) {
      kv.second = format_sync_owner(static_cast<std::uint32_t>(iv));
      continue;
    }
    if (should_hex_field(key)) format_hex_int64(&kv.second);
  }
  for (auto it = args.begin(); it != args.end(); ) {
    const std::string& k = it->first;
    if (k.size() >= 2 && k[0] == 'd' && std::all_of(k.begin() + 1, k.end(), [](char c) { return c >= '0' && c <= '9'; })) {
      std::int64_t v = 0;
      if (parse_int64(it->second, &v) && v == 0) { it = args.erase(it); continue; }
    }
    ++it;
  }
}

void format_general_args(std::unordered_map<std::string, std::string>& args) {
  combine_lo_hi(args);
  for (auto& kv : args) {
    if (!should_hex_field(kv.first)) continue;
    format_hex_int64(&kv.second);
  }
}


}  // namespace qst
