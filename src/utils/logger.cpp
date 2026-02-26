#include <iostream>
#include <iomanip>
#include <utils/logger.h>

namespace Logger {

void print_circuit_state(const Circuit& circuit) {

    const int name_width = 10;
    const int value_width = 7;

    std::cout << "\n---------------------------------\n";
    std::cout << "| "
              << std::left << std::setw(name_width) << "Signal"
              << "| "
              << std::setw(value_width) << "Value"
              << "|\n";
    std::cout << "---------------------------------\n";

    for (auto* s : circuit.get_signals()) {
        std::cout << "| "
                  << std::left << std::setw(name_width) << s->get_name()
                  << "| "
                  << std::setw(value_width) << s->get_value()
                  << "|\n";
    }

    std::cout << "---------------------------------\n";
}

}