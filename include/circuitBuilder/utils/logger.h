#ifndef LOGGER_H
#define LOGGER_H

#include <circuitBuilder/netlist/circuit.h>

namespace Logger {
    void print_circuit_state(const Circuit& circuit);
    void print_circuit(const Circuit& circuit);
    void print_state_header(const Circuit& circuit);
}
#endif