#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <compiler/parser.h>
#include <string>
#include <set>

class SemanticAnalyzer {
public:
    void analyze(const ProgramNode& program, const SimulatorNode& sim,
                 const std::map<std::string, ProgramNode>& subEntities);

private:
    void checkEntity(const EntityNode& entity);
    void checkArch(const EntityNode& entity, const ArchNode& arch,
                   const std::map<std::string, ProgramNode>& subEntities);
    void checkSimulator(const EntityNode& entity, const ArchNode& arch, const SimulatorNode& sim);
    void checkExprSignals(const std::shared_ptr<ExprNode>& node,
                          const std::set<std::string>& allowedSignals,
                          const std::string& context);
    void error(const std::string& msg);
};

#endif