#include <iostream>
#include <circuitBuilder/jsonParser.h>
#include <circuitBuilder/core/signal.h>
#include <circuitBuilder/gates/gate.h>
#include <circuitBuilder/netlist/circuit.h>
#include <circuitBuilder/utils/logger.h>
#include <simulator/sequential/sequential_sim.h>
#include <simulator/parallel/v1.h>
#include <chrono>

// namespaces used
using namespace std;
using namespace std::chrono;


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
        if(sm.signalType == "input")
            s = new Signal(sm.internalSignal, SignalType::INPUT, false);
        else
            s = new Signal(sm.internalSignal, SignalType::OUTPUT, false);

        circuit->add_signal(s);
    }

    // Create intermediate signals
    for (const auto& inter: parser.intermediates){
        Signal* s = new Signal(inter, SignalType::INTERMEDIATE, false);
        circuit->add_signal(s);
    }

    // Create gates
    for (auto& behaviour : parser.behaviours){

        GateType type = stringToGateType(behaviour.gatesUsed[0]);

        std::vector<Signal*> inputs;

        for(auto& in : behaviour.gateExpressionInputs){
            Signal* s = circuit->get_signal(in);

            if(!s){
                cout << "[Circuit Builder / Error] Input signal not found: "
                     << in << endl;
                return -1;
            }

            inputs.push_back(s);
        }

        Signal* out = circuit->get_signal(behaviour.outputSignal);

        if(!out){
            cout << "[Circuit Builder / Error] Output signal not found: "
                 << behaviour.outputSignal << endl;
            return -1;
        }

        Gate* gate = new Gate(type, inputs, out);

        circuit->add_process(gate);

        for(auto* s : inputs)
            s->add_sensitive(gate);
    }

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
    
    SequentialSimulator seqsim(&circuit);
    ParallelV1Simulator parSim(&circuit);


    // SEQUENTIAL EXECUTION

    // first to need one intital simulation step in order to 
    // correctly set the intermediatory and outputs
    // schedule initial evaluation
    for (auto* process : circuit.get_processes()) {
        seqsim.schedule(process, 0, 0);
    }

    // now we need to get the user defined processes
    addProcesses(&seqsim, &circuit, parser);

    Logger::print_state_header(circuit);

    auto start = high_resolution_clock::now();
    seqsim.run();
    auto stop = high_resolution_clock::now();
    duration<double> seqDuration = stop - start;


    // PARALLEL EXECUTION
    for (auto* process : circuit.get_processes()) {
        parSim.schedule(process, 0, 0);
    }

    // now we need to get the user defined processes
    addProcesses(&parSim, &circuit, parser);

    Logger::print_state_header(circuit);

    start = high_resolution_clock::now();
    parSim.run();
    stop = high_resolution_clock::now();
    duration<double> parDuration = stop - start;


    // FINAL LOGS
    cout << "Sequential Execution time: " << seqDuration.count() << " seconds" << endl;
    cout << "Parallel Execution time: " << parDuration.count() << " seconds" << endl;

    cout << "Speed Up: " << parDuration.count() / seqDuration.count() << endl;
    return 0;
}
