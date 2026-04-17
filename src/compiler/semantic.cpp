#include <compiler/semantic.h>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <set>
#include <map>
#include <functional> 

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

void SemanticAnalyzer::checkArch(const EntityNode& entity, const ArchNode& arch,
                                  const map<string, ProgramNode>& subEntities) {
    if (arch.name != entity.name)
        error("architecture name '" + arch.name + "' does not match entity name '" + entity.name + "'");

    set<string> declaredSignals(arch.signals.begin(), arch.signals.end());
    set<string> inputPorts(entity.inputs.begin(), entity.inputs.end());
    set<string> outputPorts(entity.outputs.begin(), entity.outputs.end());

    map<string, string> signalToPort;
    set<string> inputMappedSignals;
    set<string> outputMappedSignals;

    for (const auto& entry : arch.signalMapping) {
        if (!declaredSignals.count(entry.signal))
            error("signal '" + entry.signal + "' in signalMapping is not declared");

        if (!inputPorts.count(entry.port) && !outputPorts.count(entry.port))
            error("port '" + entry.port + "' in signalMapping is not declared in entity");

        if (signalToPort.count(entry.signal))
            error("signal '" + entry.signal + "' mapped more than once");

        signalToPort[entry.signal] = entry.port;

        if (inputPorts.count(entry.port))
            inputMappedSignals.insert(entry.signal);
        else
            outputMappedSignals.insert(entry.signal);
    }

    for (const auto& p : entity.inputs)
        if (!any_of(signalToPort.begin(), signalToPort.end(),
                    [&](const pair<string,string>& kv){ return kv.second == p; }))
            error("input port '" + p + "' has no signal mapped");

    for (const auto& p : entity.outputs)
        if (!any_of(signalToPort.begin(), signalToPort.end(),
                    [&](const pair<string,string>& kv){ return kv.second == p; }))
            error("output port '" + p + "' has no signal mapped");

    // FIXED DRIVER TRACKING 
    set<string> behaviourDrivers;
    set<string> instanceDrivers;
    set<string> behaviourLHS;

    // Behaviour checks
    for (const auto& beh : arch.behaviours) {
        if (!declaredSignals.count(beh.outputSignal))
            error("behaviour LHS '" + beh.outputSignal + "' not declared");

        if (inputMappedSignals.count(beh.outputSignal))
            error("behaviour LHS '" + beh.outputSignal + "' is input-mapped");

        if (behaviourDrivers.count(beh.outputSignal))
            error("signal '" + beh.outputSignal + "' has multiple behaviour assignments");

        behaviourDrivers.insert(beh.outputSignal);
        checkExprSignals(beh.exprTree, declaredSignals, beh.outputSignal);
    }

    // Edge behaviours
    for (const auto& eb : arch.edgeBehaviours) {
        if (!declaredSignals.count(eb.outputSignal))
            error("edge behaviour output '" + eb.outputSignal + "' not declared");

        if (inputMappedSignals.count(eb.outputSignal))
            error("edge behaviour output '" + eb.outputSignal + "' is input-mapped");

        if (!declaredSignals.count(eb.inputSignal))
            error("edge behaviour input '" + eb.inputSignal + "' not declared");

        if (!declaredSignals.count(eb.clockSignal) && !inputMappedSignals.count(eb.clockSignal))
            error("clock signal '" + eb.clockSignal + "' not declared");

        behaviourDrivers.insert(eb.outputSignal);
    }

    // Instance checks + driver collection
    for (const auto& inst : arch.instances) {
        if (!subEntities.count(inst.entityName))
            error("instance '" + inst.instanceName + "' uses unknown entity '" + inst.entityName + "'");

        const EntityNode& subEntity = subEntities.at(inst.entityName).entity;

        set<string> subPorts;
        for (const auto& p : subEntity.inputs)  subPorts.insert(p);
        for (const auto& p : subEntity.outputs) subPorts.insert(p);

        set<string> subOutputs(subEntity.outputs.begin(), subEntity.outputs.end());

        for (const auto& binding : inst.bindings) {
            if (!subPorts.count(binding.subPort))
                error("port '" + binding.subPort + "' invalid in '" + inst.entityName + "'");

            if (!declaredSignals.count(binding.localSignal))
                error("signal '" + binding.localSignal + "' in instance '" + inst.instanceName + "' not declared");

            if (subOutputs.count(binding.subPort))
                instanceDrivers.insert(binding.localSignal);
        }
    }

    // CONFLICT CHECK
    for (const auto& sig : behaviourDrivers) {
        if (instanceDrivers.count(sig)) {
            error("signal '" + sig + "' driven by both behaviour and instance");
        }
    }

    // MERGE
    behaviourLHS = behaviourDrivers;
    behaviourLHS.insert(instanceDrivers.begin(), instanceDrivers.end());

    // OUTPUT CHECK (AFTER MERGE)
    for (const auto& sig : outputMappedSignals)
        if (!behaviourLHS.count(sig))
            error("output-mapped signal '" + sig + "' has no assignment");


    // Dangling signal detection
    for (const auto& sig : arch.signals) {
        bool used = false;

        for (const auto& entry : arch.signalMapping)
            if (entry.signal == sig) { used = true; break; }

        if (behaviourLHS.count(sig)) used = true;

        if (!used) {
            for (const auto& beh : arch.behaviours) {
                function<bool(const shared_ptr<ExprNode>&)> hasSignal =
                    [&](const shared_ptr<ExprNode>& node) -> bool {
                        if (!node) return false;
                        if (node->kind == ExprKind::LEAF) return node->name == sig;
                        return hasSignal(node->left) || hasSignal(node->right);
                    };
                if (hasSignal(beh.exprTree)) { used = true; break; }
            }
        }

        if (!used) {
            for (const auto& inst : arch.instances)
                for (const auto& binding : inst.bindings)
                    if (binding.localSignal == sig) { used = true; break; }
        }

        if (!used)
            error("signal '" + sig + "' is declared but never used");
    }
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
    for (const auto& proc : sim.processes)
        for (const auto& assignment : proc.assignments)
            if (!inputMappedSignals.count(assignment.first))
                error("signal '" + assignment.first + "' in process at timestamp " +
                      to_string(proc.timestamp) + " is not an input-mapped signal");
}

void SemanticAnalyzer::analyze(const ProgramNode& program, const SimulatorNode& sim,
                                const map<string, ProgramNode>& subEntities) {
    checkEntity(program.entity);
    checkArch(program.entity, program.arch, subEntities);
    checkSimulator(program.entity, program.arch, sim);
    cout << "Semantic analysis passed.\n";
}