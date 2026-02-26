# to run
```bash
make
./simulator
```
- This will run the circuit in the main.cpp
- different circuits can be found in temp folder

# Current Versions Features
- Full Discrete Event Simulator with abilities to create circuits using `and`, `or` and `not` gates

## How to create circuits (example : Nor using or and not)

### Step 1 : Create a Circuit object
```cp
Circuit circuit;
```
### Step 2 : Add Signals (wires)
```cpp
Signal A("A", false) // signal name -> A, its starting Value -> 0(false)
Signal B("B", false)

//Intermediate signal after OR gate
Signal Y1("Y1", false)


// After Creating signals you need to add them to the circuit
circuit.add_signal(&A);
circuit.add_signal(&B);
circuit.add_signal(&Y1);
circuit.add_signal(&C);
```

### Step 3: Add gates
```cpp
OrGate orGate(&A, &B, &Y1); // 'or' and 'and' gates take 2 input and one output
NotGate notGate(&Y1, &C); // 'not' gate takes 1 input signal and 1 output signal

// After creating gates you need to add them to the circuit
circuit.add_process(&orGate);
circuit.add_process(&notGate);
```

### Step 4: Add sensitive_list
- in this step you need to add the elements which will be effected due to change in Signals and execution of gates
- This step should be done carefully, because this is the core concept which creates ripple effect for delta cycles.
```cpp
A.add_sensitive(&orGate);
B.add_sensitive(&orGate);
Y1.add_sensitive(&notGate);
```

- Now the circuit is complete you can print the current state of circuit
```cpp
 Logger::print_circuit_state(circuit, 1); // 1 just makes sure that we print the header first time from now onwards use 0.
```

## How to Simulate the circuit
- For any time where a signal might change add them to the event queue.
- first create an instance of the `simulator `and then `apply_input` function just adds the signal changes to event queue with `delta time = 0`
```cpp
Simulator sim;
sim.apply_input(&B, 1, 0) // (Signal, newValue, timestamp) (A B | 0 1)

sim.apply_input(&A, 1, 1) // (Signal, newValue, timestamp)
sim.apply_input(&B, 0, 1) // (Signal, newValue, timestamp)

sim.apply_input(&B, 1, 0) // (Signal, newValue, timestamp)
```
- Now you can run the circuit
```cpp
sim.run()
```

## How does circuit is simulated
```text
PROCEDURE run

    WHILE event_queue is not empty DO

        T, D ← time and delta of top event
        current_batch ← all events with (T, D)

        -----------------
        EXECUTE EVENTS
        -----------------
        FOR each event in current_batch DO
            IF event is SIGNAL_UPDATE THEN
                set signal next value
            ELSE IF event is PROCESS_EXEC THEN
                execute process
            ENDIF
        ENDFOR


        -----------------
        COMMIT SIGNALS
        -----------------
        changedSignals ← empty list

        FOR each signal in circuit DO
            old ← current value
            commit signal
            IF value changed THEN
                add signal to changedSignals
            ENDIF
        ENDFOR


        -----------------
        SCHEDULE NEXT DELTA
        -----------------
        FOR each signal in changedSignals DO
            FOR each sensitive process DO
                push PROCESS_EXEC event at (T, D+1)
            ENDFOR
        ENDFOR

    ENDWHILE

END PROCEDURE
```