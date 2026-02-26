#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <core/event_queue.h>
#include <core/signal.h>
#include <netlist/circuit.h>

class Simulator {
public:
    Simulator(Circuit* circuit);

    void schedule(Process* p, int time, int delta);
    void run();
    void apply_input(Signal* s, bool value, int time);

    int get_current_time() const;
    int get_current_delta() const;

private:
    Circuit* circuit;
    EventQueue event_queue;

    int current_time;
    int current_delta;
};

#endif