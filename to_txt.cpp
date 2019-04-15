#include <numeric>

#include "cmdline.h"
#include "misc.hpp"

int main(int argc, char** argv) {
  std::ios::sync_with_stdio(false);

  cmdline::parser p;
  p.add<string>("cws_fn", 'i', "input file name of cws sketches", true);
  p.add<size_t>("num", 'n', "num vectors", false, 20);
  p.parse_check(argc, argv);

  auto cws_fn = p.get<string>("cws_fn");
  auto num = p.get<size_t>("num");

  std::ifstream ifs(cws_fn);
  if (!ifs) {
    std::cerr << "open error: " << cws_fn << endl;
    return 1;
  }

  auto dim = read_value<uint32_t>(ifs);
  cout << dim << '\n';

  for (size_t i = 0; i < num; ++i) {
    for (size_t j = 0; j < dim; ++j) {
      auto v = read_value<uint8_t>(ifs);
      cout << static_cast<uint32_t>(v) << ' ';
    }
    cout << '\n';
  }
  cout << "..." << endl;

  return 0;
}