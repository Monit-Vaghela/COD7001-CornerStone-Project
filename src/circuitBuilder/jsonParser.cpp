#include <circuitBuilder/jsonParser.h>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

// Read the entire content of a file into a string
string JsonParser::readFile(const string& filename) {
    ifstream file(filename);
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Extract an integer value associated with a key from the JSON content
int JsonParser::extractInt(const string& content, const string& key) {
    size_t pos = content.find(key);
    if (pos == string::npos) return 0;

    pos = content.find(":", pos);
    size_t end = content.find_first_of(",}", pos);
    string number = content.substr(pos + 1, end - pos - 1);
    return stoi(number);
}

// Extract an array of strings associated with a key from the JSON content
vector<string> JsonParser::extractStringArray(const string& content, const string& key) {
    vector<string> result;

    size_t pos = content.find(key);
    if (pos == string::npos) return result;

    pos = content.find("[", pos);
    size_t end = content.find("]", pos);

    string arrayContent = content.substr(pos + 1, end - pos - 1);

    string temp;
    for (char c : arrayContent) {
        if (c == '"') continue;
        if (c == ',' || c == ' ') {
            if (!temp.empty()) {
                result.push_back(temp);
                temp.clear();
            }
        } else {
            temp += c;
        }
    }
    if (!temp.empty())
        result.push_back(temp);

    return result;
}

// cir.json parsing
bool JsonParser::parseCircuitFile(const string& filename) {
    string content = readFile(filename);

    // Entity metadata
    entityMetadata["noOfInputPorts"] = extractInt(content, "noOfInputPorts");
    entityMetadata["noOfOutputPorts"] = extractInt(content, "noOfOutputPorts");

    // Inputs and Outputs
    inputs = extractStringArray(content, "\"inputs\"");
    outputs = extractStringArray(content, "\"outputs\"");

    // Architecture metadata
    architectureMetadata["noOfTotalGates"] = extractInt(content, "noOfTotalGates");
    architectureMetadata["noOfAndGates"] = extractInt(content, "noOfAndGates");
    architectureMetadata["noOforGates"] = extractInt(content, "noOforGates");
    architectureMetadata["noOfNotGates"] = extractInt(content, "noOfNotGates");
    architectureMetadata["noOfXorGates"] = extractInt(content, "noOfXorGates");
    architectureMetadata["noOfXnorGates"] = extractInt(content, "noOfXnorGates");
    architectureMetadata["noOfSignals"] = extractInt(content, "noOfSignals");

    size_t pos = content.find("\"expression\"");
    while (pos != string::npos) {
        Behaviour b;

        size_t start = content.find("\"", pos + 13);
        size_t end = content.find("\"", start + 1);
        b.expression = content.substr(start + 1, end - start - 1);

        behaviours.push_back(b);

        pos = content.find("\"expression\"", end);
    }

    return true;
}

// tes.json parsing
bool JsonParser::parseSimulationFile(const string& filename) {
    string content = readFile(filename);

    simulationMetadata["noOfProcesses"] = extractInt(content, "noOfProcesses");
    simulationMetadata["initialTimeStamp"] = extractInt(content, "initialTimeStamp");
    simulationMetadata["finalTimeStamp"] = extractInt(content, "finalTimeStamp");

    size_t pos = content.find("\"timestamp\"");
    while (pos != string::npos) {
        size_t colon = content.find(":", pos);
        size_t end = content.find(",", colon);
        int timestamp = stoi(content.substr(colon + 1, end - colon - 1));

        size_t signalStart = content.rfind("{", pos);
        size_t signalEnd = content.find(",", signalStart);
        string signalBlock = content.substr(signalStart, signalEnd - signalStart);

        size_t keyStart = signalBlock.find("\"");
        size_t keyEnd = signalBlock.find("\"", keyStart + 1);
        string signal = signalBlock.substr(keyStart + 1, keyEnd - keyStart - 1);

        size_t valColon = signalBlock.find(":", keyEnd);
        int value = stoi(signalBlock.substr(valColon + 1));

        simulationProcesses[timestamp][signal] = value;

        pos = content.find("\"timestamp\"", end);
    }

    return true;
}