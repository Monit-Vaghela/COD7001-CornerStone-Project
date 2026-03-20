#include <simulator/parallel/v1.h>
#include <circuitBuilder/utils/logger.h>
#include <vector>
#include <iostream>
#include <unordered_set>
#include <algorithm>  // for std::sort
#include <omp.h>

using namespace std;

ParallelV1Simulator::ParallelV1Simulator(Circuit* c)
    : Simulator(c) {}

void ParallelV1Simulator::run() {

    int last_time = -1;
    int simulatedEvents = 0;

    while (!event_queue.empty()) {

        int T = event_queue.top().time;
        int D = event_queue.top().delta;

        // Logging when time advances
        if (last_time != -1 && T != last_time) {
            //Logger::print_circuit_state(*circuit, 0);
        }

        current_time = T;
        current_delta = D;
        last_time = T;

        std::vector<Event> current_batch;

        // Gather all events with same time & delta
        while (!event_queue.empty() &&
               event_queue.top().time == T &&
               event_queue.top().delta == D) {

            //
            Event e = event_queue.top();
            event_queue.pop();
            current_batch.push_back(e);
        }
        sort(current_batch.begin(), current_batch.end(),
            [](const Event& a, const Event& b) {

                // 1. SIGNAL_UPDATE always comes first
                if (a.type != b.type) {
                    return a.type == Event::SIGNAL_UPDATE;
                }

                // 2. If both are PROCESS_EXEC → sort by gate level
                if (a.type == Event::PROCESS_EXEC) {
                    return ((Gate*)a.process)->getLevel() <
                        ((Gate*)b.process)->getLevel();
                }

                // 3. If both are SIGNAL_UPDATE → keep original order
                return false;
            });
        // cout << "(T,D) : " << "(" << current_time << ", " << current_delta << ")" << endl;

        // print current batch
        // for(auto e : current_batch){
        //     if (e.type == Event::SIGNAL_UPDATE) {
        //         cout << "\tSIGNAL_UPDATE -> "
        //             << e.signal->get_name()
        //             << " = " << e.value << endl;
        //     }
        //     else if (e.type == Event::PROCESS_EXEC) {
        //         cout << "\tPROCESS_EXEC -> "
        //             << ((Gate*)e.process)->getName() << "-> "
        //             << ((Gate*)e.process)->getId() << ", Gate Level -> "
        //             << ((Gate*)e.process)->getLevel()
        //             << endl;
        //     }
        // }
        // Execute batch
        /*
        This execution will occur in 2 parts
        1) execute the signal changes
        2) execute the processes
        */
        // 1) signal updates
        int i = 0, n = current_batch.size();
        while (i < n && current_batch[i].type == Event::SIGNAL_UPDATE) {
            current_batch[i].signal->set_next(current_batch[i].value);
            i++;
        }

        // 2) Execute processes level-by-level
        while (i < n) {

            // Safety check
            if (current_batch[i].type != Event::PROCESS_EXEC) {
                i++;
                continue;
            }

            int currentGateLevel =
                ((Gate*)current_batch[i].process)->getLevel();

            int j = i;

            while (j < n &&
                current_batch[j].type == Event::PROCESS_EXEC &&
                ((Gate*)current_batch[j].process)->getLevel() == currentGateLevel) {
                j++;
            }

            // Execute all events in [i, j)
            int work = j - i;
            if (work > 100) {
                #pragma omp parallel for schedule(static)
                for (int k = i; k < j; k++) {
                    current_batch[k].process->execute();
                }
            } else {
                for (int k = i; k < j; k++) {
                    current_batch[k].process->execute();
                }
            }

            // Move to next group
            i = j;
        }

        // 2) processes updates
        // for (auto& e : current_batch) {

        //     // logs
        //     // cout << "[T=" << T 
        //     //     << " | D=" << D 
        //     //     << "] ";

        //     // if (e.type == Event::SIGNAL_UPDATE) {
        //     //     cout << "SIGNAL_UPDATE -> "
        //     //         << e.signal->get_name()
        //     //         << " = " << e.value << endl;
        //     // }
        //     // else if (e.type == Event::PROCESS_EXEC) {
        //     //     cout << "PROCESS_EXEC -> "
        //     //         << ((Gate*)e.process)->getName() << "-> "
        //     //         << ((Gate*)e.process)->getId() << endl;
        //     // }

        //     // for the new approach i need to first execute all the signal updates
        //     // then i will collect gates with same level and execute them in parallel
        //     if (e.type == Event::SIGNAL_UPDATE) {
        //         e.signal->set_next(e.value);
        //     }

        //     else if (e.type == Event::PROCESS_EXEC) {
        //         e.process->execute();
        //     }
        // }

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

        for (auto* signal : changedSignals) {
            for (auto* process : signal->get_sensitive_list()) {
                simulatedEvents ++;
                event_queue.push(Event(T, D + 1, process));

            }
        }
    }

    // Final state print
    if (last_time != -1) {
        //Logger::print_circuit_state(*circuit, 0);
        cout << "V1 Simulated " << simulatedEvents << endl;
    }
}