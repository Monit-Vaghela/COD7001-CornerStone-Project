#ifndef LOGGER_H
#define LOGGER_H

#include <netlist/circuit.h>

namespace Logger {
    void print_circuit_state(const Circuit& circuit);
}

#endif