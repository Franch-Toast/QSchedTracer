#include "kev/kev_parser.h"
#include "kev/name_tracker.h"
#include "kev/output_writer.h"
#include "kev/stats.h"
#include "kev/filter.h"
#include "kev/event_names.h"
#include "kev/kercall_fields.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cinttypes>
#include <ctime>

namespace kev {

static bool should_format_hex(const char* name) {
    if (!name) return true;
    const char* n = name;
    size_t len = std::strlen(n);
    if (len >= 2 && n[len-2] == '_' && n[len-1] == 'p') return true;
    if (std::strstr(n, "addr") || std::strstr(n, "ptr"))  return true;
    if (std::strstr(n, "flags") || std::strstr(n, "mask")) return true;
    if (std::strstr(n, "msg[") || std::strstr(n, "rmsg[")) return true;
    if (std::strstr(n, "offset") || std::strstr(n, "prot")) return true;
    if (std::strstr(n, "bitset") || std::strstr(n, "bits")) return true;
    if (std::strstr(n, "cmd") || std::strstr(n, "ip_"))    return true;
    if (std::strstr(n, "clock_id") || std::strstr(n, "clockid")) return true;
    if (std::strstr(n, "sig_blocked") || std::strstr(n, "sig_wait")) return true;
    if (std::strstr(n, "pad"))  return true;
    if (n[0] == 'd' && n[1] >= '0' && n[1] <= '9')        return true;
    return false;
}

static std::string format_val(const char* name, unsigned val) {
    char buf[24];
    if (should_format_hex(name))
        std::snprintf(buf, sizeof(buf), "0x%08x", val);
    else
        std::snprintf(buf, sizeof(buf), "%u", val);
    return buf;
}

static void format_wall_time_buf(char* buf, size_t sz, int64_t epoch_ns) {
    time_t sec = static_cast<time_t>(epoch_ns / 1000000000LL);
    int usec = static_cast<int>((epoch_ns % 1000000000LL) / 1000);
    struct tm tm;
    gmtime_r(&sec, &tm);
    std::snprintf(buf, sz, "%04d-%02d-%02dT%02d:%02d:%02d.%06d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec, usec);
}

static std::string format_wall_time(int64_t epoch_ns) {
    char buf[40];
    format_wall_time_buf(buf, sizeof(buf), epoch_ns);
    return buf;
}

KevParser::KevParser(ParseConfig config) : config_(std::move(config)) {
    names_  = std::make_unique<NameTracker>();
    stats_  = std::make_unique<StatsSummary>();
    filter_ = std::make_unique<EventFilter>();

    if (config_.time_start_ns >= 0) filter_->time_start_ns = config_.time_start_ns;
    if (config_.time_end_ns >= 0)   filter_->time_end_ns   = config_.time_end_ns;
    for (int p : config_.filter_pids) filter_->pids.insert(p);
    for (int t : config_.filter_tids) filter_->tids.insert(t);
    for (auto& c : config_.filter_classes) filter_->classes.insert(c);

    std::memset(cpu_ctx_, 0, sizeof(cpu_ctx_));
}

KevParser::~KevParser() {
    if (tps_) traceparser_destroy(&tps_);
}

void KevParser::set_time_filter(int64_t start_ns, int64_t end_ns) {
    config_.time_start_ns = start_ns;
    config_.time_end_ns   = end_ns;
    if (start_ns >= 0) filter_->time_start_ns = start_ns;
    if (end_ns >= 0)   filter_->time_end_ns   = end_ns;
}

bool KevParser::check_ring_gap(int64_t ts) {
    if (ts <= 0) return true;

    if (first_event_ns_ == 0) first_event_ns_ = ts;
    if (ts > last_event_ts_) last_event_ts_ = ts;
    if (ts < first_event_ns_) first_event_ns_ = ts;

    if (prescan_mode_) {
        if (prescan_prev_ts_ > 0 && ts > prescan_prev_ts_) {
            int64_t gap = ts - prescan_prev_ts_;
            if (gap > prescan_max_gap_) {
                prescan_max_gap_ = gap;
                prescan_gap_before_ts_ = prescan_prev_ts_;
            }
        }
        prescan_prev_ts_ = ts;
        return true;
    }

    if (raw_ts_ && raw_ts_->gap_detected()) {
        if (raw_ts_->is_post_collection(current_event_d0_)) {
            ++post_collection_count_;
            return false;
        }
        return true;
    }

    if (prescan_gap_boundary_ > 0) {
        if (ts > prescan_gap_boundary_) {
            ++post_collection_count_;
            return false;
        }
        return true;
    }

    return true;
}

PrescanResult KevParser::prescan() {
    prescan_mode_ = true;
    last_event_ts_ = 0;
    first_event_ns_ = 0;
    post_collection_count_ = 0;
    prescan_prev_ts_ = 0;
    prescan_max_gap_ = 0;
    prescan_gap_before_ts_ = 0;

    tps_ = traceparser_init(nullptr);
    if (!tps_) return {};

    traceparser_debug(tps_, stderr, _TRACEPARSER_DEBUG_NONE);
    register_callbacks();
    traceparser(tps_, this, config_.input_path.c_str());

    PrescanResult r;
    r.first_event_ns = first_event_ns_;
    r.clk_failed     = clk_fallback_;
    r.cycles_per_sec = clock_freq_;

    if (prescan_max_gap_ > GAP_THRESHOLD_NS) {
        r.gap_detected = true;
        r.real_end_ns = prescan_gap_before_ts_;
        prescan_gap_boundary_ = prescan_gap_before_ts_;
    } else {
        r.gap_detected = false;
        r.real_end_ns = last_event_ts_;
        prescan_gap_boundary_ = 0;
    }

    if (clk_fallback_ && config_.wall_end_ns > 0 && clock_freq_ > 0) {
        raw_ts_ = std::make_unique<RawKevTimestamps>();
        RawKevTimestamps::Config rc;
        rc.cycles_per_sec = clock_freq_;
        rc.wall_end_ns    = config_.wall_end_ns;
        rc.verbose        = config_.verbose;
        auto sr = raw_ts_->build(config_.input_path.c_str(), rc);
        if (sr.success) {
            r.first_event_ns       = sr.first_event_ns;
            r.real_end_ns          = sr.real_end_ns;
            r.gap_detected         = sr.gap_detected;
            r.post_collection_count = sr.post_collection_count;
            prescan_gap_boundary_  = sr.gap_detected
                ? sr.real_end_ns : 0;
            if (config_.verbose)
                std::fprintf(stderr, "raw_ts: built %zu timestamps, "
                             "real_end=%" PRId64 " gap=%s\n",
                             sr.total_logical_events, sr.real_end_ns,
                             sr.gap_detected ? "yes" : "no");
        } else {
            raw_ts_.reset();
            if (config_.verbose)
                std::fprintf(stderr, "raw_ts: build failed, "
                             "falling back to TIME_DELTA\n");
        }
    }

    traceparser_destroy(&tps_);
    tps_ = nullptr;

    cached_prescan_ = r;
    prescan_done_ = true;

    prescan_mode_ = false;
    last_event_ts_ = 0;
    first_event_ns_ = 0;
    post_collection_count_ = 0;
    clock_freq_ = 0;
    clk_fallback_ = false;
    prescan_prev_ts_ = 0;
    prescan_max_gap_ = 0;
    prescan_gap_before_ts_ = 0;

    return r;
}

bool KevParser::parse() {
    if (config_.wall_end_ns > 0 && !prescan_done_) {
        prescan();
    }

    if (prescan_done_) {
        prescan_real_end_ns_ = cached_prescan_.real_end_ns;
        if (config_.verbose) {
            std::fprintf(stderr, "prescan: first_ns=%" PRId64 " real_end_ns=%" PRId64
                         " gap=%s post_events=%" PRIu64 "\n",
                         cached_prescan_.first_event_ns, cached_prescan_.real_end_ns,
                         cached_prescan_.gap_detected ? "yes" : "no",
                         cached_prescan_.post_collection_count);
        }
    }

    tps_ = traceparser_init(nullptr);
    if (!tps_) {
        std::fprintf(stderr, "kev_parser: traceparser_init() failed\n");
        return false;
    }

    traceparser_debug(tps_, stderr, _TRACEPARSER_DEBUG_NONE);

    register_callbacks();

    FILE* out = stdout;
    bool close_out = false;
    if (!config_.output_path.empty() && config_.output_path != "-") {
        out = std::fopen(config_.output_path.c_str(), "w");
        if (!out) {
            std::fprintf(stderr, "kev_parser: cannot open '%s'\n",
                         config_.output_path.c_str());
            return false;
        }
        close_out = true;
    }

    writer_ = create_writer(config_.format, out);

    int rc = traceparser(tps_, this, config_.input_path.c_str());

    if (!meta_written_) {
        Metadata meta = extract_metadata();
        clock_freq_ = meta.cycles_per_sec;
        writer_->write_metadata(meta);
        meta_written_ = true;
        for (auto& pn : pending_names_) {
            if (pn.kind == PendingName::PROC)
                writer_->write_process(pn.pid, pn.name, pn.tid_or_parent);
            else
                writer_->write_thread_name(pn.pid, pn.tid_or_parent, pn.name);
        }
        pending_names_.clear();
    }

    for (auto& [pid, ps] : stats_->per_process) {
        const auto& n = names_->process_name(pid);
        if (!n.empty()) ps.name = n;
    }

    RingBufferInfo ring;
    ring.gap_detected = (prescan_gap_boundary_ > 0);
    ring.real_end_trace_ns = (prescan_gap_boundary_ > 0)
        ? prescan_real_end_ns_ : last_event_ts_;
    ring.post_collection_count = post_collection_count_;

    if (config_.wall_end_ns > 0 && ring.real_end_trace_ns > 0) {
        ring.wall_offset_ns = config_.wall_end_ns - ring.real_end_trace_ns;
        ring.has_wall_clock = true;
    }
    if (config_.wall_start_ns > 0) {
        ring.capture_wallclock_start = format_wall_time(config_.wall_start_ns);
    }
    if (config_.wall_end_ns > 0) {
        ring.capture_wallclock_end = format_wall_time(config_.wall_end_ns);
    }

    stats_->ring = ring;
    stats_->first_time_ns = first_event_ns_;
    if (prescan_gap_boundary_ > 0 && prescan_real_end_ns_ > 0) {
        stats_->last_time_ns = prescan_real_end_ns_;
    } else if (last_event_ts_ > stats_->last_time_ns || stats_->last_time_ns == 0) {
        stats_->last_time_ns = last_event_ts_;
    }

    writer_->write_summary(*stats_, build_process_summary_json());
    writer_->flush();

    if (close_out) std::fclose(out);

    if (rc != 0) {
        unsigned elen = 0;
        auto err = traceparser_get_info(tps_, _TRACEPARSER_INFO_ERROR, &elen);
        if (err) {
            std::fprintf(stderr, "kev_parser: error code=%u\n",
                         *static_cast<const unsigned*>(err));
        }
    }

    if (config_.verbose) {
        std::fprintf(stderr, "\n--- Parse Stats ---\n");
        std::fprintf(stderr, "  Total events:   %" PRIu64 "\n", stats_->total_events);
        std::fprintf(stderr, "  Thread:         %" PRIu64 "\n", stats_->counts.thread);
        std::fprintf(stderr, "  KerCall enter:  %" PRIu64 "\n", stats_->counts.kercall_enter);
        std::fprintf(stderr, "  KerCall exit:   %" PRIu64 "\n", stats_->counts.kercall_exit);
        std::fprintf(stderr, "  Comm:           %" PRIu64 "\n", stats_->counts.comm);
        std::fprintf(stderr, "  Process:        %" PRIu64 "\n", stats_->counts.process);
        std::fprintf(stderr, "  System:         %" PRIu64 "\n", stats_->counts.system);
        std::fprintf(stderr, "  Control:        %" PRIu64 "\n", stats_->counts.control);
        std::fprintf(stderr, "  Interrupt:      %" PRIu64 "\n", stats_->counts.interrupt);
    }

    return rc == 0;
}

void KevParser::register_callbacks() {
    const unsigned th_events[] = {
        _NTO_TRACE_THDEAD,    _NTO_TRACE_THRUNNING,     _NTO_TRACE_THREADY,
        _NTO_TRACE_THSTOPPED, _NTO_TRACE_THSEND,        _NTO_TRACE_THRECEIVE,
        _NTO_TRACE_THREPLY,   _NTO_TRACE_THSTACK,       _NTO_TRACE_THWAITTHREAD,
        _NTO_TRACE_THWAITPAGE,_NTO_TRACE_THSIGSUSPEND,   _NTO_TRACE_THSIGWAITINFO,
        _NTO_TRACE_THNANOSLEEP, _NTO_TRACE_THMUTEX,     _NTO_TRACE_THCONDVAR,
        _NTO_TRACE_THJOIN,    _NTO_TRACE_THINTR,        _NTO_TRACE_THSEM,
        _NTO_TRACE_THWAITCTX, _NTO_TRACE_THNET_SEND,    _NTO_TRACE_THNET_REPLY,
        _NTO_TRACE_THCREATE,  _NTO_TRACE_THDESTROY,
#if KEV_QNX_VERSION >= 80
        _NTO_TRACE_THMUON_MUTEX, _NTO_TRACE_THTRACEBUFFER,
        _NTO_TRACE_THINTR_ATTACH_EV, _NTO_TRACE_THTIMER_DELEGATE,
        _NTO_TRACE_THBARRIER,
#endif
    };
    for (auto ev : th_events) {
        traceparser_cs(tps_, this, cb_thread, _NTO_TRACE_THREAD, ev);
    }

    // CONTROL
    traceparser_cs(tps_, this, cb_control, _NTO_TRACE_CONTROL, _NTO_TRACE_CONTROLTIME);
    traceparser_cs(tps_, this, cb_control, _NTO_TRACE_CONTROL, _NTO_TRACE_CONTROLBUFFER);

    // KERCALL ENTER/EXIT: 32-bit (0-127) + 64-bit (512-639)
    traceparser_cs_range(tps_, this, cb_kercall_enter,
                         _NTO_TRACE_KERCALLENTER,
                         _NTO_TRACE_KERCALLFIRST, _NTO_TRACE_KERCALLLAST);
    traceparser_cs_range(tps_, this, cb_kercall_enter,
                         _NTO_TRACE_KERCALLENTER,
                         _NTO_TRACE_KERCALL64,
                         _NTO_TRACE_KERCALL64 + _NTO_TRACE_KERCALLLAST);
    traceparser_cs_range(tps_, this, cb_kercall_exit,
                         _NTO_TRACE_KERCALLEXIT,
                         _NTO_TRACE_KERCALLFIRST, _NTO_TRACE_KERCALLLAST);
    traceparser_cs_range(tps_, this, cb_kercall_exit,
                         _NTO_TRACE_KERCALLEXIT,
                         _NTO_TRACE_KERCALL64,
                         _NTO_TRACE_KERCALL64 + _NTO_TRACE_KERCALLLAST);

    // COMM
    traceparser_cs_range(tps_, this, cb_comm,
                         _NTO_TRACE_COMM,
                         _NTO_TRACE_COMM_SMSG, _NTO_TRACE_COMM_LAST);

    // SYSTEM
    traceparser_cs_range(tps_, this, cb_system,
                         _NTO_TRACE_SYSTEM,
                         _NTO_TRACE_SYS_RESERVED, _NTO_TRACE_SYS_LAST);

    // INTERRUPT
    traceparser_cs_range(tps_, this, cb_interrupt,
                         _NTO_TRACE_INT,
                         _NTO_TRACE_INTFIRST, 256);

    // PROCESS
    traceparser_cs(tps_, this, cb_proc_create,      _NTO_TRACE_PROCESS, _NTO_TRACE_PROCCREATE);
    traceparser_cs(tps_, this, cb_proc_destroy,      _NTO_TRACE_PROCESS, _NTO_TRACE_PROCDESTROY);
    traceparser_cs(tps_, this, cb_proc_create_name,  _NTO_TRACE_PROCESS, _NTO_TRACE_PROCCREATE_NAME);
    traceparser_cs(tps_, this, cb_proc_thread_name,  _NTO_TRACE_PROCESS, _NTO_TRACE_PROCTHREAD_NAME);

}

Metadata KevParser::extract_metadata() {
    Metadata m;
    unsigned len = 0;
    auto get_str = [&](info_modes_t mode) -> std::string {
        auto p = traceparser_get_info(tps_, mode, &len);
        return p ? std::string(static_cast<const char*>(p), len) : std::string();
    };

    m.qnx_version      = get_str(_TRACEPARSER_INFO_SYS_RELEASE);
    m.machine           = get_str(_TRACEPARSER_INFO_MACHINE);
    m.trace_date        = get_str(_TRACEPARSER_INFO_DATE);
    m.boot_date         = get_str(_TRACEPARSER_INFO_BOOT_DATE);
    m.trace_file        = get_str(_TRACEPARSER_INFO_FILE_NAME);

    auto cpu_s = get_str(_TRACEPARSER_INFO_CPU_NUM);
    if (!cpu_s.empty()) m.cpu_count = static_cast<unsigned>(std::strtoul(cpu_s.c_str(), nullptr, 10));

    auto cyc_s = get_str(_TRACEPARSER_INFO_CYCLES_PER_SEC);
    if (!cyc_s.empty()) m.cycles_per_sec = std::strtoull(cyc_s.c_str(), nullptr, 10);

    // TODO: tracelogger args not reliably exposed via traceparser_get_info()

    return m;
}

int64_t KevParser::get_timestamp_ns() {
    if (raw_ts_) {
        return raw_ts_->lookup(current_event_d0_);
    }

    if (clock_freq_ == 0) {
        unsigned clen = 0;
        auto cyc_s = static_cast<const char*>(
            traceparser_get_info(tps_, _TRACEPARSER_INFO_CYCLES_PER_SEC, &clen));
        if (cyc_s && clen > 0) clock_freq_ = std::strtoull(cyc_s, nullptr, 10);
        if (clock_freq_ == 0) return 0;
    }

    uint64_t cycles = 0;
    unsigned len = 0;

    if (!clk_fallback_) {
        auto* clk = static_cast<const uint64_t*>(
            traceparser_get_info(tps_, _TRACEPARSER_INFO_CLK, &len));
        if (clk && *clk != UINT64_MAX) {
            cycles = *clk;
        } else {
            clk_fallback_ = true;
        }
    }

    if (clk_fallback_) {
        auto* td = static_cast<const char*>(
            traceparser_get_info(tps_, _TRACEPARSER_INFO_TIME_DELTA, &len));
        if (!td || len == 0) return 0;
        int64_t sec = 0, ms = 0, us = 0;
        std::sscanf(td, "%" PRId64 ".%" PRId64 ".%" PRId64, &sec, &ms, &us);
        return sec * 1000000000LL + ms * 1000000LL + us * 1000LL;
    }

    int64_t sec = static_cast<int64_t>(cycles / clock_freq_);
    uint64_t rem = cycles % clock_freq_;
    return sec * 1000000000LL + static_cast<int64_t>(rem * 1000000000ULL / clock_freq_);
}

bool KevParser::should_write(int64_t ts, int pid, int tid, const char* cls) const {
    if (config_.mode == "summary") return false;
    if (!filter_->passes(ts, pid, tid)) return false;
    if (!filter_->passes_class(cls)) return false;
    return true;
}

void KevParser::write_event(int cpu, const char* cls, const char* evt,
                            int pid, int tid, int64_t ts, KVList data) {
    OutputWriter::Event ev;
    ev.seq = ++seq_;
    ev.time_ns = ts;
    ev.cpu = cpu;
    ev.event_class = cls;
    ev.event_name = evt;
    ev.pid = pid;
    ev.tid = tid;
    ev.process_name = &names_->process_name(pid);
    ev.thread_name  = &names_->thread_name(pid, tid);
    ev.data = std::move(data);

    if (config_.wall_end_ns > 0 && prescan_real_end_ns_ > 0) {
        int64_t wall_offset = config_.wall_end_ns - prescan_real_end_ns_;
        format_wall_time_buf(ev.wall_time, sizeof(ev.wall_time),
                             ts + wall_offset);
    }

    writer_->write_event(ev);
}

std::string KevParser::build_process_summary_json() {
    struct ProcEntry { int pid; std::string name; size_t thr; uint64_t cnt; };
    std::vector<ProcEntry> entries;
    entries.reserve(stats_->per_process.size());
    for (auto& [pid, ps] : stats_->per_process) {
        entries.push_back({pid, ps.name, ps.tids.size(), ps.event_count});
    }
    std::sort(entries.begin(), entries.end(),
              [](auto& a, auto& b) { return a.cnt > b.cnt; });

    std::string json = "[";
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) json += ",";
        char tmp[256];
        auto ename = escape_json(entries[i].name);
        std::snprintf(tmp, sizeof(tmp),
            "{\"pid\":%d,\"name\":\"%s\",\"thread_count\":%zu,\"event_count\":%" PRIu64 "}",
            entries[i].pid, ename.c_str(),
            entries[i].thr, entries[i].cnt);
        json += tmp;
    }
    json += "]";
    return json;
}

