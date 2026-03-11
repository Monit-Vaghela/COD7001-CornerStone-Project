#ifndef AND_GATE_H
#define AND_GATE_H

#include <circuitBuilder/core/process.h>
#include <circuitBuilder/core/signal.h>
#include <circuitBuilder/gates/gate.h>

class AndGate : public Gate {
public:
    AndGate(Signal* a, Signal* b, Signal* y)
        : A(a), B(b), Y(y)
    {
        gateId = "A" + std::to_string(++gateCounter);
    }

    void execute() override {
        Y->set_next(A->get_value() && B->get_value());
    }

    ProcessType getType() const override {
        return ProcessType::GATE;
    }

    std::string getName() const override {
        return "AND";
    }
    const std::string& getId() const { return gateId; }

    std::vector<Signal*> getInputs() const override {
        return {A, B};
    }
    std::vector<Signal*> getOutputs() const override {
        return {Y};
    }
    
private:
    Signal* A;
    Signal* B;
    Signal* Y;

    std::string gateId;
    // shared accross all and gates
    inline static int gateCounter = 0;
};

#endif