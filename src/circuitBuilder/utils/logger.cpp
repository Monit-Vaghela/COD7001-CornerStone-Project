#include <iostream>
#include <iomanip>
#include <circuitBuilder/utils/logger.h>
#include <circuitBuilder/gates/gate.h>

namespace Logger {
void print_circuit_state(const Circuit& circuit, int header) {

    // get the signal lists
    
    const int width = 5;
    const std::vector<Signal*> signals = circuit.get_signals();
    
    // Print only the input
    for (auto* s : signals) {
        if(s->get_type() == SignalType::INPUT){
            std::cout << "| " << std::left << std::setw(width) << s->get_value();
        } 
    }
    // print the intermediates
    for (auto* s : signals) {
        if(s->get_type() == SignalType::INTERMEDIATE){
            std::cout << "| " << std::left << std::setw(width) << s->get_value();
        } 
    }

    // Print only the output
    for (auto* s : signals) {
        if(s->get_type() == SignalType::OUTPUT){
            std::cout << "| " << std::left << std::setw(width) << s->get_value();
        } 
    }
    std::cout << "|\n";

}

void print_circuit(const Circuit& circuit) {

    std::cout << "\n========== CIRCUIT DETAILS ==========\n";

    //------------------------------------
    // Signals (compact view)
    //------------------------------------
    std::cout << "\n--- Signals ---\n";

    for (const auto* signal : circuit.get_signals()) {

        std::cout << "Signal: "
                  << signal->get_name()
                  << " = " << signal->get_value()
                  << " | Sensitive: ";

        const auto& sens_list = signal->get_sensitive_list();

        if (sens_list.empty()) {
            std::cout << "(none)";
        } else {
            for (size_t i = 0; i < sens_list.size(); ++i) {
                std::cout << sens_list[i]->getName();
                if (i != sens_list.size() - 1)
                    std::cout << ", ";
            }
        }

        std::cout << "\n";
    }

    //------------------------------------
    // Processes (compact view)
    //------------------------------------
    std::cout << "\n--- Processes ---\n";

    for (const auto* process : circuit.get_processes()) {

        std::cout << "\nProcess: " << process->getName();

        if (process->getType() == Process::ProcessType::GATE) {

            const Gate* gate = dynamic_cast<const Gate*>(process);

            // Inputs
            std::cout << " | Inputs: ";
            const auto& inputs = gate->getInputs();

            if (inputs.empty()) {
                std::cout << "(none)";
            } else {
                for (size_t i = 0; i < inputs.size(); ++i) {
                    std::cout << inputs[i]->get_name()
                              << "=" << inputs[i]->get_value();
                    if (i != inputs.size() - 1)
                        std::cout << ", ";
                }
            }

            // Outputs
            std::cout << " | Outputs: ";
            const auto& outputs = gate->getOutputs();

            if (outputs.empty()) {
                std::cout << "(none)";
            } else {
                for (size_t i = 0; i < outputs.size(); ++i) {
                    std::cout << outputs[i]->get_name()
                              << "=" << outputs[i]->get_value();
                    if (i != outputs.size() - 1)
                        std::cout << ", ";
                }
            }
        }

        std::cout << "\n";
    }

    std::cout << "\n=====================================\n";
}

void print_state_header(const Circuit& circuit){
    const int width = 5;
    // First print the header
    std::cout << "\n---------------------------------\n";
    for(auto &signal : circuit.get_signals()){
        // Inputs header
        if(signal->get_type() == SignalType::INPUT) {
            std::cout << "| "  << std::left << std::setw(width) << signal->get_name();
        }

        // Intermediate
        if(signal->get_type() == SignalType::INTERMEDIATE) {
            std::cout << "| "  << std::left << std::setw(width) << signal->get_name();
        }

        // Outputs
        if(signal->get_type() == SignalType::OUTPUT) {
            std::cout << "| "  << std::left << std::setw(width) << signal->get_name();
        }
        
    }
    std::cout << "|\n";
}

}