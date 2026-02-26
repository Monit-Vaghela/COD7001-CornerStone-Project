#ifndef NOT_GATE_H
#define NOT_GATE_H

#include <core/process.h>
#include <core/signal.h>

class NotGate : public Process {
public:
    NotGate(Signal* a, Signal* y)
        : A(a), Y(y) 
    {
        gateId = "N" + std::to_string(++gateCounter);
    }

    void execute() override {
        Y->set_next(!A->get_value());
    }

    const std::string& getId() const { return gateId; }

    private:
    Signal* A;
    Signal* Y;

    std::string gateId;

    inline static int gateCounter = 0;
};

#endif