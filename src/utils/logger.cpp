#include <iostream>
#include <iomanip>
#include <utils/logger.h>

namespace Logger {

void print_circuit_state(const Circuit& circuit, int header) {

    // get the signal lists
    
    const int width = 5;
    const std::vector<Signal*> signals = circuit.get_signals();

    if(header){
        // First print the header
        std::cout << "\n---------------------------------\n";
        for(auto &signal : signals){
            std::cout << "| "  << std::left << std::setw(width) << signal->get_name();
        }
        std::cout << "|\n";
    }
    
    for (auto* s : signals) {
        std::cout << "| " << std::left << std::setw(width) << s->get_value(); 
    }
    std::cout << "|\n";

}

}