#ifndef GATE_H
#define GATE_H

#include <circuitBuilder/core/process.h>
#include <circuitBuilder/core/signal.h>

class Gate : public Process {
public:
    virtual std::vector<Signal*> getInputs() const = 0;
    virtual std::vector<Signal*> getOutputs() const = 0;
};

#endif