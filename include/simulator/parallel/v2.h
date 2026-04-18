#ifndef V2_SIM_H
#define V2_SIM_H

#include <simulator/simulator.h>
#include <preProcessor/circuitGraph.h>
#include <preProcessor/condensedGraph.h>
#include <map>
#include <vector>

using namespace std;
class ParallelV2Simulator : public Simulator {
public:
    ParallelV2Simulator(Circuit* circuit, CircuitGraph* cir, CondensedGraph* cond);

    CircuitGraph* circuitGraph;
    CondensedGraph* condensedGraph;
    map<int, std::vector<Process*>> processesByLevel;
    int maxLevel;
    vector<Signal*> allSignals;
    void run() override;
    void stabilize();
};

#endif