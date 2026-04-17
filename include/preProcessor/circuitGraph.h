#ifndef CIRCUIT_GRAPH_H
#define CIRCUIT_GRAPH_H


#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <circuitBuilder/netlist/circuit.h>
#include <circuitBuilder/core/signal.h>
#include <circuitBuilder/gates/gate.h>
#include <circuitBuilder/core/behaviour.h>

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
    Process* process;
    vector<GraphEdge*> in_edges;  
    vector<GraphEdge*> out_edges; 
    int scc_id;    
    int level;     

    GraphNode(Process* p) : process(p), scc_id(-1), level(-1) {}
};


class CircuitGraph {
public:
    vector<GraphNode*> nodes; 
    vector<GraphEdge*> edges;  
    vector<GraphEdge*> primary_inputs;           
    vector<GraphEdge*> primary_outputs; 
    unordered_map<Signal*, GraphEdge*> edgeMap;
    unordered_map<Process*, GraphNode*> nodeMap;         
    int sccCount;
    
    void build(const Circuit& circuit);

    void applyTarjans(); 
    void printGraph();
};

#endif