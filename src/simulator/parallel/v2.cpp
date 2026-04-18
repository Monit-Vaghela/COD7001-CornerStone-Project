#include <simulator/parallel/v2.h>
#include <circuitBuilder/utils/logger.h>
#include <circuitBuilder/gates/gate.h>
#include <vector>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <omp.h>
#include <map>
#include <unordered_map>
#include <mutex>

using namespace std;

// Minimum events in a level batch to justify OMP thread spawn overhead.
// Below this threshold, run sequentially to avoid paying ~10-50µs of OMP
// startup cost for work that takes <1µs.
static constexpr int OMP_THRESHOLD = 16;

vector<map<int, int>> deltaSimulations;

ParallelV2Simulator::ParallelV2Simulator(Circuit* c, CircuitGraph* cir, CondensedGraph* cond)
    : Simulator(c), circuitGraph(cir), condensedGraph(cond) {
    vcd = new VCDWriter("wavePar.vcd");
    for (auto* s : circuit->get_signals()) vcd->registerSignal(s->get_name());
    vcd->endDefinitions();
    for (auto* process : circuit->get_processes()) {
        auto it = cir->nodeMap.find(process);
        if (it != cir->nodeMap.end() && it->second != nullptr) {
            processesByLevel[it->second->level].push_back(process);
        }
    }
}

void ParallelV2Simulator::run() {
    for (auto* s : circuit->get_signals()) {
        vcd->dumpValue(s->get_name(), s->get_value());
    }

    int last_time = 0;
    int simulatedEvents = 0;

    // Pre-build a lookup from signal pointer -> index so the commit phase
    // can write into a thread-local changed-signals array without a mutex.
    const auto& allSignals = circuit->get_signals();
    const int numSignals = (int)allSignals.size();

    // Per-thread dirty flags: one byte per signal, indexed by signal index.
    // Thread i marks dirtyFlags[i * numSignals + sigIdx] = 1.
    // After the parallel section we OR the columns to find changed signals.
    // This eliminates false sharing between threads writing adjacent signals.
    int numThreads = 1;
    #pragma omp parallel
    {
        #pragma omp single
        numThreads = omp_get_num_threads();
    }

    // signal index map: pointer -> index into allSignals
    unordered_map<Signal*, int> signalIndex;
    signalIndex.reserve(numSignals);
    for (int i = 0; i < numSignals; i++) {
        signalIndex[allSignals[i]] = i;
    }

    while (!event_queue.empty()) {
        int T = event_queue.top().time;
        int D = event_queue.top().delta;

        current_time = T;
        current_delta = D;
        if (T != last_time) {
            vcd->dumpTime(T);
            last_time = T;
        }

        // Collect all events at this (T, D)
        std::vector<Event> current_batch;
        while (!event_queue.empty() &&
               event_queue.top().time == T &&
               event_queue.top().delta == D) {
            current_batch.push_back(event_queue.pop());
        }

        // Group by level.
        // Use unordered_map for O(1) insertion instead of O(log n) std::map.
        // SIGNAL_UPDATE -> level -1, always runs first.
        map<int, int> thisLevelSimulations;
        unordered_map<int, std::vector<Event*>> byLevel;
        byLevel.reserve(16);
        for (auto& e : current_batch) {
            if (e.type == Event::SIGNAL_UPDATE) {
                byLevel[-1].push_back(&e);
                continue;
            }
            auto it = circuitGraph->nodeMap.find(e.process);
            if (it == circuitGraph->nodeMap.end() || it->second == nullptr) {
                byLevel[0].push_back(&e);
                continue;
            }
            byLevel[it->second->level].push_back(&e);
        }

        // Execute level by level — implicit OMP barrier between levels
        // (parallel for has an implied barrier at its closing brace).
        // Use sorted iteration so level -1 (SIGNAL_UPDATE) always runs first.
        std::vector<std::pair<int, std::vector<Event*>*>> sortedLevels;
        sortedLevels.reserve(byLevel.size());
        for (auto& [lvl, evts] : byLevel) {
            sortedLevels.push_back({lvl, &evts});
        }
        std::sort(sortedLevels.begin(), sortedLevels.end(),
                  [](const auto& a, const auto& b){ return a.first < b.first; });

        for (auto& [level, eventsPtr] : sortedLevels) {
            auto& events = *eventsPtr;
            int n = (int)events.size();
            thisLevelSimulations[level] = n;

            if (n >= OMP_THRESHOLD) {
                // Large batch: worth parallelising.
                #pragma omp parallel for schedule(dynamic, 4)
                for (int i = 0; i < n; i++) {
                    Event* e = events[i];
                    if (e->type == Event::SIGNAL_UPDATE) {
                        e->signal->set_next(e->value);
                    } else if (e->type == Event::PROCESS_EXEC) {
                        e->process->execute();
                    }
                }
                // Implicit OMP barrier here — correct inter-level ordering.
            } else {
                // Small batch: run sequentially to avoid OMP overhead.
                for (int i = 0; i < n; i++) {
                    Event* e = events[i];
                    if (e->type == Event::SIGNAL_UPDATE) {
                        e->signal->set_next(e->value);
                    } else if (e->type == Event::PROCESS_EXEC) {
                        simulatedEvents++;
                        e->process->execute();
                    }
                }
            }
            // Count simulated events for the parallel path too
            if (n >= OMP_THRESHOLD) {
                for (int i = 0; i < n; i++) {
                    if (events[i]->type == Event::PROCESS_EXEC) simulatedEvents++;
                }
            }
        }

        // ----------------------------------------------------------------
        // Commit phase: parallelise signal commit, then gather changed ones.
        //
        // Strategy: each thread commits its slice of allSignals and records
        // which indices changed into a thread-local vector. After the parallel
        // section the master collects all changed signals and does the serial
        // work (VCD write + sensitive-list walk) which cannot be parallelised
        // without locking.
        // ----------------------------------------------------------------

        // Per-thread changed-signal index lists (avoid false sharing via
        // separate vectors rather than a shared array).
        std::vector<std::vector<int>> threadChanged(numThreads);

        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            auto& local = threadChanged[tid];
            #pragma omp for schedule(static)
            for (int i = 0; i < numSignals; i++) {
                Signal* s = allSignals[i];
                bool oldVal = s->get_value();
                s->commit();                    // writes next -> current
                if (s->get_value() != oldVal) {
                    local.push_back(i);
                }
            }
        }

        // Collect and deduplicate changed signals (order doesn't matter here).
        std::vector<Signal*> changedSignals;
        for (auto& local : threadChanged) {
            for (int idx : local) {
                changedSignals.push_back(allSignals[idx]);
            }
        }

        // VCD write and sensitive-list scheduling: inherently serial.
        for (auto* signal : changedSignals) {
            vcd->dumpValue(signal->get_name(), signal->get_value());
            for (auto* process : signal->get_sensitive_list()) {
                event_queue.push(Event(T, D + 1, process));
            }
        }

        deltaSimulations.push_back(thisLevelSimulations);
    }

    // Final state dump
    if (last_time != -1) {
        for (auto& levelMap : deltaSimulations) {
            for (auto& [level, count] : levelMap) {
                cout << count << ", ";
            }
            cout << "\n";
        }
        vcd->dumpTime(current_time + 1);
        cout << "Parallel Simulated " << simulatedEvents << "\n";
    }

    delete vcd;
    vcd = nullptr;
}

