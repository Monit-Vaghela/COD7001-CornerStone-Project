#ifndef JSON_WRITER_H
#define JSON_WRITER_H

#include <compiler/parser.h>
#include <string>
#include <vector>
#include <map>
#include <set>

struct FlatBehaviour {
    std::string              outputSignal;
    std::vector<std::string> inputs;
    std::string              gate;
    std::string              expression;
};

class JsonWriter {
public:
    void writeCir(const ProgramNode& program, const std::string& outPath);
    void writeTes(const SimulatorNode& sim,   const std::string& outPath);

private:
    // Flattening: converts nested ExprNode tree → list of single-gate FlatBehaviours
    // Intermediate signals get names y1, y2, y3...
    int yCounter;
    std::vector<FlatBehaviour> flatBehaviours;
    std::set<std::string>      intermediates;

    // Recursively flatten one expression tree.
    // Returns the signal name that holds the result of this node.
    std::string flatten(const std::shared_ptr<ExprNode>& node,
                        const std::string& preferredName);

    void collectLeafInputs(const std::shared_ptr<ExprNode>& node,
                           std::vector<std::string>& out);

    std::string nextY();
};

#endif