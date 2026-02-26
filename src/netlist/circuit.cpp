#include <netlist/circuit.h>

void Circuit::add_signal(Signal* s) {
    signals.push_back(s);
}

void Circuit::add_process(Process* p) {
    processes.push_back(p);
}

const std::vector<Signal*>& Circuit::get_signals() const {
    return signals;
}

const std::vector<Process*>& Circuit::get_processes() const {
    return processes;
}

void Circuit::commit_all_signals() {
    for (auto* s : signals) {
        s->commit();
    }
}