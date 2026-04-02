#ifndef CONDENSED_GRAPH_H
#define CONDENSED_GRAPH_H


#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <circuitBuilder/netlist/circuit.h>
#include <circuitBuilder/core/signal.h>
#include <circuitBuilder/gates/gate.h>
#include <preProcessor/circuitGraph.h>

using namespace std;
class CondensedNode;

class CondensedEdge {
public:
    CondensedNode* from;        
    vector<CondensedNode*> to; 
    bool is_cyclic;            
};


class CondensedNode {
public:
    int sccId;
    vector<GraphNode*> scc_members;
    bool isSeq;
    int level;

    vector<CondensedEdge*> in_edges;  
    vector<CondensedEdge*> out_edges;     

    CondensedNode(int sccId) : sccId(sccId) {}
};


class CondensedGraph {
public:
    vector<CondensedNode*> nodes; 
    vector<CondensedEdge*> edges;            

    void build(const CircuitGraph& cg);

    void assignLevels();  

    void printGraph();
    
};

#endif