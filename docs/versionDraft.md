# Flow of code
- The compilation to simualtion is broken into 3 parts
```md
1. Compilation
    a. input : .cir and .tes fikes
    b. output : cir.json and tes.json
2. Circuit Builder
    a. input : cir.json and tes.json
    b. output : start simulation
3. Simulation
```
- Each circuit will have 2 files 
```bash
.cir -> This file will define circuit and its behaviour
.tes -> This file will create processes and define simulation behaviour
```
- FLOW
```md
1. compiler will take 2 input files (.cir and its .tes)
2. compiler will compile the files as per the defined grammer and syntax
3. compiler will output 2 json files cir.json, tes.json
4. circuitBuilder will parse both these json file and build the circuit and initialize the simulation
5. simulator will simulate the defined behaviour
```


# Compiler
## Stages
- Divide into - stages
```md
1. lexer + parser : grammer cheking and ast building
2. TBD
```

## Grammer and syntax
### `.cir` file
#### Keywords
```md
blackbox circuit : Here black box circuit is defined as a circuit with just inputs and outputs defined with all the input/output ports, it does not have any behavioural traits
```
| Sr. No | Keyword | Brief Meaning |
|--------|---------|--------------|
| 1 |`Entity`|   Used to define the blackBox of circuit |
| 2 | `Inputs` | Used to take the inputs of blackbox circuit |
| 3 | `Outputs`| Used to take the outputs of blackbox circuit|
| 4 | `AND` | and gate |
| 5 | `OR` | or gate |
| 6 | `NOT` | not gate |
| 7 | `XOR` | xor gate |
| 8 | `XNOR` | xnor gate |
| 9 | `Signals` | used to create signals (internal wires used in circuit) |
| 10 | `SignalMapping` | used to map the signals with ports defined in entity |
| 11 | `Behaviour` | Used to define the internal behaviour of the blackbox circuit |
| 12 | `Architecture` | This defines the full behaviour of our circuit, building every gates, signals and connection required |

#### Grammer
- For now only one entity and one architecture to be defined in `.cir` file.
- Firstly every circuit element is to be defined using `entity` keyword.
```text
entity <EntityName> {
    Inputs : [ <InputPortName1, InputPortName2 ......>];
    Outputs : [ <OutputPortName1, OutputPortName2 ......>];
}
```
- constraints
```c
// 1. Naming Convention is standard
// 2. Inputs and outputs define the ports of the blackbox circuit
// 3. no names should clash across types
// 4. for now data type assumed to be bit type only
```

- Now using `architecture` behaviour can be defined.
```text
architecture <EntityName> {
    Signals : {<SignalName1>, <SignalName2>, .....};
    SignalMapping : [
        <SignalName_i> <= <PortName_j>,
        .
        .
        .
    ];
    Behavoiour{
        <SignalName_i> <= <digital_expression>;
        <SignalName_j> <= <digital_expression>;
        <SignalName_k> <= <digital_expression>;
        ....
    }

}
```
- Constraints
```c
// 1. EnityName must match with the declared entity
// 2. PortName must be defined in entity
// 3. <= is assignment
// 4. <PortName1> <= <SignalName_i> can be many to one, i.e different ports can have same signal
// 5. After signalmapping no signal and port should be left unmapped
// 6. <digital_expression> : S := S <gate> S
// Where S is signalname and gate can be or, and, not, xor, xnor
// 7. precedence : not > and > or (havent decided about xor, xnor)
// 8. in behavoiut <SignalName_i> must be mapped to an output port, input signals cannot be assigned digital expression.
```

### `.tes` file
- This file will allow user to define simulation time behaviour

#### Keywords
| Sr. No | Keyword | Brief Meaning |
|--------|---------|--------------|
| 1 |`Simulation`|   Used to define the Simulation time behaviour of the circuit |
| 2 | `Processes` | Used list out the processes at given timestamp |
| 3 | `Process`| Each process of processes|

#### Grammer
```text
Simulator <EntityName>{
    Processes : {
        Process([<SignalName1> <= 0 or 1>, <SignalName2> <= 0 or 1> ....], timestamp (unsigned int));
        Process([<SignalName1> <= 0 or 1>, <SignalName2> <= 0 or 1> ....], timestamp (unsigned int));
        ......
    }
}
```

- Constraints
```c
// 1. Entity must match the entity which is declared in .cir file with its architecture.
// 2. Signal name which is assigned 0 or 1 must be mapped to an input port.
```

## Outputs
### `cir.json`
```json
{
    "entity" : {
        "metadata" : {
            "noOfInputPorts" : "int",
            "noOfOutputPorts" : "int"
        },

        "inputs" : ["List of input names"],
        "otuptus" : ["List of output names"]
    },

    "Architecture" : {
        "metadata" : {
            "noOfTotalGates" : "int",
            "noOfAndGates" : "int",
            "noOforGates" : "int",
            "noOfNotGates" : "int",
            "noOfXorGates" : "int",
            "noOfXnorGates" : "int",
            "noOfSignals" : "int"
        },

        "SignalMapping" : {
            [
                {"SignalMapping" : "MappedPortName", "SignalType" : "0->input, 1->output"},
            ]
        },

        "Behaviour" : {
            "TBD" : "TBD"
        }
    }
}
```

### `tes.json`
```json
{
    "simulation" : {
        "metadata" : {
            "noOfProcesses" : "int",
            "initialTimeStamp" : "int",
            "finalTimeStamp" : "int"
        },
        "Processes" : {
            [
                {"SingalName" : "Value(0, 1)", "timestamp" : "int"},
                {"LIST": "LIST"}
            ]
        }
    }
}
```

# Circuit Builder 
- Will take input json files and parse it to build the circuit
- TBD


# Simulator
- TBD