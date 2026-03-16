#include <simulator/simulator.h>

Simulator::Simulator(Circuit* c)
    : circuit(c), current_time(0), current_delta(0) {}

void Simulator::schedule(Process* p, int time, int delta) {
    event_queue.push(Event(time, delta, p));
}

void Simulator::apply_input(Signal* s, bool value, int time) {
    event_queue.push(Event(time, 0, s, value));
}

int Simulator::get_current_time() const {
    return current_time;
}

int Simulator::get_current_delta() const {
    return current_delta;
}