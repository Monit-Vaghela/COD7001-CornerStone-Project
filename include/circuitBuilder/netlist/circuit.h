#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <vector>
#include <circuitBuilder/core/signal.h>
#include <circuitBuilder/core/process.h>
#include <circuitBuilder/gates/gate.h>

class Circuit {
public:
    void add_signal(Signal* s);
    void add_process(Process* p);
    void add_gates(Gate* g);

    const std::vector<Signal*>& get_signals() const;
    const std::vector<Process*>& get_processes() const;
    const std::vector<Gate*>& get_gates() const;

    Signal* get_signal(const std::string& name);
    void commit_all_signals();

private:
    std::vector<Signal*> signals;
    std::vector<Process*> processes;
    std::vector<Gate*> gates;
};

#endif