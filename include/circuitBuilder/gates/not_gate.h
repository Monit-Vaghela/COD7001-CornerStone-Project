#ifndef NOT_GATE_H
#define NOT_GATE_H

#include <circuitBuilder/core/process.h>
#include <circuitBuilder/core/signal.h>
#include <circuitBuilder/gates/gate.h>

class NotGate : public Gate{
public:
    NotGate(Signal* a, Signal* y)
        : A(a), Y(y) 
    {
        gateId = "N" + std::to_string(++gateCounter);
    }

    void execute() override {
        Y->set_next(!A->get_value());
    }
    ProcessType getType() const override {
        return ProcessType::GATE;
    }

    std::string getName() const override {
        return "NOT";
    }
    const std::string& getId() const { return gateId; }

    std::vector<Signal*> getInputs() const override {
        return {A};
    }
    std::vector<Signal*> getOutputs() const override {
        return {Y};
    }
    
private:
    Signal* A;
    Signal* Y;

    std::string gateId;

    inline static int gateCounter = 0;
};

#endif