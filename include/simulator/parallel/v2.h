#ifndef V2_SIM_H
#define V2_SIM_H

#include <simulator/simulator.h>

class ParallelV2Simulator : public Simulator {
public:
    ParallelV2Simulator(Circuit* circuit);

    void run() override;
    void stabilize();
};

#endif