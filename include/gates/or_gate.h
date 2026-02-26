#ifndef OR_GATE_H
#define OR_GATE_H

#include <core/signal.h>
#include <core/process.h>

class OrGate : public Process {
public:
    OrGate(Signal* a, Signal* b, Signal* y)
        : A(a), B(b), Y(y) {}

    void execute() override { 
        Y->set_next(A->get_value() || B->get_value());
    }

private:
    Signal* A;
    Signal* B;
    Signal* Y;
};

#endif