#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <string>
#include <map>
#include <vector>

class JsonParser {
public:
    bool parseCircuitFile(const std::string& filename);
    bool parseSimulationFile(const std::string& filename);

    // Circuit data
    std::map<std::string, int> entityMetadata;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;

    std::map<std::string, int> architectureMetadata;
    std::map<std::string, std::string> signalMapping;

    struct Behaviour {
        std::string outputSignal;
        std::vector<std::string> gateInputs;
        std::vector<std::string> gatesUsed;
        std::string expression;
    };

    std::vector<Behaviour> behaviours;

    // Simulation data
    std::map<std::string, int> simulationMetadata;
    std::map<int, std::map<std::string, int>> simulationProcesses;

private:
    std::string readFile(const std::string& filename);

    int extractInt(const std::string& content, const std::string& key);
    std::vector<std::string> extractStringArray(const std::string& content, const std::string& key);
};

#endif