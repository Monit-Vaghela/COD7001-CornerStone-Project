// comparator.cpp — diff two canonical trace files produced by TraceWriter.
//
// Build:   g++ -O2 -std=c++17 comparator.cpp -o comparator
// Usage:   ./comparator trace_seq.bin trace_par.bin
//
// Exit 0 = traces are identical.
// Exit 1 = mismatch (details printed to stdout).
// Exit 2 = file error or bad format.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static constexpr uint32_t TRACE_MAGIC = 0x54524143u; // 'TRAC'

// ---------------------------------------------------------------------------
// Minimal binary reader
// ---------------------------------------------------------------------------
struct Reader {
    FILE*       fp   = nullptr;
    const char* path = nullptr;
    uint64_t    pos  = 0;   // bytes read (for diagnostics)

    bool open(const char* p) {
        path = p;
        fp = std::fopen(p, "rb");
        return fp != nullptr;
    }
    ~Reader() { if (fp) std::fclose(fp); }

    bool read_bytes(void* dst, size_t n) {
        size_t got = std::fread(dst, 1, n, fp);
        pos += got;
        return got == n;
    }
    bool read_i32(int32_t& v)  { return read_bytes(&v, 4); }
    bool read_u32(uint32_t& v) { return read_bytes(&v, 4); }
    bool read_u8 (uint8_t&  v) { return read_bytes(&v, 1); }
    bool eof() { return std::feof(fp) != 0; }
};

// ---------------------------------------------------------------------------
// Snapshot representation
// ---------------------------------------------------------------------------
struct Entry { uint32_t hash; uint8_t value; };

struct Snapshot {
    int32_t             time  = 0;
    int32_t             delta = 0;
    std::vector<Entry>  entries;
};

static bool read_snapshot(Reader& r, Snapshot& snap) {
    if (!r.read_i32(snap.time))  return false;
    if (!r.read_i32(snap.delta)) return false;
    int32_t n = 0;
    if (!r.read_i32(n)) return false;
    if (n < 0 || n > 1'000'000) {
        std::printf("  [%s] Corrupt record: num_signals=%d at byte %llu\n",
                    r.path, n, (unsigned long long)r.pos);
        return false;
    }
    snap.entries.resize(static_cast<size_t>(n));
    for (auto& e : snap.entries) {
        if (!r.read_u32(e.hash)) return false;
        if (!r.read_u8 (e.value)) return false;
    }
    return true;
}

static bool check_magic(Reader& r) {
    uint32_t magic = 0;
    if (!r.read_u32(magic)) return false;
    if (magic != TRACE_MAGIC) {
        std::printf("  [%s] Bad magic: expected 0x%08X, got 0x%08X\n",
                    r.path, TRACE_MAGIC, magic);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Comparison
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::printf("Usage: %s <trace_a> <trace_b>\n", argv[0]);
        return 2;
    }

    Reader ra, rb;
    if (!ra.open(argv[1])) { std::printf("Cannot open %s\n", argv[1]); return 2; }
    if (!rb.open(argv[2])) { std::printf("Cannot open %s\n", argv[2]); return 2; }

    if (!check_magic(ra) || !check_magic(rb)) return 2;

    std::printf("Comparing:\n  A: %s\n  B: %s\n\n", argv[1], argv[2]);

    int    mismatches  = 0;
    int    snapshots   = 0;
    bool   length_diff = false;

    while (true) {
        Snapshot sa, sb;
        bool ok_a = read_snapshot(ra, sa);
        bool ok_b = read_snapshot(rb, sb);

        if (!ok_a && !ok_b) break;  // both exhausted — normal exit

        if (ok_a != ok_b) {
            length_diff = true;
            std::printf("MISMATCH: trace length differs after %d snapshots.\n"
                        "  %s has %s snapshots remaining.\n",
                        snapshots,
                        ok_a ? argv[1] : argv[2],
                        ok_a ? "more" : "fewer");
            mismatches++;
            break;
        }

        snapshots++;

        // Check (time, delta)
        if (sa.time != sb.time || sa.delta != sb.delta) {
            std::printf("MISMATCH at snapshot %d: step mismatch\n"
                        "  A: T=%d D=%d\n  B: T=%d D=%d\n",
                        snapshots, sa.time, sa.delta, sb.time, sb.delta);
            mismatches++;
            if (mismatches >= 20) { std::printf("  (too many mismatches, stopping)\n"); break; }
            continue;
        }

        // Check signal count
        if (sa.entries.size() != sb.entries.size()) {
            std::printf("MISMATCH at T=%d D=%d: signal count A=%zu B=%zu\n",
                        sa.time, sa.delta,
                        sa.entries.size(), sb.entries.size());
            mismatches++;
            if (mismatches >= 20) { std::printf("  (too many mismatches, stopping)\n"); break; }
            continue;
        }

        // Check each signal (both sides are sorted by hash)
        for (size_t i = 0; i < sa.entries.size(); i++) {
            const auto& ea = sa.entries[i];
            const auto& eb = sb.entries[i];
            if (ea.hash != eb.hash) {
                std::printf("MISMATCH at T=%d D=%d index %zu: "
                            "signal hash A=0x%08X B=0x%08X (different signal sets)\n",
                            sa.time, sa.delta, i, ea.hash, eb.hash);
                mismatches++;
                break;
            }
            if (ea.value != eb.value) {
                std::printf("MISMATCH at T=%d D=%d: signal hash 0x%08X "
                            "A=%u B=%u\n",
                            sa.time, sa.delta, ea.hash,
                            (unsigned)ea.value, (unsigned)eb.value);
                mismatches++;
            }
        }

        if (mismatches >= 20) { std::printf("  (too many mismatches, stopping)\n"); break; }
    }

    std::printf("\nSnapshots compared: %d\n", snapshots);
    if (mismatches == 0) {
        std::printf("PASS — traces are identical.\n");
        return 0;
    } else {
        std::printf("FAIL — %d mismatch(es) found.\n", mismatches);
        return 1;
    }
}