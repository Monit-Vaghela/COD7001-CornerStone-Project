#ifndef AND_GATE_H
#define AND_GATE_H

#include <core/process.h>
#include <core/signal.h>

#include <string>

class AndGate : public Process {
public:
    AndGate(Signal* a, Signal* b, Signal* y)
        : A(a), B(b), Y(y)
    {
        gateId = "A" + std::to_string(++gateCounter);
    }

    void execute() override {
        Y->set_next(A->get_value() && B->get_value());
    }

    const std::string& getId() const { return gateId; }

private:
    Signal* A;
    Signal* B;
    Signal* Y;

    std::string gateId;

    // shared accross all and gates
    inline static int gateCounter = 0;
};

#endif