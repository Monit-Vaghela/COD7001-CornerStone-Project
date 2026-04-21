#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstring>

// ---------------------------------------------------------------------------
// TraceWriter — emits canonical signal snapshots for correctness comparison.
//
// One snapshot is written per (time, delta) step, AFTER all signal commits
// for that step are done. Signals are sorted by their name's FNV-1a hash
// before writing, making the output fully deterministic regardless of the
// order in which the parallel simulator commits signals.
//
// Binary record layout (little-endian):
//   int32_t  time
//   int32_t  delta
//   int32_t  num_signals
//   For each signal (sorted by hash):
//     uint32_t  name_hash   (FNV-1a 32-bit)
//     uint8_t   value       (0 or 1)
//
// The file begins with a 4-byte magic number 0x54524143 ('TRAC') so the
// comparator can detect truncated or wrong-format files early.
// ---------------------------------------------------------------------------

static constexpr uint32_t TRACE_MAGIC = 0x54524143u; // 'TRAC'

inline uint32_t fnv1a_32(const std::string& s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}

class TraceWriter {
public:
    explicit TraceWriter(const std::string& filename) {
        file_.open(filename, std::ios::binary);
        if (!file_.is_open()) return;
        file_.write(reinterpret_cast<const char*>(&TRACE_MAGIC), 4);
    }

    ~TraceWriter() {
        if (file_.is_open()) {
            file_.flush();
            file_.close();
        }
    }

    // Call once at startup to pre-compute hashes for all signal names.
    // The order of this call doesn't matter; sorting is done per snapshot.
    void registerSignals(const std::vector<std::string>& names) {
        entries_.reserve(names.size());
        for (const auto& name : names) {
            entries_.push_back({ fnv1a_32(name), name, false });
        }
        // Sort once; value updates find entries by name at write time.
        std::sort(entries_.begin(), entries_.end(),
                  [](const Entry& a, const Entry& b){ return a.hash < b.hash; });
    }

    // Update the stored value for a signal. Call for every changed signal
    // after the commit phase, or call updateAll() with the full signal list.
    void updateValue(const std::string& name, bool value) {
        uint32_t h = fnv1a_32(name);
        // Binary search by hash (hashes are unique per signal name).
        auto it = std::lower_bound(entries_.begin(), entries_.end(), h,
                                   [](const Entry& e, uint32_t h){ return e.hash < h; });
        if (it != entries_.end() && it->hash == h) {
            it->value = value;
        }
    }

    // Convenience: update all signals at once from a parallel array.
    // T must expose .get_name() -> std::string and .get_value() -> bool.
    template<typename SignalPtr>
    void updateAll(const std::vector<SignalPtr>& signals) {
        for (auto* s : signals) {
            updateValue(s->get_name(), s->get_value());
        }
    }

    // Write one snapshot for the completed (time, delta) step.
    // Call AFTER all commits for this step are done and updateValue/updateAll
    // has been called.
    void writeSnapshot(int32_t time, int32_t delta) {
        if (!file_.is_open()) return;
        int32_t n = static_cast<int32_t>(entries_.size());
        write32(time);
        write32(delta);
        write32(n);
        for (const auto& e : entries_) {
            write_u32(e.hash);
            uint8_t v = e.value ? 1u : 0u;
            file_.write(reinterpret_cast<const char*>(&v), 1);
        }
    }

    bool good() const { return file_.is_open() && file_.good(); }

private:
    struct Entry {
        uint32_t    hash;
        std::string name;
        bool        value = false;
    };

    std::ofstream      file_;
    std::vector<Entry> entries_;

    void write32(int32_t v)  { file_.write(reinterpret_cast<const char*>(&v), 4); }
    void write_u32(uint32_t v){ file_.write(reinterpret_cast<const char*>(&v), 4); }
};