// ============================================================================
// Static C callbacks → instance method dispatch
// ============================================================================

int KevParser::cb_thread(struct traceparser_state*, void* ud,
                         unsigned h, unsigned time_off, unsigned* buf, unsigned len) {
    auto* self = static_cast<KevParser*>(ud);
    self->current_event_d0_ = time_off;
    self->on_thread(h, buf, len);
    return 0;
}
int KevParser::cb_control(struct traceparser_state*, void* ud,
                          unsigned h, unsigned time_off, unsigned* buf, unsigned len) {
    auto* self = static_cast<KevParser*>(ud);
    self->current_event_d0_ = time_off;
    self->on_control(h, buf, len);
    return 0;
}
int KevParser::cb_kercall_enter(struct traceparser_state*, void* ud,
                                unsigned h, unsigned time_off, unsigned* buf, unsigned len) {
    auto* self = static_cast<KevParser*>(ud);
    self->current_event_d0_ = time_off;
    self->on_kercall(h, buf, len, true);
    return 0;
}
int KevParser::cb_kercall_exit(struct traceparser_state*, void* ud,
                               unsigned h, unsigned time_off, unsigned* buf, unsigned len) {
    auto* self = static_cast<KevParser*>(ud);
    self->current_event_d0_ = time_off;
    self->on_kercall(h, buf, len, false);
    return 0;
}
int KevParser::cb_comm(struct traceparser_state*, void* ud,
                       unsigned h, unsigned time_off, unsigned* buf, unsigned len) {
    auto* self = static_cast<KevParser*>(ud);
    self->current_event_d0_ = time_off;
    self->on_comm(h, buf, len);
    return 0;
}
int KevParser::cb_proc_create(struct traceparser_state*, void* ud,
                              unsigned h, unsigned time_off, unsigned* buf, unsigned len) {
    auto* self = static_cast<KevParser*>(ud);
    self->current_event_d0_ = time_off;
    self->on_proc_create(h, buf, len);
    return 0;
}
int KevParser::cb_proc_create_name(struct traceparser_state*, void* ud,
                                   unsigned h, unsigned time_off, unsigned* buf, unsigned len) {
    auto* self = static_cast<KevParser*>(ud);
    self->current_event_d0_ = time_off;
    self->on_proc_create_name(h, buf, len);
    return 0;
}
int KevParser::cb_proc_thread_name(struct traceparser_state*, void* ud,
                                   unsigned h, unsigned time_off, unsigned* buf, unsigned len) {
    auto* self = static_cast<KevParser*>(ud);
    self->current_event_d0_ = time_off;
    self->on_proc_thread_name(h, buf, len);
    return 0;
}
int KevParser::cb_proc_destroy(struct traceparser_state*, void* ud,
                               unsigned h, unsigned time_off, unsigned* buf, unsigned len) {
    auto* self = static_cast<KevParser*>(ud);
    self->current_event_d0_ = time_off;
    self->on_proc_destroy(h, buf, len);
    return 0;
}
int KevParser::cb_system(struct traceparser_state*, void* ud,
                         unsigned h, unsigned time_off, unsigned* buf, unsigned len) {
    auto* self = static_cast<KevParser*>(ud);
    self->current_event_d0_ = time_off;
    self->on_system(h, buf, len);
    return 0;
}
int KevParser::cb_interrupt(struct traceparser_state*, void* ud,
                            unsigned h, unsigned time_off, unsigned* buf, unsigned len) {
    auto* self = static_cast<KevParser*>(ud);
    self->current_event_d0_ = time_off;
    self->on_interrupt(h, buf, len);
    return 0;
}
// ============================================================================
// Event handlers
// ============================================================================

