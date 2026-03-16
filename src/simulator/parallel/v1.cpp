#include <simulator/parallel/v1.h>
#include <circuitBuilder/utils/logger.h>
#include <vector>

ParallelV1Simulator::ParallelV1Simulator(Circuit* c)
    : Simulator(c) {}

void ParallelV1Simulator::run() {

    int last_time = -1;

    while (!event_queue.empty()) {

        int T = event_queue.top().time;
        int D = event_queue.top().delta;

        // Logging when time advances
        if (last_time != -1 && T != last_time) {
            Logger::print_circuit_state(*circuit, 0);
        }

        current_time = T;
        current_delta = D;
        last_time = T;

        std::vector<Event> current_batch;

        // Gather all events with same time & delta
        while (!event_queue.empty() &&
               event_queue.top().time == T &&
               event_queue.top().delta == D) {

            current_batch.push_back(event_queue.pop());
        }

        // Execute batch
        for (auto& e : current_batch) {

            if (e.type == Event::SIGNAL_UPDATE) {
                e.signal->set_next(e.value);
            }

            else if (e.type == Event::PROCESS_EXEC) {
                e.process->execute();
            }
        }

        // Commit signal updates
        std::vector<Signal*> changedSignals;

        for (auto* signal : circuit->get_signals()) {

            bool oldVal = signal->get_value();
            signal->commit();
            bool newVal = signal->get_value();

            if (oldVal != newVal) {
                changedSignals.push_back(signal);
            }
        }

        // Schedule processes sensitive to changed signals
        for (auto* signal : changedSignals) {

            for (auto* process : signal->get_sensitive_list()) {

                event_queue.push(
                    Event(T, D + 1, process)
                );
            }
        }
    }

    // Final state print
    if (last_time != -1) {
        Logger::print_circuit_state(*circuit, 0);
    }
}