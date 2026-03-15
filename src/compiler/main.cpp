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

// Derive output path: strip directory and extension, append suffix.
static string outputPath(const string& inputPath, const string& suffix) {
    // find last '/' or use start
    size_t slash = inputPath.find_last_of("/\\");
    string base  = (slash == string::npos) ? inputPath : inputPath.substr(slash + 1);
    // strip extension
    size_t dot = base.find_last_of('.');
    if (dot != string::npos) base = base.substr(0, dot);
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

    // ── Stage 3: Semantic Analysis ──────────
    SemanticAnalyzer sem;
    sem.analyze(program, sim);

    // ── Stage 4: Write JSON ─────────────────
    JsonWriter jw;
    jw.writeCir(program, outputPath(cirPath, "_cir.json"));
    jw.writeTes(sim,     outputPath(tesPath, "_tes.json"));

    return 0;
}