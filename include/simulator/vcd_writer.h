#ifndef VCD_WRITER_H
#define VCD_WRITER_H

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

class VCDWriter {
private:
    std::ofstream file;
    std::unordered_map<std::string, std::string> signalToId;
    int idCounter;
    bool headerClosed;

    std::string generateId() {
        std::string id;
        int x = idCounter++;

        do {
            char c = 33 + (x % 94);
            id += c;
            x /= 94;
        } while (x > 0);

        return id;
    }

public:
    VCDWriter(const std::string& filename)
        : idCounter(0), headerClosed(false) {
        file.open(filename);
        
        file << "$date\n";
        file << "    today\n";
        file << "$end\n";

        file << "$version\n";
        file << "    custom circuit simulator\n";
        file << "$end\n";

        file << "$timescale 1ns $end\n";
        file << "$scope module logic $end\n";
    }

    ~VCDWriter() {
        if (file.is_open()) {
            file.flush();
            file.close();
        }
    }

    void registerSignal(const std::string& name) {
        if (headerClosed) return;

        std::string id = generateId();
        signalToId[name] = id;

        file << "$var wire 1 " << id << " " << name << " $end\n";
    }

    void endDefinitions() {
        if (headerClosed) return;

        file << "$upscope $end\n";
        file << "$enddefinitions $end\n";
        headerClosed = true;
    }

    void dumpTime(int time) {
        if (!headerClosed) return;
        file << "#" << time << "\n";
    }

    void dumpValue(const std::string& name, bool value) {
        if (!headerClosed) return;

        auto it = signalToId.find(name);
        if (it == signalToId.end()) return;

        file << (value ? "1" : "0") << it->second << "\n";
    }

    void dumpAll(const std::vector<std::pair<std::string, bool>>& signals) {
        if (!headerClosed) return;

        for (const auto& s : signals) {
            dumpValue(s.first, s.second);
        }
    }
};

#endif