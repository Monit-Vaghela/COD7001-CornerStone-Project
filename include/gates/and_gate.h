#ifndef AND_GATE_H
#define AND_GATE_H

#include <core/process.h>
#include <core/signal.h>

class AndGate : public Process {
public:
    AndGate(Signal* a, Signal* b, Signal* y)
        : A(a), B(b), Y(y) {}

    void execute() override {
        Y->set_next(A->get_value() && B->get_value());
    }
private:
    Signal* A;
    Signal* B;
    Signal* Y;
};

#endif