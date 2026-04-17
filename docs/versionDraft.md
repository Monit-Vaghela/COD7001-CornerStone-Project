# Compiler
- New addition of different control flow
- in .cir - behaviour can have if and else to define rising or falling edge
- in .tes - processes can have a for loop to define multiple processes with a single statement.
- more to think

## what to add

### Entity Recursion(MOST IMP)
Q. what is entity recursion
- being able to define a sub entity which can be used in another entity
- ex (conceptual)

```md
entity halfAdder{

}
behaviour halfadder{

}

entity full adder uses [halfAdder] {

}
behaviour fullAdder{

}
```
- full liberty on upto you on how to implement this ....... need to follow VHLD semantics not syntax
- this does not change anything in JSON only in how code is written, gives reproducibility
- do run a full pipeline in order to see if its working correctly

### IF ELSE in .cir
Q. what about if,else
- in behaviour user can define behaviour using if and else
- ex
```md
enitity Dflipflop{
    inputs : ["x", y"];
    outputs: ["a", "b"];
}

behaviour{
    signals : ["D", "CLK", "Q", "Qb"];
    signalMapping : [
        "D" <= "x",
        "CLK" <= "y"
    ];
    behaviour{
        if is_risingEdge(CLK){
            Q = D;
        }
        Qb = not(Q);
    }
}
```
- what changes in JSON
- PREVIOUSLY
```md
{
    "OutputSignal" : "outputSignal1", 
    "gateExpressionInputs" : ["inputSignal1", "inputSignal2"], 
    "gatesUsed" : ["gate1"], 
    "expression" : "gate1(inputSignal1, inputSignal2)"
},
```
- only a single behaviour type, now there are 2 : `behaviour` and `gate`
- behaviour type is of format
```json
{
    "ProcessType" : "Behaviour",
    "OutputSignal" : "<outputSignal>",
    "InputSignal" : "<inputSignal>",
    "edgetype" : "rising",
    "clockSignal" : "CLK"
}
```
- gate type is of format(similar to before just defines it is a gate before hand)
```json
{
    "ProcessType" : "Gate",
    "OutputSignal" : "outputSignal1", 
    "gateExpressionInputs" : ["inputSignal1", "inputSignal2"], 
    "gatesUsed" : ["gate1"], 
    "expression" : "gate1(inputSignal1, inputSignal2)"
}
```
- look at following examples in test for better understading
1. `dflipflop/cir.json`
2. `enabledDFF/cir.json`

- another use of if in behaviour in .cir - TO DEFINE STATES
- ex
```md
if (<inputSignali> == "0" && <inputSignalj> == "0" ......){
    circuitState = INVALID;
}
```
- this are useful for stuff like SR latch which has invalid state
- havent defined how this will be implemented or send in JSON file
- you can decide on your own.
### input checking in tes.json
- timestamp should start from 1
- no two signals should have different value for same timestamp
```json
{"x" : 0, "timestamp" : 1},
{"x" : 1, "timestamp" : 1},
```
- this is not allowed
```json
{"x" : 0, "timestamp" : 1},
{"x" : 0, "timestamp" : 1},
```
- neither is this allowed

### large code generation
- need to make sure that the code works properly for large cicuits
- current end test will be tested on 2 test cases
1. 64 register file each register cointaining 8 dflipflop with a databus to read from and write into
2. n bit adder
- check out code in `registerFile_small` and `registerFileWithDataBus` for better understanding what we need to do

### small QOL
- json file should be generated from where the input files were passed.
# Parallelization
- new approach with use a dependency graph to understand what can be runned in parallel and what cannot be
- the graph creation is devided into 3 parts 

## Part 1.1 : CircuitGraph
- This graph is direct respresentation of the circuit, as it is. nothing more nothing less
- This graph can contain cycles.

### Formal definition of CircuitGraph
- G = (V, E)
- where V = set of graphnodes
    * A graph node cointains following data
```cpp
Gate* gate; // pointer to the gate it is representing node of 
vector<GraphEdge*> in_edges; // this gates inputs 
vector<GraphEdge*> out_edges; // this gate's output
int scc_id; // wether this node is part of a strongly connected component (initially 1)    
int level; // same as previously defined in naive attempt
```
- where E = set of graphEdges
```cpp
Signal* signal; // the signal this edge is representing
GraphNode* from_node; // the gate for which this signal is output for (null if input signal)       
vector<GraphNode*> to_nodes;  // the gate(node) to which this it input as (null if output gate)
bool is_cyclic; // (currectly false) later is true if the the edge is part forms a cycle
```

- Some important semantics
    - out_edges : can only be one unique from a node but can be multiple of that unique.
    - in_edges : can have many to one mapping
    - from_nodes : one to one mapping
    - to_nodes : one to many mapping.


