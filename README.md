# For Detailed Document for the project read `docs/versionDraft.md`
---

# About Version
- this version allows the user to code in a custom VHDL like language which compiles the circuit defined in the langauge and simulates it in the given inputs.

---
# How to run
- Firstly use the make file to make the required components

## 1 Comiler (Work in progress)

## 2. CircuitBiulder
```bash
make circuitBuilder
```
- this will create circuitBuilder object file
- now 2 inputs files are needed `cir.json` and `tes.json`(about these files can be read in `versionDraft.md`)
- tests folder cointains multiple simple testCases

```bash
./circuitBuilder ./tests/and/cir.json ./tests/and/tes.json
```

- this will build and simulate the circuit.