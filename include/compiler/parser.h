#ifndef PARSER_H
#define PARSER_H

#include <compiler/lexer.h>
#include <string>
#include <vector>
#include <map>
#include <memory>


enum class ExprKind { LEAF, UNARY, BINARY };

struct ExprNode {
    ExprKind kind;

    // LEAF
    std::string name;

    // UNARY / BINARY
    std::string gate;                       // "and" | "or" | "not" | "xor" | "xnor"
    std::shared_ptr<ExprNode> left;         // nullptr for UNARY
    std::shared_ptr<ExprNode> right;

    // convenience constructors
    // Leaf
    static std::shared_ptr<ExprNode> makeLeaf(const std::string& n) {
        auto node   = std::make_shared<ExprNode>();
        node->kind  = ExprKind::LEAF;
        node->name  = n;
        return node;
    }
    // Unary  (not)
    static std::shared_ptr<ExprNode> makeUnary(const std::string& g,
                                                std::shared_ptr<ExprNode> operand) {
        auto node   = std::make_shared<ExprNode>();
        node->kind  = ExprKind::UNARY;
        node->gate  = g;
        node->right = operand;
        return node;
    }
    // Binary (and / or / xor / xnor)
    static std::shared_ptr<ExprNode> makeBinary(const std::string& g,
                                                  std::shared_ptr<ExprNode> l,
                                                  std::shared_ptr<ExprNode> r) {
        auto node   = std::make_shared<ExprNode>();
        node->kind  = ExprKind::BINARY;
        node->gate  = g;
        node->left  = l;
        node->right = r;
        return node;
    }
};

//  AST Nodes  —  .cir file

struct SignalMapEntry {
    std::string signal;   // internal signal name, e.g. "x"
    std::string port;     // entity port name,    e.g. "a"
};

struct BehaviourNode {
    std::string                  outputSignal;   // e.g. "mus"
    std::shared_ptr<ExprNode>    exprTree;       // full expression tree
};

struct EdgeBehaviourNode {
    std::string outputSignal;
    std::string inputSignal;
    std::string edgeType;      // "rising" or "falling"
    std::string clockSignal;
};

struct EntityNode {
    std::string              name;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
};

struct ArchNode {
    std::string                    name;
    std::vector<std::string>       signals;
    std::vector<SignalMapEntry>    signalMapping;
    std::vector<BehaviourNode>     behaviours;
    std::vector<EdgeBehaviourNode> edgeBehaviours;
};

// Top-level result for a .cir file
struct ProgramNode {
    EntityNode entity;
    ArchNode   arch;
};

//  AST Nodes  —  .tes file

struct ProcessNode {
    std::map<std::string, int> assignments;   // signal → 0 or 1
    int                        timestamp;
};

struct SimulatorNode {
    std::string              name;
    std::vector<ProcessNode> processes;
};

//  Parser

class Parser {
public:
    // Parse a .cir token list → ProgramNode
    ProgramNode   parseCir(const std::vector<Token>& tokens);

    // Parse a .tes token list → SimulatorNode
    SimulatorNode parseTes(const std::vector<Token>& tokens);

private:
    // ── Token stream state ───────────────────
    std::vector<Token> toks;
    int                pos;        // index of current token

    // Return current token without consuming it
    Token peek();

    // Assert current token matches expected type, consume and return it.
    // Prints error + exits if mismatch.
    Token consume(TokenType expected, const std::string& errorMsg);

    // Same but also checks the token's value matches expectedValue
    Token consume(TokenType expected,
                  const std::string& expectedValue,
                  const std::string& errorMsg);

    // True if current token is a GATE token
    bool isGate();


    // Parse ["a", "b", "c"]  →  vector of strings
    std::vector<std::string> parseStringList();


    EntityNode               parseEntity();
    ArchNode                 parseArch();
    std::vector<SignalMapEntry> parseSignalMapping();
    BehaviourNode            parseBehaviour();
    EdgeBehaviourNode        parseEdgeBehaviour();

    // Expression parsing (recursive descent)
    std::shared_ptr<ExprNode> parseExpression();   // handles binary chains
    std::shared_ptr<ExprNode> parseAtom();         // leaf | (expr) | not atom


    ProcessNode parseProcess();
};

#endif 