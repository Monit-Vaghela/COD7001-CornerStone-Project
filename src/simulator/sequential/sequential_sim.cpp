#include <simulator/sequential/sequential_sim.h>
#include <circuitBuilder/utils/logger.h>
#include <vector>
#include <iostream>
#include <unordered_set>

using namespace std;

SequentialSimulator::SequentialSimulator(Circuit* c)
    : Simulator(c) {
        vcd = new VCDWriter("waveSeq.vcd");
        for (auto* s : circuit->get_signals()) vcd->registerSignal(s->get_name());
        vcd->endDefinitions();
    }


void SequentialSimulator::run() {

    for (auto* s : circuit->get_signals()) {
        vcd->dumpValue(s->get_name(), s->get_value());
    }

    int last_time = 0;
    int simulatedEvents = 0;

    while (!event_queue.empty()) {
        // cout << "\n---------------Event Queue------------------\n";
        // event_queue.printEventQueue();
        // cout << simulatedEvents << endl;
        if(simulatedEvents > 100000) {
            cout << "[Simulator / Error] : Stuck in looping Delta Cycles\n";
            return;
        }
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

        std::vector<Event> current_batch;
        // Gather all events with same time & delta
        while (!event_queue.empty() &&
               event_queue.top().time == T &&
               event_queue.top().delta == D) {
            current_batch.push_back(event_queue.pop());
        }

        std::unordered_set<Process*> unique_processes;
        // Execute batch
        for (auto& e : current_batch) {

            if (e.type == Event::SIGNAL_UPDATE) {
                simulatedEvents ++;
                // cout << "Executing Signal Change : " << e.signal->get_name() << endl; 
                e.signal->set_next(e.value);
            }

            else if (e.type == Event::PROCESS_EXEC) {
                if(unique_processes.find(e.process) == unique_processes.end()){
                    simulatedEvents ++;
                    // cout << "Executing Process : " << e.process->getId() << endl;
                    unique_processes.insert(e.process);
                    e.process->execute();
                }
                
            }
        }

        // Commit signal updates
        std::vector<Signal*> changedSignals;

        // cout << "\n--------------Changed Signals--------------------\n";
        for (auto* signal : circuit->get_signals()) {

            bool oldVal = signal->get_value();
            signal->commit();
            bool newVal = signal->get_value();

            if (oldVal != newVal) {
                vcd->dumpValue(signal->get_name(), newVal);
                // cout << signal->get_name() << " Changed from : " << oldVal << " To : " << newVal << endl;
                changedSignals.push_back(signal);
            }
        }

        // Schedule processes sensitive to changed signals
        // cout << "\n----------------Sensitive Processes---------------\n";
        for (auto* signal : changedSignals) {
            // cout << "\tSignal " << signal->get_name() << " Pushed Processes : " << endl;
            for (auto* process : signal->get_sensitive_list()) {
                // cout << "\t\t" << process->getName() << ", (T, D) : " << T << ", " << D + 1 << endl; 
                
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
        cout << endl; 
        cout << "Sequential Simulated " << simulatedEvents << endl;
    }

    delete vcd;
    vcd = nullptr;
}

void SequentialSimulator::stabilize() {
    int stop = 0;
    // Schedule all gates at T=0, D=0
    for (auto* process : circuit->get_processes()) {
        if(process->getType() == Process::ProcessType::GATE) event_queue.push(Event(0, 0, process));
    }

    // Drain events but stop advancing real time, only delta cycles
    while (!event_queue.empty()) {
        if(stop > 100000) {
            cout << "[Simulator / Error] Cant stabalize circuit. check inputs\n";
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

    vcd->dumpTime(0);

    for (auto* s : circuit->get_signals()) vcd->dumpValue(s->get_name(), s->get_value());
    
}
