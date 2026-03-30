import itertools

def generate_simulator_inputs(n_inputs=10000, variable_inputs=4):
    inputs = [f"x{i+1}" for i in range(n_inputs)]

    fixed_count = n_inputs - variable_inputs

    with open("output.txt", "w") as f:
        f.write("simulator andGate {\n")
        f.write("    processes {\n")

        timestamp = 1

        # Only vary last `variable_inputs`
        for combo in itertools.product([0, 1], repeat=variable_inputs):

            assignments = []

            # First fixed inputs = 0
            for i in range(fixed_count):
                assignments.append(f'"{inputs[i]}" <= 0')

            # Last variable inputs
            for i in range(variable_inputs):
                idx = fixed_count + i
                assignments.append(f'"{inputs[idx]}" <= {combo[i]}')

            f.write(f'        process([{", ".join(assignments)}], {timestamp});\n')
            timestamp += 1

        f.write("    }\n")
        f.write("}")


# Run
generate_simulator_inputs(10000, 4)