void ParallelV2Simulator::stabilize() {
    int stop = 0;
    for (auto* process : circuit->get_processes()) {
        if (process->getType() == Process::ProcessType::GATE)
            event_queue.push(Event(0, 0, process));
    }

    while (!event_queue.empty()) {
        if (stop > 100000) {
            cout << "[Simulator / Error] Cant stabilize circuit. check inputs\n";
            return;
        }
        int T = event_queue.top().time;
        int D = event_queue.top().delta;
        if (T != 0) break;

        std::vector<Event> batch;
        while (!event_queue.empty() &&
               event_queue.top().time == T &&
               event_queue.top().delta == D) {
            batch.push_back(event_queue.pop());
        }

        // Stabilize runs at init time; parallelise if the batch is large enough
        int n = (int)batch.size();
        if (n >= OMP_THRESHOLD) {
            #pragma omp parallel for schedule(dynamic, 4)
            for (int i = 0; i < n; i++) {
                if (batch[i].type == Event::PROCESS_EXEC)
                    batch[i].process->execute();
            }
        } else {
            for (auto& e : batch) {
                if (e.type == Event::PROCESS_EXEC)
                    e.process->execute();
            }
        }

        std::vector<Signal*> changed;
        for (auto* s : circuit->get_signals()) {
            bool old = s->get_value();
            s->commit();
            if (old != s->get_value())
                changed.push_back(s);
        }

        for (auto* s : changed) {
            for (auto* p : s->get_sensitive_list()) {
                event_queue.push(Event(0, D + 1, p));
                stop++;
            }
        }
    }

    for (auto* s : circuit->get_signals()) {
        s->initializeSignal();
    }
}