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
#include <bitset>


#define maxN 10000
using namespace std;
class CondensedNode;

class CondensedEdge {
public:
    CondensedNode* from;        
    CondensedNode* to; 

    vector<GraphEdge*> signals;
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

    CondensedNode(int sccId) : sccId(sccId), isSeq(false) {}
};


class CondensedGraph {
public:
    unordered_map<int, CondensedNode*> nodes; 
    vector<CondensedEdge*> edges;

    void build(const CircuitGraph& cg);

    void getReachability(vector<bitset<maxN>> &reachabilityMatrix);

    vector<int> applyTopologicalSort();  
    void assignLevels();

    void printGraph();

    void printReachabilityList(const vector<bitset<maxN>> &reach);

};

#endif