void KevParser::on_thread(unsigned header, unsigned* buf, unsigned len) {
    int cpu = _NTO_TRACE_GETCPU(header);
    current_event_cpu_ = cpu;

    unsigned cb_len = 0;
    auto* cb_ev = static_cast<const unsigned*>(
        traceparser_get_info(tps_, _TRACEPARSER_INFO_NOW_CALLBACK_EVENT, &cb_len));
    int state_index = cb_ev ? event_names::event_to_state_index(*cb_ev) : -1;
    const char* state_name = event_names::thread_state_name(state_index);

    int pid = (len >= 1) ? static_cast<int>(buf[0]) : 0;
    int tid = (len >= 2) ? static_cast<int>(buf[1]) : 0;
    int64_t ts = get_timestamp_ns();

    if (state_index == 1 && cpu < 64) {
        cpu_ctx_[cpu] = {pid, tid};
    }

    if (!check_ring_gap(ts)) return;
    if (prescan_mode_) return;

    stats_->record(EventClass::THREAD, pid, tid, state_index);
    stats_->update_time(ts);

    if (!meta_written_) {
        Metadata meta = extract_metadata();
        clock_freq_ = meta.cycles_per_sec;
        writer_->write_metadata(meta);
        meta_written_ = true;
        for (auto& pn : pending_names_) {
            if (pn.kind == PendingName::PROC)
                writer_->write_process(pn.pid, pn.name, pn.tid_or_parent);
            else
                writer_->write_thread_name(pn.pid, pn.tid_or_parent, pn.name);
        }
        pending_names_.clear();
    }

    if (should_write(ts, pid, tid, "THREAD")) {
        KVList data;
        if (len >= 4) {
            data.push_back({"priority", std::to_string(buf[2])});
            data.push_back({"policy", std::to_string(buf[3])});
        }
        if (len >= 6) {
            data.push_back({"partition_id", std::to_string(buf[4])});
            data.push_back({"sched_flags", format_val("sched_flags", buf[5])});
        }
        write_event(cpu, "THREAD", state_name, pid, tid, ts, std::move(data));
    }
}

