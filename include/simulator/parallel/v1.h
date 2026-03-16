#ifndef V1_SIM_H
#define V1_SIM_H

#include <simulator/simulator.h>

class ParallelV1Simulator : public Simulator {
public:
    ParallelV1Simulator(Circuit* circuit);

    void run() override;
};

#endif