#include <preProcessor/condensedGraph.h>
#include <stack>
#include <map>
#include <queue>
#include <bitset>

void CondensedGraph::build(const CircuitGraph& cg){

    // 1) create condensed node for each scc
    int maxScc = cg.sccCount;

    for(int i = 0; i < maxScc; i++){
        CondensedNode *node = new CondensedNode(i);
        nodes[i] = node;
    }

    // now add the Nodes to this condensed graph.
    for(auto *node : cg.nodes) nodes[node->scc_id]->scc_members.push_back(node);
    
    // now decide if this node is seq.
    for(auto *node : cg.nodes) if(nodes[node->scc_id]->scc_members.size() >= 2) nodes[node->scc_id]->isSeq = true;
        
    map<pair<int,int>, CondensedEdge*> edgeMap;
    // Step 2 — convert CircuitGraph edges to CondensedEdges
    for (GraphEdge* cgEdge : cg.edges) {

        // skip primary inputs — no from_node
        if (cgEdge->from_node == nullptr) continue;

        int from_scc = cgEdge->from_node->scc_id;

        for (GraphNode* to_node : cgEdge->to_nodes) {
            int to_scc = to_node->scc_id;

            // skip internal SCC edges
            if (from_scc == to_scc) continue;

            auto key = make_pair(from_scc, to_scc);

            // check if CondensedEdge already exists
            if (edgeMap.find(key) == edgeMap.end()) {
                // create new condensed edge
                CondensedEdge* ce = new CondensedEdge();
                ce->from = nodes[from_scc];
                ce->to   = nodes[to_scc];

                // wire into both nodes
                ce->from->out_edges.push_back(ce);
                ce->to->in_edges.push_back(ce);

                edges.push_back(ce);
                edgeMap[key] = ce;
            }

            // always add the signal to the edge (even if edge existed)
            edgeMap[key]->signals.push_back(cgEdge);
        }
    }
}

vector<int> CondensedGraph::applyTopologicalSort() {
    // 1) need to assign in degree for each node.
    int n = nodes.size();
    vector<int> inDegree(n, 0);
    for(auto &[key, value] : nodes){
        inDegree[key] = value->in_edges.size();
    }
    // 2) topological sorting

    vector<int> topo;
    queue<int> q;

    for(int i = 0; i < n; i++) if(inDegree[i] == 0) q.push(i);

    while(!q.empty()){
        int currentNode = q.front();
        topo.push_back(currentNode);
        q.pop();

        // now what I need to find out this nodes neigbors and 
        // then decrease there inDegree
        // if there inDegree is also 0, then push them to queue

        for(auto *outEdge : nodes[currentNode]->out_edges){
            int neighborId = outEdge->to->sccId;
            inDegree[neighborId]--;
            if(inDegree[neighborId] == 0) q.push(neighborId);
        }
    }
    return topo;
}

void CondensedGraph::assignLevels() {
    vector<int> topo = applyTopologicalSort();
    
    // Initialize all levels to 0
    for (auto& [id, node] : nodes) node->level = 0;
    
    // Process in topological order
    for (int sccId : topo) {
        CondensedNode* node = nodes[sccId];
        for (auto* outEdge : node->out_edges) {
            int neighborId = outEdge->to->sccId;
            nodes[neighborId]->level = max(
                nodes[neighborId]->level, 
                node->level + 1
            );
        }
    }
    
    // Propagate level down to individual GraphNodes
    for (auto& [id, node] : nodes) {
        for (GraphNode* member : node->scc_members) {
            member->level = node->level;
        }
    }
}

void CondensedGraph::getReachability(vector<bitset<maxN>> &reachabilityMatrix){
    vector<int> topo = applyTopologicalSort();
    int n = topo.size();
    for(int i = n - 1; i >= 0; i--){
        int currentNode = topo[i];

        for(auto *outEdge : nodes[currentNode]->out_edges){
            int neigborId = outEdge->to->sccId;
            reachabilityMatrix[currentNode][neigborId] = 1;
            reachabilityMatrix[currentNode] |= reachabilityMatrix[neigborId];
        }
    }
}

// GPT Generated.
void CondensedGraph::printGraph() {
    cout << "===== Condensed Graph =====\n";

    for (auto &pair : nodes) {
        CondensedNode* node = pair.second;

        cout << "\n[Node SCC ID: " << node->sccId << "]\n";

        cout << "  Members: ";
        for (auto *member : node->scc_members) {
            cout << member->process->getId() << " ";  // assuming GraphNode has id
        }
        cout << "\n";

        cout << "  Type: " << (node->isSeq ? "Sequential (cycle)" : "Combinational") << "\n";
        cout << "  Level: " << node->level << "\n";

        // Outgoing edges
        cout << "  Outgoing Edges:\n";
        for (auto *edge : node->out_edges) {
            cout << "    -> SCC " << edge->to->sccId;
            cout << " | signals: " << edge->signals.size();
            cout << "\n";
        }

        // Incoming edges
        cout << "  Incoming Edges:\n";
        for (auto *edge : node->in_edges) {
            cout << "    <- SCC " << edge->from->sccId;
            cout << " | signals: " << edge->signals.size();
            cout << "\n";
        }
    }

    cout << "\n===== Edge Summary =====\n";
    for (auto *edge : edges) {
        cout << "SCC " << edge->from->sccId
             << " -> SCC " << edge->to->sccId
             << " | signals: " << edge->signals.size()
             << "\n";
    }

    cout << "=========================\n";
}

void CondensedGraph::printReachabilityList(const vector<bitset<maxN>> &reach) {
    cout << "\n===== Reachability List =====\n";

    for (auto &[id, node] : nodes) {
        cout << "SCC " << id << " -> ";

        bool empty = true;
        for (size_t j = 0; j < nodes.size(); j++) {
            if (reach[id][j]) {
                cout << j << " ";
                empty = false;
            }
        }

        if (empty) cout << "(none)";
        cout << "\n";
    }

    cout << "==============================\n";
}
