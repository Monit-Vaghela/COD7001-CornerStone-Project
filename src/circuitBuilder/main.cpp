#include <iostream>
#include <circuitBuilder/jsonParser.h>
#include <circuitBuilder/core/signal.h>
#include <circuitBuilder/gates/and_gate.h>
#include <circuitBuilder/gates/or_gate.h>
#include <circuitBuilder/gates/not_gate.h>
#include <circuitBuilder/gates/xor_gate.h>
#include <circuitBuilder/netlist/circuit.h>
#include <circuitBuilder/utils/logger.h>

using namespace std;

void printCircuitData(JsonParser& parser) {
    cout << "========== CIRCUIT DATA ==========\n";

    cout << "\n--- Entity Metadata ---\n";
    for (const auto& pair : parser.entityMetadata) {
        cout << pair.first << " : " << pair.second << endl;
    }

    cout << "\n--- Inputs ---\n";
    for (const auto& input : parser.inputs) {
        cout << input << endl;
    }

    cout << "\n--- Outputs ---\n";
    for (const auto& output : parser.outputs) {
        cout << output << endl;
    }

    cout << "\n--- Architecture Metadata ---\n";
    for (const auto& pair : parser.architectureMetadata) {
        cout << pair.first << " : " << pair.second << endl;
    }

    cout << "\n--- Signal Mapping ---\n";
    for (const auto& sm : parser.signalMappings) {
        cout << sm.internalSignal << " -> "
             << sm.mappedSignal
             << " (" << sm.signalType << ")"
             << endl;
    }

    cout << "\n--- Behaviours ---\n";
    int i = 1;
    for (const auto& beh : parser.behaviours) {

        cout << "Behaviour " << i++ << ":\n";

        cout << "Output Signal: " << beh.outputSignal << endl;

        cout << "Inputs: ";
        for (const auto& in : beh.gateExpressionInputs) {
            cout << in << " ";
        }
        cout << endl;

        cout << "Gates Used: ";
        for (const auto& gate : beh.gatesUsed) {
            cout << gate << " ";
        }
        cout << endl;

        cout << "Expression: " << beh.expression << endl;

        cout << endl;
    }
}

void printSimulationData(JsonParser& parser) {
    cout << "\n========== SIMULATION DATA ==========\n";

    cout << "\n--- Simulation Metadata ---\n";
    for (const auto& pair : parser.simulationMetadata) {
        cout << pair.first << " : " << pair.second << endl;
    }

    cout << "\n--- Simulation Processes ---\n";
    for (const auto& timeEntry : parser.simulationProcesses) {
        cout << "Timestamp " << timeEntry.first << ":\n";
        for (const auto& signal : timeEntry.second) {
            cout << "   " << signal.first << " = " << signal.second << endl;
        }
    }
}

// int main(int argc, char* argv[]) {

//     if (argc != 3) {
//         cout << "Usage: " << argv[0] << " <circuit_file.json> <simulation_file.json>\n";
//         return 1;
//     }

//     string circuitFile = argv[1];
//     string simulationFile = argv[2];

//     JsonParser parser;

//     if (!parser.parseCircuitFile(circuitFile)) {
//         cout << "Failed to parse circuit file.\n";
//         return 1;
//     }

//     if (!parser.parseSimulationFile(simulationFile)) {
//         cout << "Failed to parse simulation file.\n";
//         return 1;
//     }

//     printCircuitData(parser);
//     printSimulationData(parser);

//     return 0;
// }

int main() {
    Circuit circuit;

    // Inputs (for Or gate)
    Signal x("x", false);
    Signal y("y", false);
    Signal z("z", false);
    // Internal wire
    Signal Y1("Y1", true);
    Signal Y2("Y2", true);
    Signal Y3("Y3", true);

    // Output
    Signal mus("mus", false);
    Signal yrrac("yrrac", false);

    // Register signals
    circuit.add_signal(&x);
    circuit.add_signal(&y);
    circuit.add_signal(&z);
    circuit.add_signal(&Y1);
    circuit.add_signal(&Y2);
    circuit.add_signal(&Y3);
    circuit.add_signal(&mus);
    circuit.add_signal(&yrrac);

    // Create gates
    XorGate XxorBY(&x, &y, &Y1);
    XorGate Y1xorZ(&Y1, &z, &mus);
    AndGate Y1andZ(&Y1, &z, &Y2);
    AndGate Y1orY2(&x, &y, &Y3);
    OrGate Y2orY3(&Y2, &Y3, &yrrac);

    // Register processes in circuit
    circuit.add_process(&XxorBY);
    circuit.add_process(&Y1xorZ);
    circuit.add_process(&Y1andZ);
    circuit.add_process(&Y1orY2);
    circuit.add_process(&Y2orY3); 
    

    // Setup sensitivity
    x.add_sensitive(&XxorBY);
    y.add_sensitive(&XxorBY);
    Y1.add_sensitive(&Y1xorZ);
    Y1.add_sensitive(&Y1andZ);
    z.add_sensitive(&Y1andZ);
    z.add_sensitive(&Y1andZ);
    Y2.add_sensitive(&Y2orY3);
    Y3.add_sensitive(&Y2orY3);


    Logger::print_circuit(circuit);
}