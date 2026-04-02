#ifndef SEQUENTIAL_SIM_H
#define SEQUENTIAL_SIM_H

#include <simulator/simulator.h>

class SequentialSimulator : public Simulator {
public:
    SequentialSimulator(Circuit* circuit);

    void run() override;
    void stabilize();
    
};

#endif