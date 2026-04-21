#include <circuitBuilder/utils/Dashboardjson.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace DashboardJSON {
// ─── helpers ──────────────────────────────────────────────────────────────
static void writeIndent(std::ofstream& f, int n) {
for (int i = 0; i < n; i++) f << "  ";
}
// ─── sequential export ────────────────────────────────────────────────────
void exportSeq(const std::vector<SeqDeltaRecord>& records,
const std::string& path)
{
std::ofstream f(path);
if (!f.is_open()) {
std::cerr << "[DashboardJSON] Cannot open " << path << " for writing\n";
return;
    }
f << "[\n";
for (int i = 0; i < (int)records.size(); i++) {
const auto& r = records[i];
writeIndent(f, 1);
f << "{"
<< "\"time\":"      << r.time      << ","
<< "\"delta\":"     << r.delta     << ","
<< "\"events\":"    << r.events    << ","
<< "\"elapsedUs\":" << r.elapsedUs         // ← added
<< "}";
if (i + 1 < (int)records.size()) f << ",";
f << "\n";
    }
f << "]\n";
std::cout << "[DashboardJSON] Wrote " << records.size()
<< " sequential records to " << path << "\n";
}
// ─── parallel export ──────────────────────────────────────────────────────
void exportPar(const std::vector<ParDeltaRecord>& records,
const std::string& path)
{
std::ofstream f(path);
if (!f.is_open()) {
std::cerr << "[DashboardJSON] Cannot open " << path << " for writing\n";
return;
    }
f << "[\n";
for (int i = 0; i < (int)records.size(); i++) {
const auto& r = records[i];
writeIndent(f, 1);
f << "{"
<< "\"time\":"         << r.time         << ","
<< "\"delta\":"        << r.delta         << ","
<< "\"ompThreshold\":" << r.ompThreshold  << ","
<< "\"elapsedUs\":"    << r.elapsedUs     << ","  // ← added
<< "\"levelMap\":{";
bool first = true;
for (const auto& [level, count] : r.levelMap) {
if (!first) f << ",";
f << "\"" << level << "\":" << count;
first = false;
        }
f << "}}";
if (i + 1 < (int)records.size()) f << ",";
f << "\n";
    }
f << "]\n";
std::cout << "[DashboardJSON] Wrote " << records.size()
<< " parallel records to " << path << "\n";
}
} // namespace DashboardJSON
