#include <compiler/lexer.h>
#include <compiler/parser.h>
#include <compiler/semantic.h>
#include <compiler/json_writer.h>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

static string readFile(const string& path) {
    ifstream f(path);
    if (!f) {
        cerr << "Error: cannot open file '" << path << "'\n";
        exit(1);
    }
    stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static string outputPath(const string& inputPath, const string& suffix) {
    size_t dot = inputPath.find_last_of('.');
    string base = (dot != string::npos) ? inputPath.substr(0, dot) : inputPath;
    return base + suffix;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <circuit.cir> <simulation.tes>\n";
        return 1;
    }

    string cirPath = argv[1];
    string tesPath = argv[2];

// ── Stage 1: Lex ────────────────────────
    Lexer lexer;
    auto cirTokens = lexer.tokenize(readFile(cirPath));
    auto tesTokens = lexer.tokenize(readFile(tesPath));

    // ── Stage 2: Parse ──────────────────────
    Parser parser;
    ProgramNode   program = parser.parseCir(cirTokens);
    SimulatorNode sim     = parser.parseTes(tesTokens);

    // ── Stage 2b: Load sub-entities ─────────
    // Get directory of the .cir file
    string cirDir = "";
    size_t lastSlash = cirPath.find_last_of("/\\");
    if (lastSlash != string::npos)
        cirDir = cirPath.substr(0, lastSlash + 1);

    map<string, ProgramNode> subEntities;
    for (const auto& subName : program.entity.uses) {
        string subPath = cirDir + subName + ".cir";
        string subSrc  = readFile(subPath);
        auto   subToks = lexer.tokenize(subSrc);
        ProgramNode subProgram = parser.parseCir(subToks);
        subEntities[subName] = subProgram;
    }

    // ── Stage 3: Semantic Analysis ──────────
    SemanticAnalyzer sem;
    sem.analyze(program, sim, subEntities);

    // ── Stage 4: Write JSON ─────────────────
    JsonWriter jw;
    jw.writeCir(program, outputPath(cirPath, "_cir.json"), subEntities);
    jw.writeTes(sim,     outputPath(tesPath, "_tes.json"));

    return 0;
}