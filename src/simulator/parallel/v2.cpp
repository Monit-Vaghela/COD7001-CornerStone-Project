#include <simulator/parallel/v2.h>
#include <circuitBuilder/utils/logger.h>
#include<circuitBuilder/gates/gate.h>
#include <vector>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <omp.h>
#include <map>

using namespace std;

ParallelV2Simulator::ParallelV2Simulator(Circuit* c, CircuitGraph* cir, CondensedGraph* cond)
    : Simulator(c), circuitGraph(cir), condensedGraph(cond){
        vcd = new VCDWriter("wavePar.vcd");
        for (auto* s : circuit->get_signals()) vcd->registerSignal(s->get_name());
        vcd->endDefinitions();
        for (auto* process : circuit->get_processes()) {
            auto it = cir->nodeMap.find(process);
            if (it != cir->nodeMap.end() && it->second != nullptr) {
                processesByLevel[it->second->level].push_back(process);
            }
        }
    }

void ParallelV2Simulator::run() {
    // cout << "STARTIN SIMULATION\n";
    for (auto* s : circuit->get_signals()) {
        vcd->dumpValue(s->get_name(), s->get_value());
    }
    int last_time = 0;
    int simulatedEvents = 0;

    while (!event_queue.empty()) {
        int T = event_queue.top().time;
        int D = event_queue.top().delta;

        // Logging when time advances
        if (last_time != -1 && T != last_time) {
            // Logger::print_circuit_state(*circuit);
        }

        current_time = T;
        current_delta = D;
        if (T != last_time) {
            vcd->dumpTime(T);
            last_time = T;
        }

        // Collect all events at this (T, D)
        // cout << "CREATING CURRENT BATCH\n";
        std::vector<Event> current_batch;
        while (!event_queue.empty() &&
               event_queue.top().time == T &&
               event_queue.top().delta == D) {
            current_batch.push_back(event_queue.pop());
        }

        // cout << "GROUPING THIS BATCH BY LEVEL\n";
        // Group this batch's events by level.
        // SIGNAL_UPDATE events have no process so they go to level -1
        // and always flush before any PROCESS_EXEC at level 0.
        std::map<int, std::vector<Event*>> byLevel;
        for (auto& e : current_batch) {
            if (e.type == Event::SIGNAL_UPDATE) {
                byLevel[-1].push_back(&e);
                continue;
            }
            auto it = circuitGraph->nodeMap.find(e.process);
            if (it == circuitGraph->nodeMap.end() || it->second == nullptr) {
                byLevel[0].push_back(&e);
                continue;
            }
            int lvl = it->second->level;
            byLevel[lvl].push_back(&e);
        }

        // cout << "EXECUTING LEVEL BY LEVEL\n";
        // Execute level by level — barrier between levels
        for (auto& [level, events] : byLevel) {
            
            #pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < (int)events.size(); i++) {
                Event* e = events[i];
                if (e->type == Event::SIGNAL_UPDATE) {
                    // cout << "UPDATING SIGNAL\n";
                    e->signal->set_next(e->value);
                    // cout << "UPDATED SIGNAL\n";
                } else if (e->type == Event::PROCESS_EXEC) {
                    simulatedEvents++;
                    // cout<< "\tCurrent Event(Process) : " << e->process->getId() << endl;
                    e->process->execute();
                }
            }
            // Implicit OpenMP barrier here — processes at level+1
            // are guaranteed not to start until level is done
        }

        // cout << "COMMITING SIGNALS\n";
        // Commit signal updates
        std::vector<Signal*> changedSignals;

        // // cout<< "\n--------------Changed Signals--------------------\n";
        for (auto* signal : circuit->get_signals()) {

            bool oldVal = signal->get_value();
            signal->commit();
            bool newVal = signal->get_value();

            if (oldVal != newVal) {
                vcd->dumpValue(signal->get_name(), newVal);
                // // cout<< signal->get_name() << " Changed from : " << oldVal << " To : " << newVal << endl;
                changedSignals.push_back(signal);
            }
        }

        // Schedule processes sensitive to changed signals
        // // cout<< "\n----------------Sensitive Processes---------------\n";
        for (auto* signal : changedSignals) {
            // // cout<< "\tSignal " << signal->get_name() << " Pushed Processes : " << endl;
            for (auto* process : signal->get_sensitive_list()) {
                // // cout<< "\t\t" << process->getName() << ", (T, D) : " << T << ", " << D + 1 << endl; 
                // cout<< "\tThis is pusing event(Process) : " << process->getId() << endl;
                event_queue.push(
                    Event(T, D + 1, process)
                );
            }
        }
    }

    // Final state print
    if (last_time != -1) {
        vcd->dumpTime(current_time + 1);
        // Logger::print_circuit_state(*circuit);
        cout<< "Parallel Simulated " << simulatedEvents << endl;
    }

    delete vcd;
    vcd = nullptr;
}

void ParallelV2Simulator::stabilize() {
    int stop = 0;
    // Schedule all gates at T=0, D=0
    for (auto* process : circuit->get_processes()) {
        if(process->getType() == Process::ProcessType::GATE) event_queue.push(Event(0, 0, process));
    }

    // Drain events but stop advancing real time, only delta cycles
    while (!event_queue.empty()) {
        if(stop > 1000) {
            cout<< "[Simulator / Error] Cant stabalize circuit. check inputs\n";
            return;
        }
        int T = event_queue.top().time;
        int D = event_queue.top().delta;

        // Only drain delta cycles at T=0; stop at any real time event
        // (those are user inputs, scheduled separately)
        if (T != 0) break;

        std::vector<Event> batch;
        while (!event_queue.empty() &&
               event_queue.top().time == T &&
               event_queue.top().delta == D) {
            batch.push_back(event_queue.pop());
        }

        for (auto& e : batch) {
            if (e.type == Event::PROCESS_EXEC)
                e.process->execute();
        }

        std::vector<Signal*> changed;
        for (auto* s : circuit->get_signals()) {
            bool old = s->get_value();
            s->commit();
            if (old != s->get_value())
                changed.push_back(s);
        }

        for (auto* s : changed) {
            for (auto* p : s->get_sensitive_list()) {
                event_queue.push(Event(0, D + 1, p));
                stop++;
            }
        }
    }

    for (auto* s : circuit->get_signals()) {
        s->initializeSignal();
    }
}