def generate_simulator_inputs(n_inputs=10000):
    inputs = [f"x{i+1}" for i in range(n_inputs)]

    with open("output.txt", "w") as f:
        f.write("simulator andGate {\n")
        f.write("    processes {\n")

        # Process 1: all 0
        all_zero = [f'"{inp}" <= 0' for inp in inputs]
        f.write(f'        process([{", ".join(all_zero)}], 1);\n')

        # Process 2: all 1
        all_one = [f'"{inp}" <= 1' for inp in inputs]
        f.write(f'        process([{", ".join(all_one)}], 2);\n')

        f.write("    }\n")
        f.write("}")


# Run
generate_simulator_inputs(10000)