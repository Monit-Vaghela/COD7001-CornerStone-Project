#ifndef DASHBOARD_H
#define DASHBOARD_H
#pragma once
#include <string>
#include <vector>
#include <map>

// ─────────────────────────────────────────────────────────────────
//  DashboardJSON
//
//  Call from sequential simulator:
//      DashboardJSON::exportSeq(seqRecords, "seq_stats.json");
//
//  Call from parallel simulator:
//      DashboardJSON::exportPar(parRecords, "par_stats.json");
//
//  Then open dashboard.html — it loads both files automatically.
// ─────────────────────────────────────────────────────────────────
namespace DashboardJSON {

// One record per (T, D) batch — sequential simulator.
struct SeqDeltaRecord {
    int    time;
    int    delta;
    int    events;       // total events processed this batch
    double elapsedUs;    // wall-clock time to execute this batch (microseconds)
};

// One record per (T, D) batch — parallel simulator.
// levelMap mirrors deltaSimulations: level -> event count.
// Level -1 = SIGNAL_UPDATE (always serial).
// Level  0 = processes with no graph node (fallback).
// Level  1+ = actual circuit topology levels.
struct ParDeltaRecord {
    int    time;
    int    delta;
    std::map<int, int> levelMap;   // level -> event count
    int    ompThreshold;           // value of OMP_THRESHOLD used
    double elapsedUs;              // total wall-clock time for this batch (µs)
    double computeUs;              // time inside OMP parallel sections only (µs)
    double overheadUs;             // elapsedUs - computeUs: sync + commit + VCD + queue (µs)
};

void exportSeq(const std::vector<SeqDeltaRecord>& records,
               const std::string& path = "seq_stats.json");

void exportPar(const std::vector<ParDeltaRecord>& records,
               const std::string& path = "par_stats.json");

} // namespace DashboardJSON
#endif