#include <simulator/parallel/v3.h>
#include <circuitBuilder/utils/logger.h>
#include <circuitBuilder/gates/gate.h>
#include <circuitBuilder/core/behaviour.h>
#include <circuitBuilder/utils/Dashboardjson.h>
#include <vector>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <omp.h>
#include <map>
#include <unordered_map>
#include <mutex>
#include <atomic>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// getProcessSignals
//
// Process base class has no get_input/output_signals(); we obtain them by
// downcasting to the two concrete types (Gate, Behaviour).
//
// Gate:
//   inputs  = getInputs()        (vector<Signal*>)
//   outputs = getOutputs()       (vector<Signal*>, always 1 element)
//
// Behaviour:
//   readers = { getTrigger(), getInput() }   — signals this process reads
//   writers = { getOutput() }                — signal this process writes
//
// Any unknown ProcessType is treated as having no I/O, so it gets
// inDegree 0 and runs in the first wave (safe default).
// ─────────────────────────────────────────────────────────────────────────────
static void getProcessSignals(Process* p,
                               std::vector<Signal*>& readers,
                               std::vector<Signal*>& writers)
{
    readers.clear();
    writers.clear();

    switch (p->getType()) {
        case Process::ProcessType::GATE: {
            Gate* g = static_cast<Gate*>(p);
            readers = g->getInputs();          // vector<Signal*>
            writers = g->getOutputs();         // vector<Signal*>  (1 element)
            break;
        }
        case Process::ProcessType::BEHAVIOUR: {
            Behaviour* b = static_cast<Behaviour*>(p);
            // A Behaviour reads its trigger and its data input;
            // it writes its single output.
            readers.push_back(b->getTrigger());
            readers.push_back(b->getInput());
            writers.push_back(b->getOutput());
            break;
        }
        default:
            // Unknown process type — no dependencies, runs freely.
            break;
    }
}

// Minimum processes in the ready-set to justify spawning OMP threads.
static constexpr int OMP_THRESHOLD = 16;

// ─── dashboard bookkeeping (mirrors v2) ──────────────────────────────────────
static vector<map<int, int>>          deltaSimulations_v3;
static vector<std::pair<int,int>>     deltaTimes_v3;
static vector<double>                 elapsedTime_v3;

