#include <compiler/semantic.h>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <set>
#include <map>

using namespace std;

void SemanticAnalyzer::error(const string& msg) {
    cerr << "Semantic error: " << msg << "\n";
    exit(1);
}

// Walk an expression tree and verify every leaf signal is in allowedSignals
void SemanticAnalyzer::checkExprSignals(const shared_ptr<ExprNode>& node,
                                         const set<string>& allowedSignals,
                                         const string& context) {
    if (!node) return;
    if (node->kind == ExprKind::LEAF) {
        if (!allowedSignals.count(node->name))
            error("signal '" + node->name + "' used in expression for '" + context + "' is not a declared signal");
        return;
    }
    checkExprSignals(node->left,  allowedSignals, context);
    checkExprSignals(node->right, allowedSignals, context);
}

void SemanticAnalyzer::checkEntity(const EntityNode& entity) {
    if (entity.name.empty())
        error("entity has no name");

    if (entity.inputs.empty())
        error("entity '" + entity.name + "' has no inputs");

    if (entity.outputs.empty())
        error("entity '" + entity.name + "' has no outputs");

    // No name clashes between inputs and outputs
    set<string> inputSet(entity.inputs.begin(), entity.inputs.end());
    for (const auto& o : entity.outputs)
        if (inputSet.count(o))
            error("port '" + o + "' appears in both inputs and outputs");
}

void SemanticAnalyzer::checkArch(const EntityNode& entity, const ArchNode& arch) {
    // Architecture name must match entity name
    if (arch.name != entity.name)
        error("architecture name '" + arch.name + "' does not match entity name '" + entity.name + "'");

    set<string> declaredSignals(arch.signals.begin(), arch.signals.end());
    set<string> inputPorts(entity.inputs.begin(), entity.inputs.end());
    set<string> outputPorts(entity.outputs.begin(), entity.outputs.end());

    // Build signal->port and signal->type maps from signalMapping
    map<string, string> signalToPort;
    set<string> inputMappedSignals;
    set<string> outputMappedSignals;

    for (const auto& entry : arch.signalMapping) {
        // Signal must be declared
        if (!declaredSignals.count(entry.signal))
            error("signal '" + entry.signal + "' in signalMapping is not declared in signals list");

        // Port must exist in entity
        if (!inputPorts.count(entry.port) && !outputPorts.count(entry.port))
            error("port '" + entry.port + "' in signalMapping is not declared in entity");

        if (signalToPort.count(entry.signal))
            error("signal '" + entry.signal + "' is mapped more than once in signalMapping");

        signalToPort[entry.signal] = entry.port;

        if (inputPorts.count(entry.port))
            inputMappedSignals.insert(entry.signal);
        else
            outputMappedSignals.insert(entry.signal);
    }

    // Every input port must be mapped
    for (const auto& p : entity.inputs)
        if (!any_of(signalToPort.begin(), signalToPort.end(),
                    [&](const pair<string,string>& kv){ return kv.second == p; }))
            error("input port '" + p + "' has no signal mapped to it");

    // Every output port must be mapped
    for (const auto& p : entity.outputs)
        if (!any_of(signalToPort.begin(), signalToPort.end(),
                    [&](const pair<string,string>& kv){ return kv.second == p; }))
            error("output port '" + p + "' has no signal mapped to it");

    // Behaviour checks
    set<string> behaviourLHS;
    for (const auto& beh : arch.behaviours) {
        // LHS must be a declared signal
        if (!declaredSignals.count(beh.outputSignal))
            error("behaviour LHS '" + beh.outputSignal + "' is not a declared signal");

        // LHS must NOT be an input-mapped signal
        if (inputMappedSignals.count(beh.outputSignal))
            error("behaviour LHS '" + beh.outputSignal + "' is mapped to an input port and cannot be assigned");

        if (behaviourLHS.count(beh.outputSignal))
            error("signal '" + beh.outputSignal + "' has more than one behaviour assignment");

        behaviourLHS.insert(beh.outputSignal);

        // RHS leaves must all be declared signals
        checkExprSignals(beh.exprTree, declaredSignals, beh.outputSignal);
    }

    // Every output-mapped signal must have a behaviour
    for (const auto& sig : outputMappedSignals)
        if (!behaviourLHS.count(sig))
            error("output-mapped signal '" + sig + "' has no behaviour assignment");
}

void SemanticAnalyzer::checkSimulator(const EntityNode& entity,
                                       const ArchNode& arch,
                                       const SimulatorNode& sim) {
    // Simulator name must match entity
    if (sim.name != entity.name)
        error("simulator name '" + sim.name + "' does not match entity name '" + entity.name + "'");

    // Build set of input-mapped signals
    set<string> inputPorts(entity.inputs.begin(), entity.inputs.end());
    set<string> inputMappedSignals;
    for (const auto& entry : arch.signalMapping)
        if (inputPorts.count(entry.port))
            inputMappedSignals.insert(entry.signal);

    // Every signal assigned in a process must be input-mapped
    for (const auto& proc : sim.processes) {
        for (const auto& assignment : proc.assignments) {
            if (!inputMappedSignals.count(assignment.first))
                error("signal '" + assignment.first + "' in process at timestamp " +
                    to_string(proc.timestamp) + " is not an input-mapped signal");

            if (assignment.second != 0 && assignment.second != 1)
                error("signal '" + assignment.first + "' has invalid value " +
                    to_string(assignment.second) + " — must be 0 or 1");
        }
    }
}

void SemanticAnalyzer::analyze(const ProgramNode& program, const SimulatorNode& sim) {
    checkEntity(program.entity);
    checkArch(program.entity, program.arch);
    checkSimulator(program.entity, program.arch, sim);
    cout << "Semantic analysis passed.\n";
}