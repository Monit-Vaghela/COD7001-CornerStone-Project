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

    Circuit circuit;

    // Inputs (for Or gate)
    Signal A("A", false);
    Signal B("B", false);

    // Internal wire
    Signal Y1("Y1", true);
    Signal Y2("Y2", true);
    Signal Y3("Y3", true);

    // Output
    Signal C("C", false);

    // Register signals
    circuit.add_signal(&A);
    circuit.add_signal(&B);
    circuit.add_signal(&Y1);
    circuit.add_signal(&Y2);
    circuit.add_signal(&Y3);
    circuit.add_signal(&C);

    // Create gates
    NotGate notA(&A, &Y1);
    NotGate notB(&B, &Y2);
    OrGate Y1orY2(&Y1, &Y2, &Y3);
    NotGate notY3(&Y3, &C);

    // Register processes in circuit
    circuit.add_process(&notA);
    circuit.add_process(&notB);
    circuit.add_process(&Y1orY2);
    circuit.add_process(&notY3);
    

    // Setup sensitivity
    A.add_sensitive(&notA);
    B.add_sensitive(&notB);
    Y1.add_sensitive(&Y1orY2);
    Y2.add_sensitive(&Y1orY2);
    Y3.add_sensitive(&notY3);


    Logger::print_circuit_state(circuit, 1);

    // now we will start the actual Simulation

    Simulator sim(&circuit);

    sim.apply_input(&B, 1, 1);
    sim.apply_input(&B, 0, 2);
    sim.apply_input(&A, 1, 2);
    sim.apply_input(&B, 1, 3);
    sim.run();
    

}