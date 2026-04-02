#ifndef CIRCUIT_GRAPH_H
#define CIRCUIT_GRAPH_H


#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <circuitBuilder/netlist/circuit.h>
#include <circuitBuilder/core/signal.h>
#include <circuitBuilder/gates/gate.h>

using namespace std;
class GraphNode;

class GraphEdge {
public:
    Signal* signal;
    GraphNode* from_node;        
    vector<GraphNode*> to_nodes;  
    bool is_cyclic;            

    GraphEdge(Signal* s) : signal(s), from_node(nullptr), is_cyclic(false) {}
};


class GraphNode {
public:
    Gate* gate;
    vector<GraphEdge*> in_edges;  
    vector<GraphEdge*> out_edges; 
    int scc_id;    
    int level;     

    GraphNode(Gate* g) : gate(g), scc_id(-1), level(-1) {}
};


class CircuitGraph {
public:
    vector<GraphNode*> nodes; 
    vector<GraphEdge*> edges;  
    vector<GraphEdge*> primary_inputs;           
    vector<GraphEdge*> primary_outputs;          
    int sccCount;
    
    void build(const Circuit& circuit);

    void applyTarjans();  

    void printGraph();
    
};

#endif