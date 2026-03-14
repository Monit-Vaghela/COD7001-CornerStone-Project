#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <string>
#include <map>
#include <vector>

using namespace std;

class JsonParser {
public:
    bool parseCircuitFile(const string& filename);
    bool parseSimulationFile(const string& filename);

    // ---------------- Circuit Data ----------------

    // Entity metadata
    map<string, int> entityMetadata;
    vector<string> inputs;
    vector<string> outputs;
    vector<string> intermediates;
    
    // Architecture metadata
    map<string, int> architectureMetadata;

    // Signal Mapping structure
    struct SignalMapping {
        string internalSignal;
        string mappedSignal;
        string signalType;
    };

    vector<SignalMapping> signalMappings;

    // Behaviour structure
    struct Behaviour {
        string outputSignal;
        vector<string> gateExpressionInputs;
        vector<string> gatesUsed;
        string expression;
    };

    vector<Behaviour> behaviours;

    // ---------------- Simulation Data ----------------

    map<string, int> simulationMetadata;

    // timestamp -> { signal -> value }
    map<int, map<string, int>> simulationProcesses;

private:
    string readFile(const string& filename);

    int extractInt(const string& content, const string& key);

    vector<string> extractStringArray(
        const string& content,
        const string& key
    );
};

#endif