// ─── Constructor ─────────────────────────────────────────────────────────────
ParallelV3Simulator::ParallelV3Simulator(Circuit* c,
                                         CircuitGraph*   cir,
                                         CondensedGraph* cond)
    : Simulator(c), circuitGraph(cir), condensedGraph(cond)
{
    vcd = new VCDWriter("wavePar3.vcd");
    for (auto* s : circuit->get_signals()) vcd->registerSignal(s->get_name());
    vcd->endDefinitions();

    for (auto* process : circuit->get_processes()) {
        auto it = cir->nodeMap.find(process);
        if (it != cir->nodeMap.end() && it->second != nullptr) {
            processesByLevel[it->second->level].push_back(process);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// executeDynamic
//
// Given a set of processes that all belong to the current (T, D) batch,
// build a local dependency graph and execute them with a ready-set loop.
//
// Dependency types modelled
//   RAW (Read-After-Write): writer → reader on the same signal
//   WAW (Write-After-Write): earlier writer → later writer on same signal
//   (WAR is not needed because within one delta all reads see the *previous*
//    committed value; writes only become visible after commit.)
// ─────────────────────────────────────────────────────────────────────────────
void ParallelV3Simulator::executeDynamic(const std::vector<Process*>& procs)
{
    if (procs.empty()) return;

    const int N = (int)procs.size();

    // Assign a compact index 0..N-1 to every process in this batch.
    unordered_map<Process*, int> procIndex;
    procIndex.reserve(N);
    for (int i = 0; i < N; i++) procIndex[procs[i]] = i;

    // Adjacency list and in-degree, indexed by the compact index.
    vector<vector<int>> adj(N);
    vector<int>         inDegree(N, 0);

    // Map signal → writers/readers *within this batch* (compact indices).
    unordered_map<Signal*, vector<int>> signalWriters;
    unordered_map<Signal*, vector<int>> signalReaders;
    signalWriters.reserve(N * 2);
    signalReaders.reserve(N * 2);

    std::vector<Signal*> tmpReaders, tmpWriters;
    for (int i = 0; i < N; i++) {
        getProcessSignals(procs[i], tmpReaders, tmpWriters);
        for (auto* s : tmpWriters)  signalWriters[s].push_back(i);
        for (auto* s : tmpReaders)  signalReaders[s].push_back(i);
    }

    // Deduplicate helper — avoids multi-edges inflating inDegree.
    // We record each (from, to) pair at most once.
    // Using a flat set encoded as a sorted vector would be expensive;
    // instead we use a per-node unordered_set built on the fly.
    vector<unordered_set<int>> adjSet(N);

    auto addEdge = [&](int from, int to) {
        if (from == to) return;
        if (adjSet[from].insert(to).second) {   // true → new edge
            adj[from].push_back(to);
            inDegree[to]++;
        }
    };

    // RAW: writer → reader
    for (auto& [sig, writers] : signalWriters) {
        auto rit = signalReaders.find(sig);
        if (rit == signalReaders.end()) continue;
        for (int w : writers)
            for (int r : rit->second)
                addEdge(w, r);
    }

    // WAW: for each signal with multiple writers impose a total order
    // (preserves determinism; the order chosen is the batch order).
    for (auto& [sig, writers] : signalWriters) {
        if (writers.size() < 2) continue;
        for (int i = 0; i < (int)writers.size() - 1; i++)
            addEdge(writers[i], writers[i + 1]);
    }

    // ── Dynamic topological scheduling ──────────────────────────────────────
    // Seed the ready set with all nodes whose in-degree is 0.
    vector<int> ready;
    ready.reserve(N);
    for (int i = 0; i < N; i++)
        if (inDegree[i] == 0) ready.push_back(i);

    while (!ready.empty()) {
        int waveSize = (int)ready.size();

        // ── Parallel execution of the current wave ─────────────────────────
        if (waveSize >= OMP_THRESHOLD) {
            #pragma omp parallel for schedule(dynamic, 4)
            for (int i = 0; i < waveSize; i++) {
                procs[ready[i]]->execute();
            }
        } else {
            for (int idx : ready) {
                procs[idx]->execute();
            }
        }

        // ── Dependency update (sequential — very fast) ─────────────────────
        vector<int> nextReady;
        nextReady.reserve(waveSize);   // conservative hint

        for (int idx : ready) {
            for (int nb : adj[idx]) {
                if (--inDegree[nb] == 0)
                    nextReady.push_back(nb);
            }
        }

        ready = std::move(nextReady);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// run
// ─────────────────────────────────────────────────────────────────────────────
void ParallelV3Simulator::run()
{
    for (auto* s : circuit->get_signals())
        vcd->dumpValue(s->get_name(), s->get_value());

    int last_time       = 0;
    int simulatedEvents = 0;

    // Build signal-index map once (used in the parallel commit phase).
    const auto& allSignals = circuit->get_signals();
    const int   numSignals = (int)allSignals.size();

    int numThreads = 1;
    #pragma omp parallel
    {
        #pragma omp single
        numThreads = omp_get_num_threads();
    }

    unordered_map<Signal*, int> signalIndex;
    signalIndex.reserve(numSignals);
    for (int i = 0; i < numSignals; i++)
        signalIndex[allSignals[i]] = i;

    // ── Main simulation loop ─────────────────────────────────────────────────
    while (!event_queue.empty()) {
        if (simulatedEvents > 100000) {
            cout << "[Simulator / Error] : Stuck in looping Delta Cycles\n";
            return;
        }

        int T = event_queue.top().time;
        int D = event_queue.top().delta;

        deltaTimes_v3.push_back({T, D});
        current_time  = T;
        current_delta = D;
        if (T != last_time) {
            vcd->dumpTime(T);
            last_time = T;
        }

        auto t_start = std::chrono::high_resolution_clock::now();

        // ── STEP 1: Extract batch ────────────────────────────────────────────
        std::vector<Event> current_batch;
        while (!event_queue.empty() &&
               event_queue.top().time  == T &&
               event_queue.top().delta == D) {
            current_batch.push_back(event_queue.pop());
        }

        std::vector<Event*>   signalEvents;
        std::vector<Process*> processEvents;
        signalEvents.reserve(current_batch.size());
        processEvents.reserve(current_batch.size());

        for (auto& e : current_batch) {
            if (e.type == Event::SIGNAL_UPDATE)
                signalEvents.push_back(&e);
            else
                processEvents.push_back(e.process);
        }

        // ── STEP 2: Apply signal updates (sequential) ────────────────────────
        for (auto* e : signalEvents)
            e->signal->set_next(e->value);

        // ── STEP 3 + 4: Build local dep graph & execute dynamically ──────────
        //
        // We also record a "level simulation" map for the dashboard.
        // Since V3 doesn't use circuit-graph levels for scheduling, we bucket
        // by the *wave index* (0-based round of the ready-set loop) instead.
        // This gives an equally informative picture of parallelism.
        //
        // To capture the wave counts we inline the executeDynamic logic here
        // so we can collect the waveSizes without extra overhead.

        map<int, int> thisLevelSimulations;   // wave_idx → process_count

        // ── inline executeDynamic with dashboard instrumentation ─────────────
        {
            const int N = (int)processEvents.size();

            if (N > 0) {
                unordered_map<Process*, int> procIndex;
                procIndex.reserve(N);
                for (int i = 0; i < N; i++) procIndex[processEvents[i]] = i;

                vector<vector<int>> adj(N);
                vector<int>         inDegree(N, 0);

                unordered_map<Signal*, vector<int>> signalWriters;
                unordered_map<Signal*, vector<int>> signalReaders;
                signalWriters.reserve(N * 2);
                signalReaders.reserve(N * 2);

                std::vector<Signal*> tmpReaders, tmpWriters;
                for (int i = 0; i < N; i++) {
                    getProcessSignals(processEvents[i], tmpReaders, tmpWriters);
                    for (auto* s : tmpWriters)  signalWriters[s].push_back(i);
                    for (auto* s : tmpReaders)  signalReaders[s].push_back(i);
                }

                vector<unordered_set<int>> adjSet(N);
                auto addEdge = [&](int from, int to) {
                    if (from == to) return;
                    if (adjSet[from].insert(to).second) {
                        adj[from].push_back(to);
                        inDegree[to]++;
                    }
                };

                for (auto& [sig, writers] : signalWriters) {
                    auto rit = signalReaders.find(sig);
                    if (rit == signalReaders.end()) continue;
                    for (int w : writers)
                        for (int r : rit->second)
                            addEdge(w, r);
                }
                for (auto& [sig, writers] : signalWriters) {
                    if (writers.size() < 2) continue;
                    for (int i = 0; i < (int)writers.size() - 1; i++)
                        addEdge(writers[i], writers[i + 1]);
                }

                vector<int> ready;
                ready.reserve(N);
                for (int i = 0; i < N; i++)
                    if (inDegree[i] == 0) ready.push_back(i);

                int waveIdx = 0;
                while (!ready.empty()) {
                    int waveSize = (int)ready.size();
                    thisLevelSimulations[waveIdx] = waveSize;
                    simulatedEvents += waveSize;

                    if (waveSize >= OMP_THRESHOLD) {
                        #pragma omp parallel for schedule(dynamic, 4)
                        for (int i = 0; i < waveSize; i++)
                            processEvents[ready[i]]->execute();
                    } else {
                        for (int idx : ready)
                            processEvents[idx]->execute();
                    }

                    vector<int> nextReady;
                    nextReady.reserve(waveSize);
                    for (int idx : ready)
                        for (int nb : adj[idx])
                            if (--inDegree[nb] == 0)
                                nextReady.push_back(nb);

                    ready = std::move(nextReady);
                    waveIdx++;
                }
            }
        }
        // ── end inline executeDynamic ────────────────────────────────────────

        // ── STEP 5: Commit phase (parallel) ──────────────────────────────────
        std::vector<std::vector<int>> threadChanged(numThreads);

        #pragma omp parallel
        {
            int tid    = omp_get_thread_num();
            auto& local = threadChanged[tid];
            #pragma omp for schedule(static)
            for (int i = 0; i < numSignals; i++) {
                Signal* s    = allSignals[i];
                bool    oldVal = s->get_value();
                s->commit();
                if (s->get_value() != oldVal)
                    local.push_back(i);
            }
        }

        std::vector<Signal*> changedSignals;
        for (auto& local : threadChanged)
            for (int idx : local)
                changedSignals.push_back(allSignals[idx]);

        // ── STEP 6: VCD + next-delta scheduling (serial) ─────────────────────
        for (auto* signal : changedSignals) {
            vcd->dumpValue(signal->get_name(), signal->get_value());
            for (auto* process : signal->get_sensitive_list())
                event_queue.push(Event(T, D + 1, process));
        }

        auto t_end = std::chrono::high_resolution_clock::now();
        double elapsed_us =
            std::chrono::duration<double, std::micro>(t_end - t_start).count();

        elapsedTime_v3.push_back(elapsed_us);
        deltaSimulations_v3.push_back(thisLevelSimulations);
    }

    // ── Final state dump ──────────────────────────────────────────────────────
    if (last_time != -1) {
        vcd->dumpTime(current_time + 1);
        cout << "ParallelV3 Simulated " << simulatedEvents << "\n";

        std::vector<DashboardJSON::ParDeltaRecord> parRecords;
        for (int i = 0; i < (int)deltaSimulations_v3.size(); i++) {
            parRecords.push_back({
                deltaTimes_v3[i].first,    // time
                deltaTimes_v3[i].second,   // delta
                deltaSimulations_v3[i],    // waveMap (wave_idx → count)
                OMP_THRESHOLD,
                elapsedTime_v3[i]
            });
        }
        DashboardJSON::exportPar(parRecords, "dashboard/par_v3_stats.json");
    }

    delete vcd;
    vcd = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// stabilize  (identical logic to v2 — bootstrap circuit to a quiescent state)
// ─────────────────────────────────────────────────────────────────────────────
void ParallelV3Simulator::stabilize()
{
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
               event_queue.top().time  == T &&
               event_queue.top().delta == D) {
            batch.push_back(event_queue.pop());
        }

        // Collect processes from this batch and execute dynamically.
        std::vector<Process*> procs;
        procs.reserve(batch.size());
        for (auto& e : batch)
            if (e.type == Event::PROCESS_EXEC)
                procs.push_back(e.process);

        executeDynamic(procs);

        std::vector<Signal*> changed;
        for (auto* s : circuit->get_signals()) {
            bool old = s->get_value();
            s->commit();
            if (old != s->get_value())
                changed.push_back(s);
        }

        for (auto* s : changed)
            for (auto* p : s->get_sensitive_list()) {
                event_queue.push(Event(0, D + 1, p));
                stop++;
            }
    }

    for (auto* s : circuit->get_signals())
        s->initializeSignal();
}