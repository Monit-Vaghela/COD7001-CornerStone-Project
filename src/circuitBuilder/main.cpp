#include <iostream>
#include <circuitBuilder/jsonParser.h>
#include <circuitBuilder/core/signal.h>
#include <circuitBuilder/gates/and_gate.h>
#include <circuitBuilder/gates/or_gate.h>
#include <circuitBuilder/gates/not_gate.h>
#include <circuitBuilder/gates/xor_gate.h>
#include <circuitBuilder/netlist/circuit.h>
#include <circuitBuilder/utils/logger.h>
#include <simulator/simulator.h>

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

    cout << "\n--- Intermediates ---\n";
    for (const auto& output : parser.intermediates) {
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

int buildCircuitFromJson(Circuit* circuit, JsonParser& parser) {

    // First create input and output Signals
    for (const auto& sm : parser.signalMappings) {
        Signal *s;
        if(sm.signalType == "input") s = new Signal(sm.internalSignal, SignalType::INPUT, false); // default initial value
        else  s = new Signal(sm.internalSignal, SignalType::OUTPUT, false); // default initial value
        (*circuit).add_signal(s);
    }

    // now lets create and add intermediate inputs
    for (const auto& inter: parser.intermediates){
        Signal* s = new Signal(inter, SignalType::INTERMEDIATE, false);
        (*circuit).add_signal(s);
    }

    // Create gates based on behaviours
    for (auto& behaviour : parser.behaviours){
        if (behaviour.gatesUsed[0] != "not"){
            // get its inputs
            Signal *a = (*circuit).get_signal(behaviour.gateExpressionInputs[0]);
            Signal *b = (*circuit).get_signal(behaviour.gateExpressionInputs[1]);

            // get its output
            cout << "C is : " << behaviour.outputSignal << endl;
            Signal *c = (*circuit).get_signal(behaviour.outputSignal);
            // if(a){
            //     cout << "Got a" << endl;
            // }
            // if(b){
            //     cout << "Got b" << endl;
            // }
            // if(c){
            //     cout << "Got c" << endl;
            // }
            if(a && b && c){
                if(behaviour.gatesUsed[0] == "and"){
                    // Create and add gate
                    AndGate *andGate = new AndGate(a, b, c);
                    (*circuit).add_process(andGate);

                    // Add the gate to sensitivity of inputs
                    a->add_sensitive(andGate);
                    b->add_sensitive(andGate);
                }
                else if(behaviour.gatesUsed[0] == "or"){
                    OrGate *orGate = new OrGate(a, b, c);
                    (*circuit).add_process(orGate);

                    a->add_sensitive(orGate);
                    b->add_sensitive(orGate);
                }
                else if(behaviour.gatesUsed[0] == "xor"){
                    XorGate *xorGate = new XorGate(a, b, c);
                    (*circuit).add_process(xorGate);

                    a->add_sensitive(xorGate);
                    b->add_sensitive(xorGate);
                }
                else if(behaviour.gatesUsed[0] == "xnor"){
                }
            } else {
                cout << "[Circuit Builder / Error] couldnt find specified signals" << endl; 
                return -1;
            }
        }
        else{
            // only a single input
            Signal *a = (*circuit).get_signal(behaviour.gateExpressionInputs[0]);

            // output
            Signal *c = (*circuit).get_signal(behaviour.outputSignal);

            if(a && c){
                NotGate *notGate = new NotGate(a, c);
                (*circuit).add_process(notGate);

                a->add_sensitive(notGate);
            }
        }
    }

    //Logger::print_circuit(*circuit);

    // Logger::print_circuit_state(*circuit, 1);
    return 0;
}

int addProcesses(Simulator* sim, Circuit* circuit, JsonParser& parser){
    for (const auto& timeEntry : parser.simulationProcesses) {
        for (const auto& signal : timeEntry.second) {
            Signal *s = (*circuit).get_signal(signal.first);
            sim->apply_input(s, (bool)signal.second, (int)timeEntry.first);
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <circuit_file.json> <simulation_file.json>\n";
        return 1;
    }

    string circuitFile = argv[1];
    string simulationFile = argv[2];

    JsonParser parser;

    if (!parser.parseCircuitFile(circuitFile)) {
        cout << "Failed to parse circuit file.\n";
        return 1;
    }

    if (!parser.parseSimulationFile(simulationFile)) {
        cout << "Failed to parse simulation file.\n";
        return 1;
    }

    // printCircuitData(parser);
    // printSimulationData(parser);

    Circuit circuit;
    buildCircuitFromJson(&circuit, parser);

    Logger::print_circuit(circuit);
    
    Simulator sim(&circuit);

    // first to need one intital simulation step in order to 
    // correctly set the intermediatory and outputs
    // schedule initial evaluation
    for (auto* process : circuit.get_processes()) {
        sim.schedule(process, 0, 0);
    }

    // now we need to get the user defined processes
    addProcesses(&sim, &circuit, parser);

    Logger::print_state_header(circuit);
    sim.run();
    return 0;
}
