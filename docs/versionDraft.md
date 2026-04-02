# Compiler
- New addition of different control flow
- in .cir - behaviour can have if and else to define rising or falling edge
- in .tes - processes can have a for loop to define multiple processes with a single statement.
- more to think

# Parallelization
- new approach with use a dependency graph to understand what can be runned in parallel and what cannot be
- the graph creation is devided into 3 parts 

## Part 1 : CircuitGraph
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


### Part 2: CircuitGraph with SCC id set
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

### Part 3 : Creating a DAG from CicruitGraph (CondenseddGraph)
- we will call this new DAG as `CondensedGraph`

#### formaal definition of CondensedGraph

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