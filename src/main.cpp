#include <core/signal.h>
#include <gates/and_gate.h>
#include <gates/not_gate.h>
#include <gates/or_gate.h>
#include <netlist/circuit.h>
#include <utils/logger.h>
#include <iostream>
#include <iomanip>

// Right now assuming GATES can only have inputs
void simulate_gate(Circuit& circuit, Signal &A, Signal &B, Signal &C, Process &gate){
    // set the current value to 0, 0
    const int A_width = 3;
    const int B_width = 3;
    const int C_width = 3;
    std::cout << "\n----------------\n";
    std::cout << "| "
              << std::left << std::setw(A_width) << "A"
              << "| "
              << std::setw(B_width) << "B"
              << "| "
              << std::setw(C_width) << "C"
              << "|\n";
    std::cout << "----------------\n";
    for(int x = 0; x <= 1; x++){
        for(int y = 0; y <= 1; y++){
            A.set_next(x);
            B.set_next(y);
            circuit.commit_all_signals();
            gate.execute();
            circuit.commit_all_signals();
            
            // Add the output
            std::cout << "| "
                  << std::left << std::setw(A_width) << A.get_value()
                  << "| "
                  << std::setw(B_width) << B.get_value()
                  << "| "
                  << std::setw(C_width) << C.get_value()
                  << "|\n";
        }
    }
    std::cout << "----------------\n";

}

int main() {

    // And Gate
    Circuit circuit_and;
    
    Signal A("A", false);
    Signal B("B", false);
    Signal C("C", false);

    circuit_and.add_signal(&A);
    circuit_and.add_signal(&B);
    circuit_and.add_signal(&C);

    AndGate andGate(&A, &B, &C);

    circuit_and.add_process(&andGate);
    std::cout << "--- AND GATE ---";
    simulate_gate(circuit_and, A, B, C, andGate);

    // Or Gate
    Circuit circuit_or;
    
    Signal P("P", false);
    Signal Q("Q", false);
    Signal R("R", false);

    circuit_or.add_signal(&P);
    circuit_or.add_signal(&Q);
    circuit_or.add_signal(&R);

    OrGate orGate(&P, &Q, &R);

    circuit_or.add_process(&orGate);
    std::cout << "--- OR GATE ---";
    simulate_gate(circuit_or, P, Q, R, orGate);

    Circuit circuit_not;
    Signal X("X", false);
    Signal Y("Y", true);

    circuit_not.add_signal(&X);
    circuit_not.add_signal(&Y);

    NotGate notGate(&X, &Y);
    
    circuit_not.add_process(&notGate);
    std::cout << "--- OR GATE ---";
    std::cout << "\n----------------\n";
    std::cout << "| "
              << std::left << std::setw(3) << "A"
              << "| "
              << std::setw(3) << "B"
              << "|\n";
    std::cout << "----------------\n";

    std::cout << "| "
                  << std::left << std::setw(3) << X.get_value()
                  << "| "
                  << std::setw(3) << Y.get_value()
                  << "|\n";

    X.set_next(1);
    circuit_not.commit_all_signals();
    notGate.execute();
    circuit_not.commit_all_signals();

    std::cout << "| "
                  << std::left << std::setw(3) << X.get_value()
                  << "| "
                  << std::setw(3) << Y.get_value()
                  << "|\n";
    return 0;
}