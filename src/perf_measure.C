#include "perf_measure.hpp"

#include <fstream>
#include <sstream>

namespace measure_perf {

std::string LogPrefix;

std::unordered_map<std::string, int> parseHeader(const std::string &path) {
  std::ifstream fin(path);
  std::unordered_map<std::string, int> order;
  std::string line;

  if (!std::getline(fin, line))
    return {};
  std::stringstream ss(line);
  std::string word;
  int orderID = 0;
  while (std::getline(ss, word, ',')) {
    order[word] = orderID++;
  }
  return order;
}

void logOne(const Perf &perf, const std::vector<PerfResult> &round,
            const std::string &path) {

  std::string usePath = path;
  if (!LogPrefix.empty()) {
    usePath = LogPrefix + path;
  }
  std::unordered_map<std::string, int> headers = parseHeader(usePath);
  std::ofstream fout(usePath, std::ios::app);

  int orderID = 0;
  if (headers.empty()) {
    bool first = true;
    for (const auto &name : perf.getMetricNames()) {
      if (!first) {
        fout << ",";
      }
      fout << name;
      first = false;
      // Also add header
      headers[name] = orderID++;
    }
    fout << std::endl;
  }
  std::vector<uint64_t> values(round.size(), 0);
  for (const auto &measure : round) {
    values[headers[measure.name]] = measure.measure;
  }

  bool first = true;
  for (const auto &val : values) {
    if (!first)
      fout << ",";
    fout << val;
    first = false;
  }
  fout << std::endl;
}

} // namespace measure_perf
