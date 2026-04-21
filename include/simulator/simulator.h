#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <simulator/event_queue.h>
#include <circuitBuilder/core/signal.h>
#include <circuitBuilder/netlist/circuit.h>
#include <simulator/vcd_writer.h>
#include <simulator/tracer.h>

class Simulator {
public:
    Simulator(Circuit* circuit);
    virtual ~Simulator() = default;

    void schedule(Process* p, int time, int delta);
    void apply_input(Signal* s, bool value, int time);

    virtual void run() = 0; 

    int get_current_time() const;
    int get_current_delta() const;
    VCDWriter* vcd;
    TraceWriter* tracer;

protected:
    Circuit* circuit;
    EventQueue event_queue;

    int current_time;
    int current_delta;
};

#endif