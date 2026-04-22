# COD7001 — Cornerstone Project: VHDL Discrete Event Simulator

A cycle-accurate, event-driven VHDL simulator with sequential and parallel simulation modes, waveform output, and a performance dashboard.

---

## Folder Structure

```
.
├── compiler              # Compiled compiler binary
├── circuitBuilder        # Compiled circuit builder + simulator binary
├── comparator            # Compiled comparator binary
├── run.sh                # Pipeline runner script
│
├── src/
│   ├── compiler/         # Lexer, parser, semantic analyzer, JSON writer
│   ├── circuitBuilder/   # JSON parser, circuit netlist, gate models
│   ├── simulator/
│   │   ├── sequential/   # Sequential DES loop
│   │   └── parallel/     # Parallel simulation (v1, v2, v3)
│   ├── preProcessor/     # Circuit graph, condensed graph, SCC
│   └── comparator.cpp    # Trace comparator
│
├── include/              # Header files mirroring src/ structure
│
├── dashboard/
│   ├── dashboard.html    # Performance visualization dashboard
│   ├── seq_stats.json    # Sequential simulation stats (auto-generated)
│   └── par_stats.json    # Parallel simulation stats (auto-generated)
│
├── output/
│   ├── waveSeq.vcd       # Sequential simulation waveform
│   ├── wavePar.vcd       # Parallel simulation waveform
│   ├── traceSeq.bin      # Sequential simulation trace (binary)
│   └── tracePar.bin      # Parallel simulation trace (binary)
│
├── tests/                # Test circuits (.cir, .tes, pre-generated JSONs)
└── docs/                 # Goals and version documents
```

---

## Building

```bash
make
```

This builds all three binaries: `compiler`, `circuitBuilder`, and `comparator`.

To clean:
```bash
make clean
```

---

## How to Run

The easiest way is to use `run.sh` which runs the full pipeline automatically.

### Using run.sh

```bash
chmod +x run.sh
./run.sh <circuit.cir> <simulation.tes> -s|-c [-UIwave|-UIdash|-check]
```

**Modes (required):**
- `-s` — sequential simulation only
- `-c` — both sequential and parallel simulation

**Optional flags (only with `-c`):**
- `-UIwave` — open GTKWave with the parallel waveform after simulation
- `-UIdash` — start the dashboard server and open in browser
- `-check` — run the comparator to verify sequential and parallel outputs match

**Examples:**
```bash
# Sequential only
./run.sh tests/fulladder/fulladder.cir tests/fulladder/fulladder.tes -s

# Parallel + sequential
./run.sh tests/fulladder/fulladder.cir tests/fulladder/fulladder.tes -c

# Parallel + open waveform in GTKWave
./run.sh tests/fulladder/fulladder.cir tests/fulladder/fulladder.tes -c -UIwave

# Parallel + open dashboard
./run.sh tests/fulladder/fulladder.cir tests/fulladder/fulladder.tes -c -UIdash

# Parallel + verify correctness
./run.sh tests/fulladder/fulladder.cir tests/fulladder/fulladder.tes -c -check
```

---

## Running Components Individually

### Compiler

Takes `.cir` and `.tes` source files and produces JSON files for the circuit builder.

```bash
./compiler <circuit.cir> <simulation.tes>
```

Output: `<circuit>_cir.json` and `<simulation>_tes.json` in the same folder as the input files.

Example:
```bash
./compiler tests/fulladder/fulladder.cir tests/fulladder/fulladder.tes
# produces: tests/fulladder/fulladder_cir.json
#           tests/fulladder/fulladder_tes.json
```

### Circuit Builder

Takes the JSON files produced by the compiler and runs simulation.

```bash
./circuitBuilder <cir.json> <tes.json> -s|-c
```

- `-s` — sequential simulation
- `-c` — sequential + parallel simulation (comparison mode)

Output goes to the `output/` directory.

Example:
```bash
./circuitBuilder tests/fulladder/fulladder_cir.json tests/fulladder/fulladder_tes.json -c
```

### Comparator

Compares sequential and parallel simulation traces to verify correctness.

```bash
./comparator output/traceSeq.bin output/tracePar.bin
```

Exits with code 0 if traces match, 1 if they differ.

### Dashboard

The dashboard visualizes performance metrics comparing sequential and parallel simulation.

```bash
cd dashboard
python3 -m http.server 8080
```

Then open your browser at `http://localhost:8080/dashboard.html`.

The dashboard shows:
- **Graph 1 & 2** — Execution time per delta cycle for sequential and parallel
- **Graph 3** — Speedup of parallel over sequential per delta cycle
- **Graph 4** — Synchronization overhead introduced by parallelism

---

## Input File Format

### `.cir` — Circuit Description

Defines the entity (ports) and architecture (internal signals, mappings, behaviour).

```
entity <Name> {
    inputs  : ["port1", "port2", ...];
    outputs : ["port3", ...];
}

architecture <Name> {
    signals : ["sig1", "sig2", ...];
    signalMapping : [
        "sig1" <= "port1",
        ...
    ];
    behaviour {
        sig3 <= sig1 and sig2;
        if is_risingEdge(CLK) {
            Q <= D;
        }
    }
}
```

Entity recursion (sub-entity instantiation):
```
entity Top uses ["SubEntity"] {
    ...
}
architecture Top {
    behaviour {
        instance inst1 of SubEntity (portA => sigX, portB => sigY);
    }
}
```

### `.tes` — Simulation Description

Defines stimulus processes applied at each timestamp.

```
simulator <Name> {
    processes {
        process(["sig1" <= 0, "sig2" <= 1], 0);
        process(["sig1" <= 1, "sig2" <= 0], 1);
    }
}
```