### Part 1.2: CircuitGraph with SCC id set
- initially the SCC id is not set.
- we will run tarjans algorithm to find the scc in the graph and group then together to form a DAG
- the algorithm used 
```cpp
procedure DFS(node):
    assign discovery_time[node] = current_time
    assign low_link[node] = current_time
    increment current_time

    push node onto stack
    mark node as on_stack

    for each neighbor of node:
        
        if neighbor is not visited:
            DFS(neighbor)
            low_link[node] = min(low_link[node], low_link[neighbor])

        else if neighbor is on stack:
            low_link[node] = min(low_link[node], discovery_time[neighbor])

    // check if node is root of SCC
    if low_link[node] == discovery_time[node]:
        create new SCC

        repeat:
            top_node = pop from stack
            mark top_node as not on_stack
            assign SCC id to top_node
            add top_node to current SCC
        until top_node == node

        store current SCC

// driver algo
initialize:
    current_time = 0
    SCC_count = 0
    empty stack
    empty discovery_time map
    empty low_link map
    empty on_stack map

for each node in graph:
    if node is not visited:
        DFS(node)
```


- this algorithm will give us a CircuitGraph with proper SCC id and cyclic edges

### Part 2 : Creating a DAG from CicruitGraph (CondenseddGraph)
- we will call this new DAG as `CondensedGraph`

#### formal definition of CondensedGraph

#### Algo to build
```text
1) Create one condensed node per unique sccId

2) now for each GraphEdge in CircuitGraaph
    - skip if both the nodes of edges are in same scc
    - skip if inputs/output nodes
    - check if edge betweeen both scc already exists or not
    - if not : create a CondensedEdge
    - add this edge to list oof edges in condensedEdge

```


### Part 3 : precomputing Independent sets 
- 2 nodes A and B are independent, if There is no path from A To B and no path form B to A.
- This can be calculated using a topological sort and dp
- i.e
```text
if there exists a path from u to v (u -> v), then for every edge x where x is reachable from u then x is also reachable from v
```
#### algo for topological sort
```md
indegree[u] = number of incoming edges

queue = all nodes with indegree 0
topo = []

while queue not empty:
    u = pop(queue)
    topo.push(u)

    for v in adj[u]:
        indegree[v]--Q. what is entity recursion
- being able to define a sub entity which can be used in another entity
- ex (conceptual)

```md
entity halfAdder{

}
behaviour halfadder{

}

entity full adder uses [halfAdder] {

}
behaviour fullAdder{

}
```
- full liberty on upto you on how to implement this ....... need to follow VHLD semantics not syntax
- this does not change anything in JSON only in how code is written, gives reproducibility
- do run a full pipeline in order to see if its working correctly

Q. what about if,else
- in behaviour user can define behaviour using if and else
- ex
```md
enitity Dflipflop{
    inputs : ["x", y"];
    outputs: ["a", "b"];
}

behaviour{
    signals : ["D", "CLK", "Q", "Qb"];
    signalMapping : [
        "D" <= "x",
        "CLK" <= "y"
    ];
    behaviour{
        if is_risingEdge(CLK){
            Q = D;
        }
        Qb = not(Q);
    }
}
```
- what changes in JSON
- PREVIOUSLY
```md
{
    "OutputSignal" : "outputSignal1", 
    "gateExpressionInputs" : ["inputSignal1", "inputSignal2"], 
    "gatesUsed" : ["gate1"], 
    "expression" : "gate1(inputSignal1, inputSignal2)"
},
```
- only a single behaviour type, now there are 2 : `behaviour` and `gate`
- behaviour type is of format
```json
{
    "ProcessType" : "Behaviour",
    "OutputSignal" : "<outputSignal>",
    "InputSignal" : "<inputSignal>",
    "edgetype" : "rising",
    "clockSignal" : "CLK"
}
```
- gate type is of format(similar to before just defines it is a gate before hand)
```json
{
    "ProcessType" : "Gate",
    "OutputSignal" : "outputSignal1", 
    "gateExpressionInputs" : ["inputSignal1", "inputSignal2"], 
    "gatesUsed" : ["gate1"], 
    "expression" : "gate1(inputSignal1, inputSignal2)"
}
```
        if indegree[v] == 0:
            push v into queue
```

#### algo for DP
```md
for u in reverse(topo):
    for each edge u → v:
        reach[u][v] = true
        reach[u] |= reach[v]
```

- finally we got a reachability matrix which will be used in following manner in simulation.
```cpp
//TBD
```

# Circuit Builder and Simulator.
## Issues
1) Currently a signal can only be input and output only, but there might be a signal which is both. (sol : add a new gate, none this gate will pass the value of input to output) [fixed]
2) no intial signal type - currently signal can be 0 or 1 only, it needs another state `U` i.e uninitialized [fixed]
3) current simulation is only for sequential circuit, it needs `Delta Stablization loop` i.e stablize till there are no more changes in the circuit.[fixed]
4) need to add rising and falling edge for flipflops. hence need to save previous values and if there is a change then it trigeers the component
5) need new components like latches, flipflops , etc.
