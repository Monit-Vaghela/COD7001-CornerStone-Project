#include <preProcessor/circuitGraph.h>
#include <stack>

void CircuitGraph::build(const Circuit& circuit){

    unordered_map<Signal*, GraphEdge*> edgeMap;
    unordered_map<Gate*, GraphNode*> nodeMap;

    // first we will convert signals to edges
    for(Signal* s: circuit.get_signals()){
        GraphEdge *edge = new GraphEdge(s);

        if(s->get_type() == SignalType::INPUT) primary_inputs.push_back(edge);
        else if(s->get_type() == SignalType::OUTPUT) primary_outputs.push_back(edge);

        edges.push_back(edge);

        edgeMap[s] = edge;
    }

    // now we will convert all gates to nodes
    for(Gate* g : circuit.get_gates()){
        GraphNode* node = new GraphNode(g);
        nodes.push_back(node);
        nodeMap[g] = node;
    }

    // now connect edges with nodes
    for(Gate* g : circuit.get_gates()) {
        GraphNode* node = nodeMap[g];

        // get input signals for this gate and add to node data
        for(Signal* s : g->getInputs()) {
            GraphEdge *edge = edgeMap[s];
            node->in_edges.push_back(edge);
        }

        // similarly do for outputs
        for(Signal* s : g->getOutputs()){
            GraphEdge *edge = edgeMap[s];
            node->out_edges.push_back(edge);
            edge->from_node = node; // this edge comes from this node
        }
    }

    // still need to fill the to_nodes to produce a sensitivity list
    for (Signal* s : circuit.get_signals()) {
        GraphEdge* edge = edgeMap[s];
        for (Process* p : s->get_sensitive_list()) {
            Gate* g = dynamic_cast<Gate*>(p);
            if (g) edge->to_nodes.push_back(nodeMap[g]);
        }
    }
}


static void dfs(
    GraphNode* node,
    int& timer,
    int& scc_count,
    unordered_map<GraphNode*, int>& disc,
    unordered_map<GraphNode*, int>& low,
    unordered_map<GraphNode*, bool>& on_stack,
    stack<GraphNode*>& stck,
    vector<vector<GraphNode*>>& all_sccs 
) {
    disc[node] = timer;
    low[node]  = timer;
    timer++;
    on_stack[node] = true;
    stck.push(node);                        

    // for each edge outgoing from this node visit its neighbor
    for (GraphEdge* edge : node->out_edges) {
        for (GraphNode* neighbour : edge->to_nodes) {

            // if not visited i.e not in discovery
            if (disc.find(neighbour) == disc.end()) {   
                dfs(neighbour, timer, scc_count, disc, low, on_stack, stck, all_sccs);
                low[node] = min(low[node], low[neighbour]); 
            }
            // if visited but on stack. i.e this path forms a cycle
            // we will turn on that this edge is cyclic
            else if (on_stack[neighbour]) {            
                low[node] = min(low[node], disc[neighbour]);
                edge->is_cyclic = true;             
            }
        }
    }

    // check if node is SCC root
    if (low[node] == disc[node]) {
        vector<GraphNode*> scc;

        // pop till we get this node
        // and push all the nodes into the scc
        while (stck.top() != node) {                    
            GraphNode* member = stck.top();
            stck.pop();
            on_stack[member] = false;
            member->scc_id = scc_count;                 
            scc.push_back(member);
        }
        // pop the root itself
        stck.top()->scc_id = scc_count;                
        on_stack[stck.top()] = false;
        scc.push_back(stck.top());
        stck.pop();

        scc_count++;
        all_sccs.push_back(scc);
    }
}

void CircuitGraph::applyTarjans() {
    int timer     = 0;
    int scc_count = 0;
    unordered_map<GraphNode*, int>  disc, low;
    unordered_map<GraphNode*, bool> on_stack;
    stack<GraphNode*> stk;
    vector<vector<GraphNode*>> all_sccs;

    // for each node in graph we will apply the dfs
    for (GraphNode* node : nodes) {
        if (disc.find(node) == disc.end()) {          
            dfs(node, timer, scc_count, disc, low, on_stack, stk, all_sccs);
        }
    }

    // store results — only SCCs with >1 member are true cycles
    // singletons get scc_id assigned but are_sequential = false
    // this->sccs = all_sccs;
}

