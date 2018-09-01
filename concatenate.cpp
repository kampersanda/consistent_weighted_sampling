#include <random>

#include "cmdline.h"
#include "common.hpp"

int main(int argc, char** argv) {
  cmdline::parser p;
  p.add<std::string>("data_list", 'd', "list of input file names of data vectors", true);
  p.add<std::string>("output_fn", 'o', "output file name of cancatenated sampled vectors", true);
  p.parse_check(argc, argv);

  auto data_list = p.get<std::string>("data_list");
  auto output_fn = p.get<std::string>("output_fn") + ".cws";

  std::vector<std::string> data_fns;
  {
    std::ifstream ifs(data_list);
    if (!ifs) {
      std::cerr << "open error: " << data_list << "\n";
      exit(1);
    }
    for (std::string line; std::getline(ifs, line);) {
      data_fns.push_back(line);
    }
  }

  if (data_fns.empty()) {
    std::cerr << data_list << " is empty\n";
    return 1;
  }

  std::ofstream ofs(output_fn);
  if (!ofs) {
    std::cerr << "open error: " << output_fn << "\n";
    exit(1);
  }

  for (size_t i = 0; i < data_fns.size(); ++i) {
    std::string cws_fn = data_fns[i] + ".cws";
    std::ifstream ifs(cws_fn);
    if (!ifs) {
      std::cerr << "open error: " << cws_fn << "\n";
      exit(1);
    }

    std::string line;
    if (i != 0) {
      // skip header
      std::getline(ifs, line);
    }
    while (std::getline(ifs, line)) {
      ofs << line << '\n';
    }
  }

  return 0;
}
