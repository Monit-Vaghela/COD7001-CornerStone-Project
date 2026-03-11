#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <vector>
#include <circuitBuilder/core/signal.h>
#include <circuitBuilder/core/process.h>

class Circuit {
public:
    void add_signal(Signal* s);
    void add_process(Process* p);

    const std::vector<Signal*>& get_signals() const;
    const std::vector<Process*>& get_processes() const;

    void commit_all_signals();

private:
    std::vector<Signal*> signals;
    std::vector<Process*> processes;
};

#endif