void KevParser::on_control(unsigned header, unsigned* buf, unsigned len) {
    (void)header; (void)buf; (void)len;
    if (prescan_mode_) return;
    stats_->record(EventClass::CONTROL, 0, 0);
}

void KevParser::on_kercall(unsigned header, unsigned* buf, unsigned len, bool is_enter) {
    int cpu = _NTO_TRACE_GETCPU(header);
    current_event_cpu_ = cpu;
    int event = _NTO_TRACE_GETEVENT(header);
    int call_num = event & 0x7F;
    const char* call_name = event_names::kercall_name(call_num);
    const char* cls = is_enter ? "KERCALL_ENTER" : "KERCALL_EXIT";

    int pid = 0, tid = 0;
    if (cpu < 64) {
        pid = cpu_ctx_[cpu].pid;
        tid = cpu_ctx_[cpu].tid;
    }

    int64_t ts = get_timestamp_ns();
    if (!check_ring_gap(ts)) return;
    if (prescan_mode_) return;

    auto ec = is_enter ? EventClass::KERCALL_ENTER : EventClass::KERCALL_EXIT;
    stats_->record(ec, pid, tid, -1, call_num);
    stats_->update_time(ts);

    if (should_write(ts, pid, tid, cls)) {
        bool is_64 = (event & _NTO_TRACE_KERCALL64) != 0;

        const auto& fields = is_enter
            ? get_kercall_enter_fields(call_num)
            : get_kercall_exit_fields(call_num);
        bool use_wide = (len > 2) && (fields.wide_count > 2);
        const char* const* fnames = use_wide ? fields.wide : fields.fast;
        int fcount = use_wide ? fields.wide_count : fields.fast_count;

        KVList data;
        if (is_64) data.push_back({"mode", "64bit"});
        for (unsigned i = 0; i < len && i < 8; ++i) {
            const char* key = (static_cast<int>(i) < fcount) ? fnames[i] : nullptr;
            if (key && std::strcmp(key, "empty") == 0) continue;
            std::string kname = key ? key : ("d" + std::to_string(i));
            data.push_back({kname, format_val(key, buf[i])});
        }

        write_event(cpu, cls, call_name, pid, tid, ts, std::move(data));
    }
}

