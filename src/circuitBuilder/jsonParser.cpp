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
        if (c == ',' || c == ' ' || c == '\n' || c == '\r' || c == '\t') {
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
    intermediates = extractStringArray(content, "\"intermediates\"");

    // Architecture metadata
    architectureMetadata["noOfTotalGates"] = extractInt(content, "noOfTotalGates");
    architectureMetadata["noOfAndGates"] = extractInt(content, "noOfAndGates");
    architectureMetadata["noOforGates"] = extractInt(content, "noOforGates");
    architectureMetadata["noOfNotGates"] = extractInt(content, "noOfNotGates");
    architectureMetadata["noOfXorGates"] = extractInt(content, "noOfXorGates");
    architectureMetadata["noOfXnorGates"] = extractInt(content, "noOfXnorGates");
    architectureMetadata["noOfSignals"] = extractInt(content, "noOfSignals");

    size_t pos = content.find("\"Behaviour\"");
    if (pos != string::npos) {

        pos = content.find("{", pos);

        while (true) {

            size_t objStart = content.find("{", pos);
            if (objStart == string::npos) break;

            size_t objEnd = content.find("}", objStart);
            if (objEnd == string::npos) break;

            string block = content.substr(objStart, objEnd - objStart);

            Behaviour b;

            // -------- ProcessType --------
            size_t typeKey = block.find("\"ProcessType\"");
            if (typeKey != string::npos) {
                size_t start = block.find("\"", typeKey + 14);
                size_t end = block.find("\"", start + 1);
                b.processType = block.substr(start + 1, end - start - 1);
            }

            // -------- OutputSignal --------
            size_t outKey = block.find("\"OutputSignal\"");
            if (outKey != string::npos) {
                size_t start = block.find("\"", outKey + 14);
                size_t end = block.find("\"", start + 1);
                b.outputSignal = block.substr(start + 1, end - start - 1);
            }

            // ===============================
            // CASE 1: Gate
            // ===============================
            if (b.processType == "Gate") {

                b.gateExpressionInputs =
                    extractStringArray(block, "\"gateExpressionInputs\"");

                b.gatesUsed =
                    extractStringArray(block, "\"gatesUsed\"");

                size_t expKey = block.find("\"expression\"");
                if (expKey != string::npos) {
                    size_t start = block.find("\"", expKey + 12);
                    size_t end = block.find("\"", start + 1);
                    b.expression = block.substr(start + 1, end - start - 1);
                }
            }

            // ===============================
            // CASE 2: Behaviour (Sequential)
            // ===============================
            else if (b.processType == "Behaviour") {

                size_t inKey = block.find("\"InputSignal\"");
                if (inKey != string::npos) {
                    size_t start = block.find("\"", inKey + 13);
                    size_t end = block.find("\"", start + 1);
                    b.inputSignal = block.substr(start + 1, end - start - 1);
                }

                size_t edgeKey = block.find("\"edgetype\"");
                if (edgeKey != string::npos) {
                    size_t start = block.find("\"", edgeKey + 11);
                    size_t end = block.find("\"", start + 1);
                    b.edgeType = block.substr(start + 1, end - start - 1);
                }

                size_t clkKey = block.find("\"clockSignal\"");
                if (clkKey != string::npos) {
                    size_t start = block.find("\"", clkKey + 14);
                    size_t end = block.find("\"", start + 1);
                    b.clockSignal = block.substr(start + 1, end - start - 1);
                }
            }

            behaviours.push_back(b);
            pos = objEnd + 1;
        }
    }

    
    pos = content.find("\"SignalMapping\"");
    if (pos != string::npos) {

        size_t arrayStart = content.find("[", pos);
        size_t arrayEnd = content.find("]", arrayStart);

        size_t objPos = content.find("{", arrayStart);

        while (objPos != string::npos && objPos < arrayEnd) {

            JsonParser::SignalMapping sm;

            // key  ("x")
            size_t keyStart = content.find("\"", objPos);
            size_t keyEnd = content.find("\"", keyStart + 1);
            sm.internalSignal = content.substr(keyStart + 1, keyEnd - keyStart - 1);

            // value ("a")
            size_t valStart = content.find("\"", keyEnd + 1);
            size_t valEnd = content.find("\"", valStart + 1);
            sm.mappedSignal = content.substr(valStart + 1, valEnd - valStart - 1);

            // SignalType
            size_t typeKey = content.find("\"SignalType\"", valEnd);
            size_t typeStart = content.find("\"", typeKey + 12);
            size_t typeEnd = content.find("\"", typeStart + 1);
            sm.signalType = content.substr(typeStart + 1, typeEnd - typeStart - 1);

            signalMappings.push_back(sm);

            objPos = content.find("{", typeEnd);
        }
    }
    return true;
}

// tes.json parsing
bool JsonParser::parseSimulationFile(const string& filename) {
    string content = readFile(filename);

    simulationMetadata["noOfProcesses"] = extractInt(content, "noOfProcesses");
    simulationMetadata["initialTimeStamp"] = extractInt(content, "initialTimeStamp");
    simulationMetadata["finalTimeStamp"] = extractInt(content, "finalTimeStamp");
    
    // ----------- InitialState Parsing -----------
    size_t initPos = content.find("\"InitialState\"");
    if (initPos != string::npos) {

        size_t start = content.find("{", initPos);
        size_t end = content.find("}", start);

        string block = content.substr(start + 1, end - start - 1);

        size_t pos = 0;

        while (pos < block.size()) {

            // find key
            size_t keyStart = block.find("\"", pos);
            if (keyStart == string::npos) break;

            size_t keyEnd = block.find("\"", keyStart + 1);
            string signal = block.substr(keyStart + 1, keyEnd - keyStart - 1);

            // find value
            size_t colon = block.find(":", keyEnd);
            size_t comma = block.find(",", colon);

            string valueStr;
            if (comma == string::npos)
                valueStr = block.substr(colon + 1);
            else
                valueStr = block.substr(colon + 1, comma - colon - 1);

            int value = stoi(valueStr);

            initialState[signal] = value;

            if (comma == string::npos) break;
            pos = comma + 1;
        }
    }

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