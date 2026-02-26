#include <core/signal.h>
#include <gates/and_gate.h>
#include <gates/not_gate.h>
#include <gates/or_gate.h>
#include <netlist/circuit.h>
#include <utils/logger.h>
#include <iostream>
#include <iomanip>
#include <core/simulator.h>

// Right now assuming GATES can only have inputs

int main() {

    Circuit nandCircuit;

    // Inputs
    Signal A("A", false);
    Signal B("B", false);

    // Internal wire
    Signal temp("temp", false);

    // Output
    Signal Y("Y", true);

    // Register signals
    nandCircuit.add_signal(&A);
    nandCircuit.add_signal(&B);
    nandCircuit.add_signal(&temp);
    nandCircuit.add_signal(&Y);

    // Create gates
    AndGate andGate(&A, &B, &temp);
    NotGate notGate(&temp, &Y);

    // Register processes in circuit
    nandCircuit.add_process(&andGate);
    nandCircuit.add_process(&notGate);

    // Setup sensitivity
    A.add_sensitive(&andGate);
    B.add_sensitive(&andGate);

    temp.add_sensitive(&notGate);

    Logger::print_circuit_state(nandCircuit, 1);

    // now we will start the actual Simulation

    Simulator sim(&nandCircuit);

    sim.apply_input(&B, 1, 1);
    sim.apply_input(&B, 0, 2);
    sim.apply_input(&A, 1, 2);
    sim.apply_input(&B, 1, 3);
    sim.run();
    

}