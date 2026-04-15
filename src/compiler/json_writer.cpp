#include <compiler/json_writer.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <algorithm>
#include <cstdlib>
#include <climits>

using namespace std;

string JsonWriter::nextY() {
    return "y" + to_string(++yCounter);
}

// Collect all leaf signal names from a subtree (for gateExpressionInputs)
void JsonWriter::collectLeafInputs(const shared_ptr<ExprNode>& node,
                                    vector<string>& out) {
    if (!node) return;
    if (node->kind == ExprKind::LEAF) {
        // avoid duplicates
        if (find(out.begin(), out.end(), node->name) == out.end())
            out.push_back(node->name);
        return;
    }
    collectLeafInputs(node->left,  out);
    collectLeafInputs(node->right, out);
}

string JsonWriter::flatten(const shared_ptr<ExprNode>& node,
                            const string& preferredName) {
    if (node->kind == ExprKind::LEAF) {
        return node->name;
    }

    if (node->kind == ExprKind::UNARY) {
        // Flatten the single operand first
        string operandSig = flatten(node->right, "");

        string outSig = preferredName.empty() ? nextY() : preferredName;
        if (preferredName.empty()) intermediates.insert(outSig);

        FlatBehaviour fb;
        fb.outputSignal = outSig;
        fb.inputs       = { operandSig };
        fb.gate         = node->gate;   // "not"
        fb.expression   = "not(" + operandSig + ")";
        flatBehaviours.push_back(fb);
        return outSig;
    }

    // BINARY: flatten left and right subtrees first
    string leftSig  = flatten(node->left,  "");
    string rightSig = flatten(node->right, "");

    string outSig = preferredName.empty() ? nextY() : preferredName;
    if (preferredName.empty()) intermediates.insert(outSig);

    FlatBehaviour fb;
    fb.outputSignal = outSig;
    fb.inputs       = { leftSig, rightSig };
    fb.gate         = node->gate;
    fb.expression   = node->gate + "(" + leftSig + ", " + rightSig + ")";
    flatBehaviours.push_back(fb);
    return outSig;
}

// ─────────────────────────────────────────────
//  cir.json writer
// ─────────────────────────────────────────────