// generated using claude
void CircuitGraph::printGraph() {
    auto signalTypeStr = [](SignalType t) -> std::string {
        switch(t) {
            case SignalType::INPUT:        return "IN";
            case SignalType::OUTPUT:       return "OUT";
            case SignalType::INTERMEDIATE: return "INT";
            default:                       return "???";
        }
    };

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║              CIRCUIT GRAPH                       ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n";

    // ── Edges (signals) ──────────────────────────────────────
    std::cout << "\n┌─ EDGES (signals) ── " << edges.size() << " total\n";
    for (GraphEdge* e : edges) {
        Signal* s = e->signal;

        std::cout << "│\n";
        std::cout << "│  [" << signalTypeStr(s->get_type()) << "] "
                  << s->get_name();
        if (e->is_cyclic)
            std::cout << "  ⟲ FEEDBACK";
        std::cout << "\n";

        // producer
        if (e->from_node)
            std::cout << "│      from  : " << e->from_node->gate->getId() << ")\n";
        else
            std::cout << "│      from  : <primary input>\n";

        // consumers
        if (e->to_nodes.empty()) {
            std::cout << "│      to    : <primary output / unconnected>\n";
        } else {
            std::cout << "│      to    : ";
            for (size_t i = 0; i < e->to_nodes.size(); i++) {
                if (i) std::cout << ", ";
                std::cout << e->to_nodes[i]->gate->getId();
            }
            std::cout << "\n";
        }
    }
    std::cout << "│\n└─────────────────────────────────────────────────\n";

    // ── Nodes (gates) ────────────────────────────────────────
    std::cout << "\n┌─ NODES (gates) ── " << nodes.size() << " total\n";
    for (GraphNode* n : nodes) {
        Gate* g = n->gate;

        std::cout << "│\n";
        std::cout << "│  " << g->getId();

        if (n->scc_id != -1)
            std::cout << "  scc=" << n->scc_id;
        else
            std::cout << "  scc=none";

        if (n->level != -1)
            std::cout << "  level=" << n->level;
        else
            std::cout << "  level=?";

        std::cout << "\n";

        // in-edges
        std::cout << "│      in  : ";
        if (n->in_edges.empty()) {
            std::cout << "<none>";
        } else {
            for (size_t i = 0; i < n->in_edges.size(); i++) {
                if (i) std::cout << ", ";
                std::cout << n->in_edges[i]->signal->get_name();
            }
        }
        std::cout << "\n";

        // out-edges
        std::cout << "│      out : ";
        if (n->out_edges.empty()) {
            std::cout << "<none>";
        } else {
            for (size_t i = 0; i < n->out_edges.size(); i++) {
                if (i) std::cout << ", ";
                std::cout << n->out_edges[i]->signal->get_name();
            }
        }
        std::cout << "\n";
    }
    std::cout << "│\n└─────────────────────────────────────────────────\n";

    // ── Primary I/O summary ──────────────────────────────────
    std::cout << "\n┌─ PRIMARY INPUTS ── " << primary_inputs.size() << " total\n│  ";
    for (size_t i = 0; i < primary_inputs.size(); i++) {
        if (i) std::cout << ", ";
        std::cout << primary_inputs[i]->signal->get_name();
    }
    std::cout << "\n└─────────────────────────────────────────────────\n";

    std::cout << "\n┌─ PRIMARY OUTPUTS ── " << primary_outputs.size() << " total\n│  ";
    for (size_t i = 0; i < primary_outputs.size(); i++) {
        if (i) std::cout << ", ";
        std::cout << primary_outputs[i]->signal->get_name();
    }
    std::cout << "\n└─────────────────────────────────────────────────\n\n";
}