#ifndef V3_SIM_H
#define V3_SIM_H

#include <simulator/simulator.h>
#include <preProcessor/circuitGraph.h>
#include <preProcessor/condensedGraph.h>
#include <circuitBuilder/gates/gate.h>
#include <circuitBuilder/core/behaviour.h>
#include <map>
#include <vector>
#include <unordered_map>

using namespace std;

class ParallelV3Simulator : public Simulator {
public:
    ParallelV3Simulator(Circuit* circuit, CircuitGraph* cir, CondensedGraph* cond);

    CircuitGraph*  circuitGraph;
    CondensedGraph* condensedGraph;

    // processesByLevel kept for potential future use / compatibility
    map<int, std::vector<Process*>> processesByLevel;
    int maxLevel;
    vector<Signal*> allSignals;

    void run()      override;
    void stabilize();

private:
    // Build a local RAW/WAW dependency graph over `procs` and execute
    // them using dynamic topological scheduling.
    void executeDynamic(const std::vector<Process*>& procs);
};

#endif // V3_SIM_H