void JsonWriter::writeCir(const ProgramNode& program, const string& outPath) {
    const EntityNode& entity = program.entity;
    const ArchNode&   arch   = program.arch;

    // Reset flattening state
    yCounter       = 0;
    flatBehaviours.clear();
    intermediates.clear();

    // Flatten every behaviour
    for (const auto& beh : arch.behaviours)
        flatten(beh.exprTree, beh.outputSignal);

    // Determine signal type for signalMapping
    set<string> inputPorts(entity.inputs.begin(), entity.inputs.end());
    map<string, string> signalToPort;
    map<string, string> signalType;
    for (const auto& entry : arch.signalMapping) {
        signalToPort[entry.signal] = entry.port;
        signalType[entry.signal]   = inputPorts.count(entry.port) ? "input" : "output";
    }

    // Count gates
    int totalGates = 0, andGates = 0, orGates = 0, notGates = 0, xorGates = 0, xnorGates = 0;
    for (const auto& fb : flatBehaviours) {
        totalGates++;
        if      (fb.gate == "and")  andGates++;
        else if (fb.gate == "or")   orGates++;
        else if (fb.gate == "not")  notGates++;
        else if (fb.gate == "xor")  xorGates++;
        else if (fb.gate == "xnor") xnorGates++;
    }

    ofstream f(outPath);
    if (!f) { cerr << "Cannot open output: " << outPath << "\n"; exit(1); }

    f << "{\n";

    // entity
    f << "    \"entity\" : {\n";
    f << "        \"metadata\" : {\n";
    f << "            \"noOfInputPorts\" : "  << entity.inputs.size()  << ",\n";
    f << "            \"noOfOutputPorts\" : " << entity.outputs.size() << "\n";
    f << "        },\n\n";

    // inputs
    f << "        \"inputs\" : [";
    for (int i = 0; i < (int)entity.inputs.size(); i++)
        f << "\"" << entity.inputs[i] << "\"" << (i+1 < (int)entity.inputs.size() ? ", " : "");
    f << "],\n";

    // outputs
    f << "        \"outputs\" : [";
    for (int i = 0; i < (int)entity.outputs.size(); i++)
        f << "\"" << entity.outputs[i] << "\"" << (i+1 < (int)entity.outputs.size() ? ", " : "");
    f << "]";

    // intermediates (only if any exist)
    if (!intermediates.empty()) {
        f << ",\n        \"intermediates\" : [";
        vector<string> ims(intermediates.begin(), intermediates.end());
        // sort so y1, y2, y3... come out in order
        sort(ims.begin(), ims.end(), [](const string& a, const string& b){
            // numeric sort on the number after 'y'
            try { return stoi(a.substr(1)) < stoi(b.substr(1)); }
            catch (...) { return a < b; }
        });
        for (int i = 0; i < (int)ims.size(); i++)
            f << "\"" << ims[i] << "\"" << (i+1 < (int)ims.size() ? ", " : "");
        f << "]";
    }

    f << "\n    },\n\n";

    // ── Architecture ─────────────────────────
    f << "    \"Architecture\" : {\n";
    f << "        \"metadata\" : {\n";
    f << "            \"noOfTotalGates\" : " << totalGates  << ",\n";
    f << "            \"noOfAndGates\" : "   << andGates   << ",\n";
    f << "            \"noOforGates\" : "    << orGates    << ",\n";
    f << "            \"noOfNotGates\" : "   << notGates   << ",\n";
    f << "            \"noOfXorGates\" : "   << xorGates   << ",\n";
    f << "            \"noOfXnorGates\" : "  << xnorGates  << ",\n";
    f << "            \"noOfSignals\" : "    << arch.signals.size() << "\n";
    f << "        },\n\n";

    // SignalMapping
    f << "        \"SignalMapping\" : \n            [\n";
    for (int i = 0; i < (int)arch.signalMapping.size(); i++) {
        const auto& entry = arch.signalMapping[i];
        f << "                {\"" << entry.signal << "\" : \"" << entry.port
          << "\", \"SignalType\" : \"" << signalType[entry.signal] << "\"}";
        if (i+1 < (int)arch.signalMapping.size()) f << ",";
        f << "\n";
    }
    f << "            ]\n        ,\n\n";

    // Behaviour
    f << "        \"Behaviour\" : [\n";
    for (int i = 0; i < (int)flatBehaviours.size(); i++) {
        const auto& fb = flatBehaviours[i];
        f << "            {\n";
        f << "                \"ProcessType\" : \"Gate\",\n";
        f << "                \"OutputSignal\" : \"" << fb.outputSignal << "\",\n";
        f << "                \"gateExpressionInputs\" : [";
        for (int j = 0; j < (int)fb.inputs.size(); j++)
            f << "\"" << fb.inputs[j] << "\"" << (j+1 < (int)fb.inputs.size() ? ", " : "");
        f << "],\n";
        f << "                \"gatesUsed\" : [\"" << fb.gate << "\"],\n";
        f << "                \"expression\" : \"" << fb.expression << "\"\n";
        f << "            }";
        if (i+1 < (int)flatBehaviours.size()) f << ",";
        f << "\n";
    }
    
    // Edge behaviour entries
    for (int i = 0; i < (int)program.arch.edgeBehaviours.size(); i++) {
        const auto& eb = program.arch.edgeBehaviours[i];
        if (!flatBehaviours.empty() || i > 0) f << ",\n";
        f << "            {\n";
        f << "                \"ProcessType\" : \"Behaviour\",\n";
        f << "                \"OutputSignal\" : \"" << eb.outputSignal << "\",\n";
        f << "                \"InputSignal\" : \"" << eb.inputSignal << "\",\n";
        f << "                \"edgetype\" : \"" << eb.edgeType << "\",\n";
        f << "                \"clockSignal\" : \"" << eb.clockSignal << "\"\n";
        f << "            }";
    }
    if (!program.arch.edgeBehaviours.empty()) f << "\n";
    f << "        ]\n";
    f << "    }\n";
    f << "}\n";

    cout << "Written: " << outPath << "\n";
}

// ─────────────────────────────────────────────
//  tes.json writer
// ─────────────────────────────────────────────

void JsonWriter::writeTes(const SimulatorNode& sim, const string& outPath) {
    // Explode each process into one entry per signal assignment
    struct Entry { string signal; int value; int timestamp; };
    vector<Entry> entries;

    int minTs = INT_MAX, maxTs = INT_MIN;

    for (const auto& proc : sim.processes) {
        for (const auto& assignment : proc.assignments) {
            entries.push_back({ assignment.first, assignment.second, proc.timestamp });
        }
        minTs = min(minTs, proc.timestamp);
        maxTs = max(maxTs, proc.timestamp);
    }

    // Sort by timestamp, then signal name for consistent output
    sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b){
        if (a.timestamp != b.timestamp) return a.timestamp < b.timestamp;
        return a.signal < b.signal;
    });

    ofstream f(outPath);
    if (!f) { cerr << "Cannot open output: " << outPath << "\n"; exit(1); }

    f << "{\n";
    f << "    \"simulation\" : {\n";
    f << "        \"metadata\" : {\n";
    f << "            \"noOfProcesses\" : "   << entries.size() << ",\n";
    f << "            \"initialTimeStamp\" : " << (entries.empty() ? 0 : minTs) << ",\n";
    f << "            \"finalTimeStamp\" : "   << (entries.empty() ? 0 : maxTs) << "\n";
    f << "        },\n";
    f << "        \"Processes\" : \n        [\n";

    for (int i = 0; i < (int)entries.size(); i++) {
        const auto& e = entries[i];
        f << "            {\"" << e.signal << "\" : " << e.value
          << ", \"timestamp\" : " << e.timestamp << "}";
        if (i+1 < (int)entries.size()) f << ",";
        f << "\n";
    }

    f << "        ]\n";
    f << "    }\n";
    f << "}\n";

    cout << "Written: " << outPath << "\n";
}