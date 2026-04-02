#include <iostream>
#include <circuitBuilder/jsonParser.h>
#include <circuitBuilder/core/signal.h>
#include <circuitBuilder/gates/gate.h>
#include <circuitBuilder/netlist/circuit.h>
#include <circuitBuilder/utils/logger.h>
#include <simulator/sequential/sequential_sim.h>
#include <simulator/parallel/v1.h>
#include <chrono>
#include <unordered_map>
#include <preProcessor/circuitGraph.h>
#include <preProcessor/condensedGraph.h>
#include <cstring>
#include <circuitBuilder/core/behaviour.h>


// namespaces used
using namespace std;
using namespace std::chrono;

// GPT Generated
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

//GPT Generated
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

    // lets print out intiialState
    // First create input and output Signals
    for (const auto& sm : parser.signalMappings) {
        Signal *s;
        auto it = parser.initialState.find(sm.internalSignal);
        bool initialValue = (it != parser.initialState.end()) 
        ? (it->second != 0)
        : false;

        if(sm.signalType == "input")
            s = new Signal(sm.internalSignal, SignalType::INPUT, initialValue);
        else
            s = new Signal(sm.internalSignal, SignalType::OUTPUT, initialValue);

        circuit->add_signal(s);
    }

    // Create intermediate signals
    for (const auto& inter: parser.intermediates){
        auto it = parser.initialState.find(inter);
        bool initialValue = (it != parser.initialState.end()) 
        ? (it->second != 0)
        : false;

        Signal* s = new Signal(inter, SignalType::INTERMEDIATE, initialValue);
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
        circuit->add_gates(gate);

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

void runSequential(JsonParser& parser){
    Circuit seqCircuit, parCircuit;
    buildCircuitFromJson(&seqCircuit, parser);
    
    Logger::print_circuit(seqCircuit);
    
    SequentialSimulator seqsim(&seqCircuit);

    CircuitGraph circuitGraph;
    circuitGraph.build(seqCircuit);
    circuitGraph.applyTarjans();
    circuitGraph.printGraph();

    CondensedGraph condensedGraph;
    condensedGraph.build(circuitGraph);
    condensedGraph.printGraph();

    vector<bitset<maxN>> reachabilityMatrix(condensedGraph.nodes.size());
    condensedGraph.getReachability(reachabilityMatrix);
    condensedGraph.printReachabilityList(reachabilityMatrix);
    
    // SEQUENTIAL EXECUTION

    // first to need one intital simulation step in order to 
    // correctly set the intermediatory and outputs
    // schedule initial evaluation
    Logger::print_state_header(seqCircuit);
    Logger::print_circuit_state(seqCircuit);
    seqsim.stabilize();

    
    // now we need to get the user defined processes
    addProcesses(&seqsim, &seqCircuit, parser);
    
    auto start = high_resolution_clock::now();
    seqsim.run();
    auto stop = high_resolution_clock::now();
    duration<double> seqDuration = stop - start;

    return;
}

void runComparisional(JsonParser& parser){
    Circuit seqCircuit, parCircuit;
    buildCircuitFromJson(&seqCircuit, parser);
    buildCircuitFromJson(&parCircuit, parser);
    
    Logger::print_circuit(seqCircuit);
    
    SequentialSimulator seqsim(&seqCircuit);
    ParallelV1Simulator parSim(&parCircuit);


    // SEQUENTIAL EXECUTION

    // first to need one intital simulation step in order to 
    // correctly set the intermediatory and outputs
    // schedule initial evaluation
    for (auto* process : seqCircuit.get_processes()) {
        seqsim.schedule(process, 0, 0);
    }

    // now we need to get the user defined processes
    addProcesses(&seqsim, &seqCircuit, parser);

    Logger::print_state_header(seqCircuit);

    auto start = high_resolution_clock::now();
    seqsim.run();
    auto stop = high_resolution_clock::now();
    duration<double> seqDuration = stop - start;


    // PARALLEL EXECUTION
    for (auto* process : parCircuit.get_processes()) {
        parSim.schedule(process, 0, 0);
    }

    // now we need to get the user defined processes
    addProcesses(&parSim, &parCircuit, parser);

    // Logger::print_state_header(parCircuit);

    start = high_resolution_clock::now();
    parSim.run();
    stop = high_resolution_clock::now();
    duration<double> parDuration = stop - start;


    // FINAL LOGS
    cout << "Sequential Execution time: " << seqDuration.count() << " seconds" << endl;
    cout << "Parallel Execution time: " << parDuration.count() << " seconds" << endl;

    cout << "Speed Up: " << seqDuration.count() / parDuration.count() << endl;
}

void runTemp(){
    Circuit c;

    Signal D("D",     SignalType::INPUT,        false);
    Signal CLK("CLK", SignalType::INPUT,        true);
    Signal y1("y1",   SignalType::INTERMEDIATE, false); // NOT(D)
    Signal y2("y2",   SignalType::INTERMEDIATE, false); // NAND(D,  CLK) - master S
    Signal y3("y3",   SignalType::INTERMEDIATE, false); // NAND(y1, CLK) - master R
    Signal y4("y4",   SignalType::INTERMEDIATE, false); // NAND(y2, y5)  - master Q
    Signal y5("y5",   SignalType::INTERMEDIATE, true); // NAND(y3, y4)  - master Qb
    Signal Q("Q",     SignalType::OUTPUT,       false);
    Signal Qb("Qb",   SignalType::OUTPUT,       true);

    c.add_signal(&D);
    c.add_signal(&CLK);
    c.add_signal(&y1);
    c.add_signal(&y2);
    c.add_signal(&y3);
    c.add_signal(&y4);
    c.add_signal(&y5);
    c.add_signal(&Q);
    c.add_signal(&Qb);

    Gate notGate1 (GateType::NOT,  {&D},        &y1);
    Gate nandGate1(GateType::NAND, {&D,   &CLK}, &y2);
    Gate nandGate2(GateType::NAND, {&y1,  &CLK}, &y3);
    Gate nandGate3(GateType::NAND, {&y2,  &y5},  &y4);
    Gate nandGate4(GateType::NAND, {&y3,  &y4},  &y5);
    Gate nandGate5(GateType::PASS, {&y4},  &Q);
    Gate nandGate6(GateType::PASS, {&y5},   &Qb);

    c.add_gates(&notGate1);
    c.add_gates(&nandGate1);
    c.add_gates(&nandGate2);
    c.add_gates(&nandGate3);
    c.add_gates(&nandGate4);
    c.add_gates(&nandGate5);
    c.add_gates(&nandGate6);

    c.add_process(&notGate1);
    c.add_process(&nandGate1);
    c.add_process(&nandGate2);
    c.add_process(&nandGate3);
    c.add_process(&nandGate4);
    c.add_process(&nandGate5);
    c.add_process(&nandGate6);

    // notGate1: inputs {D}
    D.add_sensitive(&notGate1);

    // nandGate1: inputs {D, CLK}
    D.add_sensitive(&nandGate1);
    CLK.add_sensitive(&nandGate1);

    // nandGate2: inputs {y1, CLK}
    y1.add_sensitive(&nandGate2);
    CLK.add_sensitive(&nandGate2);

    // nandGate3: inputs {y2, y5}
    y2.add_sensitive(&nandGate3);
    y5.add_sensitive(&nandGate3);

    // nandGate4: inputs {y3, y4}
    y3.add_sensitive(&nandGate4);
    y4.add_sensitive(&nandGate4);

    // nandGate5: inputs {y4, Qb}
    y4.add_sensitive(&nandGate5);

    // nandGate6: inputs {y5, Q}
    y5.add_sensitive(&nandGate6);

    Logger::print_circuit(c);
    Logger::print_state_header(c);

    SequentialSimulator seqsim(&c);
    seqsim.stabilize();

    Logger::print_circuit_state(c);

    // CLK=0 baseline
    seqsim.apply_input(&D,   0, 1);
    seqsim.apply_input(&CLK, 0, 1);

    // Rising edge with D=0 → Q should capture 0
    seqsim.apply_input(&D,   0, 2);
    seqsim.apply_input(&CLK, 1, 2);

    // CLK falls, Q holds
    seqsim.apply_input(&CLK, 0, 3);

    // Rising edge with D=1 → Q should capture 1
    seqsim.apply_input(&D,   1, 4);
    seqsim.apply_input(&CLK, 1, 4);

    // CLK falls, Q holds at 1
    seqsim.apply_input(&CLK, 0, 5);
    seqsim.run();
}

int main(int argc, char* argv[]) {

    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <circuit_file.json> <simulation_file.json> -s/-c\n";
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

    if(strcmp(argv[3], "-s") == 0) runSequential(parser);
    else if(strcmp(argv[3], "-c") == 0) runComparisional(parser);
    else if(strcmp(argv[3], "-t") == 0) runTemp();
    return 0;
}