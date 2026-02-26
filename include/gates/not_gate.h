#ifndef NOT_GATE_H
#define NOT_GATE_H

#include <core/process.h>
#include <core/signal.h>

class NotGate : public Process {
public:
    NotGate(Signal* a, Signal* y)
        : A(a), Y(y) {}

    void execute() override {
        Y->set_next(!A->get_value());
    }

private:
    Signal* A;
    Signal* Y;
};

#endif