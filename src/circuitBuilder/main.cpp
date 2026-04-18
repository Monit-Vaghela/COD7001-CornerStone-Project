#include <iostream>
#include <circuitBuilder/jsonParser.h>
#include <circuitBuilder/core/signal.h>
#include <circuitBuilder/gates/gate.h>
#include <circuitBuilder/netlist/circuit.h>
#include <circuitBuilder/utils/logger.h>
#include <simulator/sequential/sequential_sim.h>
#include <simulator/parallel/v1.h>
#include <simulator/parallel/v2.h>
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
        if(behaviour.processType == "Gate"){
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
        else if (behaviour.processType == "Behaviour") {

            Signal* clk = circuit->get_signal(behaviour.clockSignal);
            Signal* in  = circuit->get_signal(behaviour.inputSignal);
            Signal* out = circuit->get_signal(behaviour.outputSignal);

            if (!clk || !in || !out) {
                cout << "[Error] Behaviour signals missing\n";
                return -1;
            }

            EdgeType edge =
                (behaviour.edgeType == "rising")
                ? EdgeType::RISING
                : EdgeType::FALLING;

            Behaviour* b =
                new Behaviour(clk, edge, in, out);

            circuit->add_process(b);

            clk->add_sensitive(b);
        }
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

    // CircuitGraph circuitGraph;
    // circuitGraph.build(seqCircuit);
    // circuitGraph.applyTarjans();
    // circuitGraph.printGraph();

    // CondensedGraph condensedGraph;
    // condensedGraph.build(circuitGraph);
    // condensedGraph.printGraph();

    // vector<bitset<maxN>> reachabilityMatrix(condensedGraph.nodes.size());
    // condensedGraph.getReachability(reachabilityMatrix);
    // condensedGraph.printReachabilityList(reachabilityMatrix);
    
    // SEQUENTIAL EXECUTION

    // first to need one intital simulation step in order to 
    // correctly set the intermediatory and outputs
    // schedule initial evaluation
    // Logger::print_state_header(seqCircuit);
    // Logger::print_circuit_state(seqCircuit);
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
    
    // Logger::print_circuit(seqCircuit);
    
    SequentialSimulator seqsim(&seqCircuit);

    cout << "\n------------SEQUENTIAL EXECUTION----------------\n";
    // Logger::print_state_header(seqCircuit);
    // Logger::print_circuit_state(seqCircuit);
    seqsim.stabilize();


    // SEQUENTIAL EXECUTION
    addProcesses(&seqsim, &seqCircuit, parser);
    
    auto start = high_resolution_clock::now();
    seqsim.run();
    auto stop = high_resolution_clock::now();
    duration<double> seqDuration = stop - start;

    cout << "\n----------- PARALLEL EXECUTION----------------\n";
    CircuitGraph circuitGraph;
    circuitGraph.build(parCircuit);
    circuitGraph.applyTarjans();
    // circuitGraph.printGraph();

    CondensedGraph condensedGraph;
    condensedGraph.build(circuitGraph);
    condensedGraph.assignLevels();
    // condensedGraph.printGraph();

    vector<bitset<maxN>> reachabilityMatrix(condensedGraph.nodes.size());
    condensedGraph.getReachability(reachabilityMatrix);
    // condensedGraph.printReachabilityList(reachabilityMatrix);
    ParallelV2Simulator parSim(&parCircuit, &circuitGraph, &condensedGraph);
    // Logger::print_state_header(parCircuit);
    // Logger::print_circuit_state(parCircuit);
    parSim.stabilize();
    // cout << "STABALIZED\n";
    // PARALLEL EXECUTION
    addProcesses(&parSim, &parCircuit, parser);

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
    Signal D("D",   SignalType::INPUT,  false);
    Signal E("E", SignalType::INPUT, true);
    Signal y1("y1", SignalType::INTERMEDIATE, false);
    Signal y2("y2", SignalType::INTERMEDIATE, false);
    Signal y3("y3", SignalType::INTERMEDIATE, false);
    Signal y4("y4", SignalType::INTERMEDIATE, false);
    Signal Deff("Deff", SignalType::INTERMEDIATE, false);
    Signal CLK("CLK", SignalType::INPUT, false);
    Signal Q("Q",   SignalType::OUTPUT, false);

    Gate andGate1(GateType::AND, {&D, &E}, {&y1});
    Gate notGate1(GateType::NOT, {&E}, {&y2});
    Gate passGate1(GateType::PASS, {&Q}, {&y3});
    Gate andGate2(GateType::AND, {&y2, &y3}, {&y4});
    Gate orGate1(GateType::OR, {&y1, &y4}, {&Deff});
    Behaviour dff(&CLK, EdgeType::RISING, &Deff, &Q);


    c.add_signal(&D);
    c.add_signal(&E);
    c.add_signal(&y1);
    c.add_signal(&y2);
    c.add_signal(&y3);
    c.add_signal(&y4);
    c.add_signal(&Deff);
    c.add_signal(&CLK);
    c.add_signal(&Q);

    c.add_process(&dff);
    c.add_process(&andGate1);
    c.add_gates(&andGate1);
    c.add_process(&notGate1);
    c.add_gates(&notGate1);
    c.add_process(&passGate1);
    c.add_gates(&passGate1);
    c.add_process(&andGate2);
    c.add_gates(&andGate2);
    c.add_process(&orGate1);
    c.add_gates(&orGate1);

    D.add_sensitive(&andGate1);
    E.add_sensitive(&andGate1);
    E.add_sensitive(&notGate1);
    y1.add_sensitive(&orGate1);
    y2.add_sensitive(&andGate2);
    Q.add_sensitive(&passGate1);
    y3.add_sensitive(&andGate2);
    y4.add_sensitive(&orGate1);
    CLK.add_sensitive(&dff);


    Logger::print_circuit(c);
    Logger::print_state_header(c);

    SequentialSimulator seqsim(&c);
    seqsim.stabilize();

    seqsim.apply_input(&D,   0, 1);
    seqsim.apply_input(&CLK, 0, 1);
    seqsim.apply_input(&D,   0, 2);
    seqsim.apply_input(&CLK, 1, 2);  // rising edge, D=0 → Q=0
    seqsim.apply_input(&CLK, 1, 3);
    seqsim.apply_input(&D,   1, 4);
    seqsim.apply_input(&CLK, 0, 4);  // rising edge, D=1 → Q=1
    seqsim.apply_input(&CLK, 1, 5);
    seqsim.apply_input(&E, 0, 6);
    seqsim.apply_input(&CLK, 0, 6);
    seqsim.apply_input(&D, 0, 7);
    seqsim.apply_input(&CLK, 1, 7);
    seqsim.apply_input(&E, 1, 8);
    seqsim.apply_input(&CLK, 0, 8);
    seqsim.apply_input(&CLK, 1, 9);
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