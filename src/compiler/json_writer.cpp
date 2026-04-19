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

void JsonWriter::collectLeafInputs(const shared_ptr<ExprNode>& node,
                                    vector<string>& out) {
    if (!node) return;
    if (node->kind == ExprKind::LEAF) {
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
        string operandSig = flatten(node->right, "");

        string outSig = preferredName.empty() ? nextY() : preferredName;
        if (preferredName.empty()) intermediates.insert(outSig);

        FlatBehaviour fb;
        fb.outputSignal = outSig;
        fb.inputs       = { operandSig };
        fb.gate         = node->gate;
        fb.expression   = "not(" + operandSig + ")";
        flatBehaviours.push_back(fb);
        return outSig;
    }

    // BINARY
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
//  Returns ALL signals that are internal to a
//  sub-entity: auto-generated y-intermediates
//  PLUS any named signal in arch.signals that
//  has no port mapping (e.g. Deff in edff.cir).
// ─────────────────────────────────────────────
static set<string> buildInternalSignals(
        const JsonWriter&           writer,
        const ArchNode&             arch,
        const map<string,string>&   sigToPort)
{
    set<string> internals = writer.intermediates;  // y1, y2, ...

    for (const auto& sig : arch.signals)
        if (!sigToPort.count(sig))
            internals.insert(sig);                 // Deff, etc.

    return internals;
}

// ─────────────────────────────────────────────
//  Remap one EdgeBehaviourNode through a
//  sigToPort + portToLocal mapping.
//  Anything in `internals` gets instPrefix.
// ─────────────────────────────────────────────
static EdgeBehaviourNode remapEdgeBehaviour(
        const EdgeBehaviourNode&    eb,
        const map<string,string>&   sigToPort,
        const map<string,string>&   portToLocal,
        const set<string>&          internals,
        const string&               instPrefix)
{
    EdgeBehaviourNode out = eb;

    auto remapSig = [&](string& sig) {
        string port = sigToPort.count(sig) ? sigToPort.at(sig) : "";
        if (!port.empty() && portToLocal.count(port))
            sig = portToLocal.at(port);
        else if (internals.count(sig))
            sig = instPrefix + sig;
    };

    remapSig(out.outputSignal);
    remapSig(out.inputSignal);
    remapSig(out.clockSignal);

    return out;
}

// ─────────────────────────────────────────────
//  cir.json writer
// ─────────────────────────────────────────────

void JsonWriter::writeCir(const ProgramNode& program, const string& outPath,
                           const map<string, ProgramNode>& subEntities) {
    const EntityNode& entity = program.entity;
    const ArchNode&   arch   = program.arch;

    // Reset state
    yCounter       = 0;
    flatBehaviours.clear();
    flatEdgeBehaviours.clear();
    intermediates.clear();

    // Flatten top-level combinational behaviours
    for (const auto& beh : arch.behaviours)
        flatten(beh.exprTree, beh.outputSignal);

    // ── Inline instances ──────────────────────────────────────────────────────
    for (const auto& inst : arch.instances) {
        if (!subEntities.count(inst.entityName)) continue;

        map<string, string> portToLocal;
        for (const auto& binding : inst.bindings)
            portToLocal[binding.subPort] = binding.localSignal;

        const ProgramNode& sub = subEntities.at(inst.entityName);

        // Build sub signal->port map
        map<string, string> subSigToPort;
        for (const auto& entry : sub.arch.signalMapping)
            subSigToPort[entry.signal] = entry.port;

        // ── Build subWriter ──────────────────────────────────────────────────
        JsonWriter subWriter;
        subWriter.yCounter = yCounter;

        // Flatten sub's own combinational behaviours
        for (const auto& beh : sub.arch.behaviours)
            subWriter.flatten(beh.exprTree, beh.outputSignal);

        // Collect sub's own edge behaviours
        for (const auto& eb : sub.arch.edgeBehaviours)
            subWriter.flatEdgeBehaviours.push_back(eb);

        // ── Recursively inline sub's instances ───────────────────────────────
        for (const auto& subInst : sub.arch.instances) {
            if (!subEntities.count(subInst.entityName)) continue;

            map<string, string> subPortToLocal;
            for (const auto& binding : subInst.bindings)
                subPortToLocal[binding.subPort] = binding.localSignal;

            const ProgramNode& subSub = subEntities.at(subInst.entityName);

            JsonWriter subSubWriter;
            subSubWriter.yCounter = subWriter.yCounter;

            // Flatten subSub combinational behaviours
            for (const auto& beh : subSub.arch.behaviours)
                subSubWriter.flatten(beh.exprTree, beh.outputSignal);

            // Collect subSub edge behaviours
            for (const auto& eb : subSub.arch.edgeBehaviours)
                subSubWriter.flatEdgeBehaviours.push_back(eb);

            // Build subSub signal->port map
            map<string, string> subSubSigToPort;
            for (const auto& entry : subSub.arch.signalMapping)
                subSubSigToPort[entry.signal] = entry.port;

            // ALL internals of subSub (y-names + named non-port signals like Deff)
            set<string> subSubInternals =
                buildInternalSignals(subSubWriter, subSub.arch, subSubSigToPort);

            const string subInstPrefix = subInst.instanceName + "_";

            // Remap subSub gate behaviours into sub namespace
            for (auto fb : subSubWriter.flatBehaviours) {
                string outPort = subSubSigToPort.count(fb.outputSignal) ?
                                 subSubSigToPort[fb.outputSignal] : "";
                if (!outPort.empty() && subPortToLocal.count(outPort))
                    fb.outputSignal = subPortToLocal[outPort];
                else if (subSubInternals.count(fb.outputSignal))
                    fb.outputSignal = subInstPrefix + fb.outputSignal;

                for (auto& inp : fb.inputs) {
                    string inPort = subSubSigToPort.count(inp) ?
                                    subSubSigToPort[inp] : "";
                    if (!inPort.empty() && subPortToLocal.count(inPort))
                        inp = subPortToLocal[inPort];
                    else if (subSubInternals.count(inp))
                        inp = subInstPrefix + inp;
                }

                if (fb.inputs.size() == 1)
                    fb.expression = fb.gate + "(" + fb.inputs[0] + ")";
                else if (fb.inputs.size() == 2)
                    fb.expression = fb.gate + "(" + fb.inputs[0] + ", " + fb.inputs[1] + ")";

                subWriter.flatBehaviours.push_back(fb);
            }

            // Remap subSub edge behaviours into sub namespace
            for (const auto& eb : subSubWriter.flatEdgeBehaviours) {
                EdgeBehaviourNode remapped = remapEdgeBehaviour(
                    eb, subSubSigToPort, subPortToLocal,
                    subSubInternals, subInstPrefix);
                subWriter.flatEdgeBehaviours.push_back(remapped);
            }

            subWriter.yCounter = subSubWriter.yCounter;
            for (const auto& im : subSubInternals)
                subWriter.intermediates.insert(subInstPrefix + im);
        }

        // ALL internals of sub (after sub's own instances have been inlined)
        set<string> subInternals =
            buildInternalSignals(subWriter, sub.arch, subSigToPort);

        const string instPrefix = inst.instanceName + "_";

        // Remap sub gate behaviours into parent namespace
        for (auto fb : subWriter.flatBehaviours) {
            string outPort = subSigToPort.count(fb.outputSignal) ?
                             subSigToPort[fb.outputSignal] : "";
            if (!outPort.empty() && portToLocal.count(outPort))
                fb.outputSignal = portToLocal[outPort];
            else if (subInternals.count(fb.outputSignal))
                fb.outputSignal = instPrefix + fb.outputSignal;

            for (auto& inp : fb.inputs) {
                string inPort = subSigToPort.count(inp) ? subSigToPort[inp] : "";
                if (!inPort.empty() && portToLocal.count(inPort))
                    inp = portToLocal[inPort];
                else if (subInternals.count(inp))
                    inp = instPrefix + inp;
            }

            if (fb.inputs.size() == 1)
                fb.expression = fb.gate + "(" + fb.inputs[0] + ")";
            else if (fb.inputs.size() == 2)
                fb.expression = fb.gate + "(" + fb.inputs[0] + ", " + fb.inputs[1] + ")";

            flatBehaviours.push_back(fb);
        }

        // Remap sub edge behaviours into parent namespace
        for (const auto& eb : subWriter.flatEdgeBehaviours) {
            EdgeBehaviourNode remapped = remapEdgeBehaviour(
                eb, subSigToPort, portToLocal, subInternals, instPrefix);
            flatEdgeBehaviours.push_back(remapped);
        }

        yCounter = subWriter.yCounter;
        for (const auto& im : subInternals)
            intermediates.insert(instPrefix + im);
    }

    // Top-level edge behaviours (no remapping needed)
    for (const auto& eb : arch.edgeBehaviours)
        flatEdgeBehaviours.push_back(eb);

    // ── Signal type map ───────────────────────────────────────────────────────
    set<string> inputPorts(entity.inputs.begin(), entity.inputs.end());
    map<string, string> signalToPort;
    map<string, string> signalType;
    for (const auto& entry : arch.signalMapping) {
        signalToPort[entry.signal] = entry.port;
        signalType[entry.signal]   = inputPorts.count(entry.port) ? "input" : "output";
    }

    set<string> mappedSignals;
    for (const auto& entry : arch.signalMapping)
        mappedSignals.insert(entry.signal);
    for (const auto& sig : arch.signals)
        if (!mappedSignals.count(sig))
            intermediates.insert(sig);

    // ── Gate counts ───────────────────────────────────────────────────────────
    int totalGates = 0, andGates = 0, orGates = 0, notGates = 0,
        xorGates = 0, xnorGates = 0;
    for (const auto& fb : flatBehaviours) {
        totalGates++;
        if      (fb.gate == "and")  andGates++;
        else if (fb.gate == "or")   orGates++;
        else if (fb.gate == "not")  notGates++;
        else if (fb.gate == "xor")  xorGates++;
        else if (fb.gate == "xnor") xnorGates++;
    }

    // ── Write JSON ────────────────────────────────────────────────────────────
    ofstream f(outPath);
    if (!f) { cerr << "Cannot open output: " << outPath << "\n"; exit(1); }

    f << "{\n";
    f << "    \"entity\" : {\n";
    f << "        \"metadata\" : {\n";
    f << "            \"noOfInputPorts\" : "  << entity.inputs.size()  << ",\n";
    f << "            \"noOfOutputPorts\" : " << entity.outputs.size() << "\n";
    f << "        },\n\n";

    f << "        \"inputs\" : [";
    for (int i = 0; i < (int)entity.inputs.size(); i++)
        f << "\"" << entity.inputs[i] << "\""
          << (i+1 < (int)entity.inputs.size() ? ", " : "");
    f << "],\n";

    f << "        \"outputs\" : [";
    for (int i = 0; i < (int)entity.outputs.size(); i++)
        f << "\"" << entity.outputs[i] << "\""
          << (i+1 < (int)entity.outputs.size() ? ", " : "");
    f << "]";

    if (!intermediates.empty()) {
        f << ",\n        \"intermediates\" : [";
        vector<string> ims(intermediates.begin(), intermediates.end());
        sort(ims.begin(), ims.end(), [](const string& a, const string& b){
            try { return stoi(a.substr(1)) < stoi(b.substr(1)); }
            catch (...) { return a < b; }
        });
        for (int i = 0; i < (int)ims.size(); i++)
            f << "\"" << ims[i] << "\"" << (i+1 < (int)ims.size() ? ", " : "");
        f << "]";
    }

    f << "\n    },\n\n";

    f << "    \"Architecture\" : {\n";
    f << "        \"metadata\" : {\n";
    f << "            \"noOfTotalGates\" : " << totalGates  << ",\n";
    f << "            \"noOfAndGates\" : "   << andGates    << ",\n";
    f << "            \"noOforGates\" : "    << orGates     << ",\n";
    f << "            \"noOfNotGates\" : "   << notGates    << ",\n";
    f << "            \"noOfXorGates\" : "   << xorGates    << ",\n";
    f << "            \"noOfXnorGates\" : "  << xnorGates   << ",\n";
    f << "            \"noOfSignals\" : "    << arch.signals.size() << "\n";
    f << "        },\n\n";

    f << "        \"SignalMapping\" : \n            [\n";
    for (int i = 0; i < (int)arch.signalMapping.size(); i++) {
        const auto& entry = arch.signalMapping[i];
        f << "                {\"" << entry.signal << "\" : \"" << entry.port
          << "\", \"SignalType\" : \"" << signalType[entry.signal] << "\"}";
        if (i+1 < (int)arch.signalMapping.size()) f << ",";
        f << "\n";
    }
    f << "            ]\n        ,\n\n";

    const int totalEntries = (int)flatBehaviours.size() +
                             (int)flatEdgeBehaviours.size();
    int entryIndex = 0;

    f << "        \"Behaviour\" : [\n";

    for (int i = 0; i < (int)flatBehaviours.size(); i++, entryIndex++) {
        const auto& fb = flatBehaviours[i];
        f << "            {\n";
        f << "                \"ProcessType\" : \"Gate\",\n";
        f << "                \"OutputSignal\" : \"" << fb.outputSignal << "\",\n";
        f << "                \"gateExpressionInputs\" : [";
        for (int j = 0; j < (int)fb.inputs.size(); j++)
            f << "\"" << fb.inputs[j] << "\""
              << (j+1 < (int)fb.inputs.size() ? ", " : "");
        f << "],\n";
        f << "                \"gatesUsed\" : [\"" << fb.gate << "\"],\n";
        f << "                \"expression\" : \"" << fb.expression << "\"\n";
        f << "            }";
        if (entryIndex + 1 < totalEntries) f << ",";
        f << "\n";
    }

    for (int i = 0; i < (int)flatEdgeBehaviours.size(); i++, entryIndex++) {
        const auto& eb = flatEdgeBehaviours[i];
        f << "            {\n";
        f << "                \"ProcessType\" : \"Behaviour\",\n";
        f << "                \"OutputSignal\" : \"" << eb.outputSignal << "\",\n";
        f << "                \"InputSignal\" : \"" << eb.inputSignal << "\",\n";
        f << "                \"edgetype\" : \"" << eb.edgeType << "\",\n";
        f << "                \"clockSignal\" : \"" << eb.clockSignal << "\"\n";
        f << "            }";
        if (entryIndex + 1 < totalEntries) f << ",";
        f << "\n";
    }

    f << "        ]\n";
    f << "    }\n";
    f << "}\n";

    cout << "Written: " << outPath << "\n";
}

// ─────────────────────────────────────────────
//  tes.json writer
// ─────────────────────────────────────────────

void JsonWriter::writeTes(const SimulatorNode& sim, const string& outPath) {
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