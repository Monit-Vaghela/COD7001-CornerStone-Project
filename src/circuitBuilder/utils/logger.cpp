#include <iostream>
#include <iomanip>
#include <circuitBuilder/utils/logger.h>
#include <circuitBuilder/gates/gate.h>

namespace Logger {
void print_circuit_state(const Circuit& circuit) {

    // get the signal lists
    
    const int width = 5;
    const std::vector<Signal*> signals = circuit.get_signals();
    
    for (auto* s : signals) 
        if(s->get_type() == SignalType::INPUT){
            if(s->initialized) std::cout << "| " << std::left << std::setw(width) << s->get_value(); 
            else std::cout << "| " << std::left << std::setw(width) << s->get_value(); 
        }

    // for (auto* s : signals) 
    //     if(s->get_type() == SignalType::INTERMEDIATE){
    //         if(s->initialized) std::cout << "| " << std::left << std::setw(width) << s->get_value(); 
    //         else std::cout << "| " << std::left << std::setw(width) << "U"; 
    //     }

    for (auto* s : signals) 
        if(s->get_type() == SignalType::OUTPUT){
            if(s->initialized) std::cout << "| " << std::left << std::setw(width) << s->get_value();
            else std::cout << "| " << std::left << std::setw(width) << s->get_value(); 
        }
    std::cout << "|\n";

}

void print_circuit(const Circuit& circuit) {

    std::cout << "\n========== CIRCUIT DETAILS ==========\n";

    //------------------------------------
    // Print Signals
    //------------------------------------
    std::cout << "\n--- Signals ---\n";

    for (const auto* signal : circuit.get_signals()) {

        std::cout << "Signal: " << signal->get_name()
                  << " = " << (signal->initialized ? std::to_string(signal->get_value()) : "U")
                  << "\n";

        std::cout << "  Sensitive Processes:\n";

        const auto& sens_list = signal->get_sensitive_list();

        if (sens_list.empty()) {
            std::cout << "    (none)\n";
        } else {
            for (const auto* proc : sens_list) {
                std::cout << "    " << proc->getName() << "\n";
            }
        }
    }

    //------------------------------------
    // Print Processes
    //------------------------------------
    std::cout << "\n--- Processes ---\n";

    for (const auto* process : circuit.get_processes()) {

        std::cout << "\nProcess: " << process->getName() << "\n";

        if (process->getType() == Process::ProcessType::GATE) {

            const Gate* gate = dynamic_cast<const Gate*>(process);

            std::cout << "  Inputs:\n";
            for (const auto* input : gate->getInputs()) {
                std::cout << "    "
                          << input->get_name()
                          << " = "
                          << (input->initialized ? std::to_string(input->get_value()) : "U")
                          << "\n";
            }

            std::cout << "  Outputs:\n";
            for (const auto* output : gate->getOutputs()) {
                std::cout << "    "
                          << output->get_name()
                          << " = "
                          << (output->initialized ? std::to_string(output->get_value()) : "U")
                          << "\n";
            }
        }
    }

    std::cout << "\n=====================================\n";
}

void print_state_header(const Circuit& circuit){
    const int width = 5;
    // First print the header
    std::cout << "\n---------------------------------\n";
    for(auto &signal : circuit.get_signals()){
        if(signal->get_type() == SignalType::INPUT)
        std::cout << "| "  << std::left << std::setw(width) << signal->get_name();
    }
    // for(auto &signal : circuit.get_signals()){
    //     if(signal->get_type() == SignalType::INTERMEDIATE)
    //     std::cout << "| "  << std::left << std::setw(width) << signal->get_name();
    // }

    for(auto &signal : circuit.get_signals()){
        if(signal->get_type() == SignalType::OUTPUT)
        std::cout << "| "  << std::left << std::setw(width) << signal->get_name();
    }
    std::cout << "|\n";
}
}