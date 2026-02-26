#include "core/simulator.h"
#include <unordered_map>
#include <utils/logger.h>

Simulator::Simulator(Circuit* c)
    : circuit(c), current_time(0), current_delta(0) {}

void Simulator::schedule(Process* p, int time, int delta) {
    event_queue.push(Event(time, delta, p));
}

void Simulator::apply_input(Signal* s, bool value, int time) {
    event_queue.push(Event(time, 0, s, value));
}

void Simulator::run() {
     int last_time = -1;

    while (!event_queue.empty()) {

        int T = event_queue.top().time;
        int D = event_queue.top().delta;

        // Just for logging
        if (last_time != -1 && T != last_time) {
            Logger::print_circuit_state(*circuit, 0);
        }

        current_time = T;
        current_delta = D;
        last_time = T;

        std::vector<Event> current_batch;

        /*
        Each event belonging to same T are considered
        of same batch. we get all the elements from the 
        top of the queue and push them back here.
        */
        while (!event_queue.empty() &&
               event_queue.top().time == T &&
               event_queue.top().delta == D) {

            current_batch.push_back(event_queue.pop());
        }

        // Go through the current batch to execute the
        // updates
        for (auto& e : current_batch) {

            if (e.type == Event::SIGNAL_UPDATE) {
                e.signal->set_next(e.value);
            }

            else if (e.type == Event::PROCESS_EXEC) {
                e.process->execute();
            }
        }

        // Commit all the changes
        // This will create a ripple effect which can be stored 
        // in changedSignals
        std::vector<Signal*> changedSignals;

        for (auto* signal : circuit->get_signals()) {

            bool oldVal = signal->get_value();
            signal->commit();
            bool newVal = signal->get_value();

            if (oldVal != newVal) {
                changedSignals.push_back(signal);
            }
        }

        // All the changed signals need more processing
        // we push them back into the event queue.
        for (auto* signal : changedSignals) {

            for (auto* process : signal->get_sensitive_list()) {

                event_queue.push(
                    Event(T, D + 1, process)
                );
            }
        }
    }
    // Print final time after queue empties
    if (last_time != -1) {
        Logger::print_circuit_state(*circuit, 0);
    }
}

int Simulator::get_current_time() const {
    return current_time;
}

int Simulator::get_current_delta() const {
    return current_delta;
}