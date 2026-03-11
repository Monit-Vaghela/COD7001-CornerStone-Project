#include <iostream>
#include <iomanip>
#include <circuitBuilder/utils/logger.h>
#include <circuitBuilder/gates/gate.h>

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

void print_circuit(const Circuit& circuit) {
    std::cout << "\n========== CIRCUIT DETAILS ==========\n";


    // std::cout << "\n--- Processes ---\n";
    for (const auto* process : circuit.get_processes()) {
        if(process->getType() == Process::ProcessType::GATE){
            std::cout << "Gate: " << process->getName() << "\n";
            for(const auto* inputs : dynamic_cast<const Gate*>(process)->getInputs()){
                std::cout << "  Input: " << inputs->get_name() << " = " << inputs->get_value() << "\n";
            }
            for(const auto* outputs : dynamic_cast<const Gate*>(process)->getOutputs()){
                std::cout << "  Output: " << outputs->get_name() << " = " << outputs->get_value() << "\n";
            }
        }
    }
}
}