void KevParser::on_comm(unsigned header, unsigned* buf, unsigned len) {
    int cpu = _NTO_TRACE_GETCPU(header);
    current_event_cpu_ = cpu;
    int event = _NTO_TRACE_GETEVENT(header);
    const char* name = event_names::comm_name(event);

    int pid = 0, tid = 0;
    if (cpu < 64) {
        pid = cpu_ctx_[cpu].pid;
        tid = cpu_ctx_[cpu].tid;
    }

    int64_t ts = get_timestamp_ns();
    if (!check_ring_gap(ts)) return;
    if (prescan_mode_) return;

    stats_->record(EventClass::COMM, pid, tid);
    stats_->update_time(ts);

    if (should_write(ts, pid, tid, "COMM")) {
        struct CommFieldDef { const char* const* names; int fast_count; int wide_count; };
        static const char* const F_SND_MSG[]     = {"target_rcvid", "target_pid"};
        static const char* const F_RCV_MSG[]     = {"target_scoid", "target_pid"};
        static const char* const F_REPLY_MSG[]   = {"target_rcvid", "target_pid"};
        static const char* const F_PULSE_SC[]    = {"target_scoid", "target_pid"};
        static const char* const F_SIGNAL[]      = {"si_signo", "si_code", "si_errno",
                                                     "pad[0]","pad[1]","pad[2]","pad[3]","pad[4]","pad[5]"};
        static const char* const F_RCVSIG[]      = {"target_tid", "target_pid"};
        static const char* const F_ERROR[]       = {"target_tid", "target_pid"};

        static const CommFieldDef COMM_DEFS[] = {
            {F_SND_MSG,   2, 2}, {F_RCV_MSG,   2, 2}, {F_REPLY_MSG, 2, 2},
            {F_PULSE_SC,  2, 2}, {F_PULSE_SC,  2, 2}, {F_PULSE_SC,  2, 2},
            {F_PULSE_SC,  2, 2}, {F_PULSE_SC,  2, 2}, {F_PULSE_SC,  2, 2},
            {F_SIGNAL,    2, 9}, {F_RCVSIG,    2, 2}, {F_ERROR,     2, 2},
        };
        static constexpr int NUM_COMM = 12;

        const char* const* fnames = nullptr;
        int fcount = 0;
        if (event >= 0 && event < NUM_COMM) {
            fnames = COMM_DEFS[event].names;
            fcount = (static_cast<int>(len) > COMM_DEFS[event].fast_count)
                     ? COMM_DEFS[event].wide_count : COMM_DEFS[event].fast_count;
        }

        KVList data;
        for (unsigned i = 0; i < len && i < 16; ++i) {
            const char* key = (fnames && static_cast<int>(i) < fcount) ? fnames[i] : nullptr;
            std::string kname = key ? key : ("d" + std::to_string(i));
            data.push_back({kname, format_val(key, buf[i])});
        }

        write_event(cpu, "COMM", name, pid, tid, ts, std::move(data));
    }
}

