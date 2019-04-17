#include <numeric>

#include "cmdline.h"
#include "misc.hpp"

int main(int argc, char** argv) {
  ios::sync_with_stdio(false);

  cmdline::parser p;
  p.add<string>("cws_fn", 'i', "input file name of cws sketches (in bvecs format)", true);
  p.add<size_t>("num", 'n', "number of sketches printed", false, 5);
  p.parse_check(argc, argv);

  auto cws_fn = p.get<string>("cws_fn");
  auto num = p.get<size_t>("num");

  if (get_ext(cws_fn) != "bvecs") {
    cerr << "Error: invalid format file" << endl;
    return 1;
  }

  ifstream ifs(cws_fn);
  if (!ifs) {
    cerr << "Open error: " << cws_fn << endl;
    return 1;
  }

  for (size_t i = 0; i < num; ++i) {
    auto dim = read_value<uint32_t>(ifs);
    for (size_t j = 0; j < dim; ++j) {
      auto v = read_value<uint8_t>(ifs);
      cout << static_cast<uint32_t>(v) << ' ';
    }
    cout << '\n';
  }

  return 0;
}