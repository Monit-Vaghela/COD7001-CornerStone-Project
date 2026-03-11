#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <string>
#include <map>
#include <vector>

class JsonParser {
public:
    bool parseCircuitFile(const std::string& filename);
    bool parseSimulationFile(const std::string& filename);

    // ---------------- Circuit Data ----------------

    // Entity metadata
    std::map<std::string, int> entityMetadata;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;

    // Architecture metadata
    std::map<std::string, int> architectureMetadata;

    // Signal Mapping structure
    struct SignalMapping {
        std::string internalSignal;
        std::string mappedSignal;
        std::string signalType;
    };

    std::vector<SignalMapping> signalMappings;

    // Behaviour structure
    struct Behaviour {
        std::string outputSignal;
        std::vector<std::string> gateExpressionInputs;
        std::vector<std::string> gatesUsed;
        std::string expression;
    };

    std::vector<Behaviour> behaviours;

    // ---------------- Simulation Data ----------------

    std::map<std::string, int> simulationMetadata;

    // timestamp -> { signal -> value }
    std::map<int, std::map<std::string, int>> simulationProcesses;

private:
    std::string readFile(const std::string& filename);

    int extractInt(const std::string& content, const std::string& key);

    std::vector<std::string> extractStringArray(
        const std::string& content,
        const std::string& key
    );
};

#endif