static std::string extract_name_from_buf(const unsigned* buf, unsigned offset, unsigned len) {
    if (len <= offset) return {};
    std::string name(reinterpret_cast<const char*>(&buf[offset]),
                     (len - offset) * sizeof(unsigned));
    auto nul = name.find('\0');
    if (nul != std::string::npos) name.resize(nul);
    return name;
}

void KevParser::on_proc_create(unsigned, unsigned* buf, unsigned len) {
    if (prescan_mode_) return;
    int pid = (len >= 2) ? static_cast<int>(buf[1]) : 0;
    stats_->record(EventClass::PROCESS, pid, 0);
}

void KevParser::on_proc_destroy(unsigned, unsigned* buf, unsigned len) {
    if (prescan_mode_) return;
    int pid = (len >= 2) ? static_cast<int>(buf[1]) : 0;
    stats_->record(EventClass::PROCESS, pid, 0);
}

void KevParser::on_proc_create_name(unsigned, unsigned* buf, unsigned len) {
    if (len >= 3) {
        int parent_pid = static_cast<int>(buf[0]);
        int pid = static_cast<int>(buf[1]);
        std::string name = extract_name_from_buf(buf, 2, len);
        names_->set_process(pid, name, parent_pid);
        if (!prescan_mode_) {
            if (meta_written_)
                writer_->write_process(pid, name, parent_pid);
            else
                pending_names_.push_back({PendingName::PROC, pid, parent_pid, name});
        }
    }
    if (!prescan_mode_) {
        int pid = (len >= 2) ? static_cast<int>(buf[1]) : 0;
        stats_->record(EventClass::PROCESS, pid, 0);
    }
}

