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
    Processes {
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
## How to parse behaviour
- Lets say if the architecture defines 2 behavioural traits
```md
<OutputSignal1> <= <inputSignal1> <gate1> <inputSignal2> 
<OutputSignal2> <= (<inputSignal3> <gate2> <inputSignal4>) <gate3> <inputSignal5>
```
- The json file behaviour will only cointain a single gate and their input outputs.
- hence the behaviours that we will get are
```json
{
    "OutputSignal" : "outputSignal1", 
    "gateExpressionInputs" : ["inputSignal1", "inputSignal2"], 
    "gatesUsed" : ["gate1"], 
    "expression" : "gate1(inputSignal1, inputSignal2)"
},
{
    "OutputSignal" : "intermediateSignal1",
    "gateExpressionInputs" : ["InputSignal3", "inputSignal4"],
    "gateUsed" : ["gate2"],
    "expression" : "gate2(inputSignal3, inputSignal4)"
},
{
    "OutputSignal" : "OutputSignal2",
    "gateExpressionInputs" : ["intermediateSignal1", "inputSignal5"],
    "gateUsed" : ["gate3"],
    "expression" : "gate3(intermediateSignal1, inputSignal5)"
}
```
- Now here the intermediateSignal must be add to the intermideateSignal list in the entity. also this increases the total number of signals which should be taken care of 

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
        "otuptus" : ["List of output names"],
        "intermediates" : ["List of intermediate signals used"]
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

        "SignalMapping" : [
            {"SignalMapping" : "MappedPortName", "SignalType" : "input, output"},
        ],

        "Behaviour" : [
            {
                "OutputSignal" : "OutputSignalName", 
                "gateExpressionInputs" : ["List of Inputs"], 
                "gatesUsed" : ["List of gates used"], 
                "expression" : "Expression in recursive function calling format"
            },
            .....
        ]
        
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
        "Processes" : 
        [
            {"SingalName" : "Value(0, 1)", "timestamp" : "int"},
            {"LIST": "LIST"}
        ]
        
    }
}
```

# Circuit Builder 
- Will take input json files and parse it to build the circuit
## Steps circuit builder must perform
- check the json file like
    * each gates should have 2 inputs and one output.
    * every input and output signal must be clearly mapped.
    * no of gates used must match with the gates used in all expressions combined, same with signals.

## Parsing
- CPP does not have an inbuilt parser, hence need to make our own.
- simple string based parsing for each element find "{" and the parse the element by repetedly finding ":" then find the "}" and then close
- using similar concepts for all other element we parse both `cir.json` and `tes.json`

## Circuit Building
### 1. Create all the signals
- every signals is created with a small difference.
- Input/output signals are created with the name that they are mapped with.
- intermediate signals are created with the defined name only.

### 2. gates are created and added
- now we will go through each behaviour and add the required gate
- give it input and output and then add it to the circuit

### 3. Add sensitivity list
- each signal will have a sensitivityList (aside from Output type)
- This list will tell us which gates are sensitive to change in this signal
ex:
```text
      |-----
A-----|     -   
      |     -------C
B-----|     -
      |-----

then this and gate (lets say A1) will be in sensitivity list of both A and B.
```

# Simulator
- Simulator will take the circuit created and add the user defined process at given timestamp to simulate the gates.
- there are no changes to simulator and eventqueue from previous version, works the same.



# First attempt at parallelization (Naice)

## Assumption : no cyclic circuits
- This approach assumes that the circuit does not have any cycles
- i.e no ouput from later stage can be input for a previous state
- ex : latches

## Formal definition of circuit.
- A circuit will be defined as a set of `gates` and `signals`.
 
```math
C = \{G, S\}
```
 
- Where:
  - $G$ = set of gates in the circuit  
  - $S$ = set of signals in the circuit
- set $S$
 
```math
S = \{s_1, s_2, s_3, \ldots, s_m\}
```
 
- set $G$
 
```math
G = \{g_1, g_2, g_3, \ldots, g_n\} \; \; \forall g_i \in G, g_i = \{I, o\}
```
 
- Where:
  - $I = \{i_1, i_2, i_3, \ldots, i_k\} \; \forall i_j \in S \; \; \text{[Inputs for the gate } g_i \text{]}$
  - $o \in S$ and $o \notin I \; \; \text{[Output of the gate } g_i \text{]}$
- level of gates
 
```math
\forall g_i \in G \; \; level(g_i) = max(level(I))
```
 
- level of signals $\forall s_j \in S$
 
```math
\text{level}(s_j) =
\begin{cases}
0 & \text{if } s_j \text{ is an input} \\
\text{level}(g_k) + 1 & \text{if } \exists\, g_k = \{I,o\} \text{ such that } s_j = o
\end{cases}
```

## parallelisation
- now with this definition of circuit we can run gates at same level parallely.