void KevParser::on_proc_thread_name(unsigned, unsigned* buf, unsigned len) {
    if (len >= 3) {
        int pid = static_cast<int>(buf[0]);
        int tid = static_cast<int>(buf[1]);
        std::string name = extract_name_from_buf(buf, 2, len);
        names_->set_thread(pid, tid, name);
        if (!prescan_mode_) {
            if (meta_written_)
                writer_->write_thread_name(pid, tid, name);
            else
                pending_names_.push_back({PendingName::THREAD, pid, tid, name});
        }
    }
    if (!prescan_mode_) {
        int pid = (len >= 1) ? static_cast<int>(buf[0]) : 0;
        stats_->record(EventClass::PROCESS, pid, 0);
    }
}

void KevParser::on_system(unsigned header, unsigned* buf, unsigned len) {
    int cpu = _NTO_TRACE_GETCPU(header);
    current_event_cpu_ = cpu;
    int event = _NTO_TRACE_GETEVENT(header);
    const char* name = event_names::system_name(event);

    int pid = 0, tid = 0;
    if (cpu < 64) {
        pid = cpu_ctx_[cpu].pid;
        tid = cpu_ctx_[cpu].tid;
    }

    int64_t ts = get_timestamp_ns();
    if (!check_ring_gap(ts)) return;
    if (prescan_mode_) return;

    stats_->record(EventClass::SYSTEM, pid, tid);
    stats_->update_time(ts);

    if (should_write(ts, pid, tid, "SYSTEM")) {
        struct SysFieldDef { const char* const* names; int count; };
        static const char* const F_PATHMGR[]    = {"pid", "tid"};
        static const char* const F_APS_NAME[]   = {"partition_id"};
        static const char* const F_APS_BUDG[]   = {"partition_id", "cpu_budget_pct",
                                                    "critical_budget_ms", "max_cpu_budget",
                                                    "critical_priority", "budget_pct_scale"};
        static const char* const F_APS_BNKR[]   = {"suspect_pid", "suspect_tid", "partition_id"};
        static const char* const F_MMAP[]       = {"pid", "addr_lo", "addr_hi",
                                                    "len_lo", "len_hi", "flags",
                                                    "prot", "fd", "align_lo", "align_hi",
                                                    "offset_lo", "offset_hi"};
        static const char* const F_MUNMAP[]     = {"pid", "addr_lo", "addr_hi",
                                                    "len_lo", "len_hi"};
        static const char* const F_MAPNAME[]    = {"pid", "addr", "len"};
        static const char* const F_ADDR[]       = {"addr"};
        static const char* const F_FUNC[]       = {"thisfn", "call_site"};
        static const char* const F_SLOG[]       = {"opcode", "severity"};
        static const char* const F_RUNSTATE[]   = {"bitset"};
        static const char* const F_POWER[]      = {"bitset", "mode"};
        static const char* const F_IPI[]        = {"ipicmd", "pad", "ip_lo", "ip_hi", "tid", "pid"};
        static const char* const F_PAGEWAIT[]   = {"pid", "tid", "ip", "vaddr"};
        static const char* const F_TIMER[]      = {"pid", "tid", "timer_id", "flags"};
        static const char* const F_DEFRAG_E[]   = {"rc", "freemem", "maxblock"};
        static const char* const F_PROFILE[]    = {"ip", "tid", "pid"};
        static const char* const F_MAPNAME64[]  = {"pid", "addr_lo", "addr_hi",
                                                    "len_lo", "len_hi"};

    #define SFD(a) {a, (int)(sizeof(a)/sizeof(a[0]))}
        static const struct { int code; SysFieldDef def; } SYS_FIELDS[] = {
            {0x02, SFD(F_PATHMGR)},   {0x03, SFD(F_APS_NAME)},
            {0x04, SFD(F_APS_BUDG)},  {0x05, SFD(F_APS_BNKR)},
            {0x06, SFD(F_MMAP)},      {0x07, SFD(F_MUNMAP)},
            {0x08, SFD(F_MAPNAME)},   {0x09, SFD(F_ADDR)},
            {0x0a, SFD(F_FUNC)},      {0x0b, SFD(F_FUNC)},
            {0x0c, SFD(F_SLOG)},      {0x0e, SFD(F_RUNSTATE)},
            {0x0f, SFD(F_POWER)},     {0x10, SFD(F_IPI)},
            {0x11, SFD(F_PAGEWAIT)},  {0x12, SFD(F_TIMER)},
            {0x13, SFD(F_DEFRAG_E)},  {0x14, SFD(F_PROFILE)},
            {0x15, SFD(F_MAPNAME64)},
        };
    #undef SFD

        const char* const* fnames = nullptr;
        int fcount = 0;
        for (auto& sf : SYS_FIELDS) {
            if (sf.code == event) {
                fnames = sf.def.names;
                fcount = sf.def.count;
                break;
            }
        }

        KVList data;
        for (unsigned i = 0; i < len && i < 16; ++i) {
            const char* key = (fnames && static_cast<int>(i) < fcount) ? fnames[i] : nullptr;
            if (key && std::strcmp(key, "empty") == 0) continue;
            std::string kname = key ? key : ("d" + std::to_string(i));
            data.push_back({kname, format_val(key, buf[i])});
        }

        write_event(cpu, "SYSTEM", name, pid, tid, ts, std::move(data));
    }
}

void KevParser::on_interrupt(unsigned header, unsigned* buf, unsigned len) {
    int cpu = _NTO_TRACE_GETCPU(header);
    current_event_cpu_ = cpu;
    int irq = _NTO_TRACE_GETEVENT(header);

    int64_t ts = get_timestamp_ns();
    if (!check_ring_gap(ts)) return;
    if (prescan_mode_) return;

    stats_->record(EventClass::INT, 0, 0);
    stats_->update_time(ts);

    char irq_name[32];
    std::snprintf(irq_name, sizeof(irq_name), "IRQ_%d", irq);

    if (should_write(ts, 0, 0, "INT")) {
        static const char* const INT_FIELDS[] = {"handler_ip", "area_p", "pid", "tid"};
        static constexpr int NUM_INT_FIELDS = 4;

        KVList data;
        for (unsigned i = 0; i < len && i < 8; ++i) {
            const char* key = (static_cast<int>(i) < NUM_INT_FIELDS) ? INT_FIELDS[i] : nullptr;
            std::string kname = key ? key : ("d" + std::to_string(i));
            data.push_back({kname, format_val(key, buf[i])});
        }

        write_event(cpu, "INT", irq_name, 0, 0, ts, std::move(data));
    }
}

} // namespace kev
