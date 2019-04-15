#include <algorithm>
#include <numeric>
#include <random>

#include "cmdline.h"
#include "misc.hpp"

using namespace std;

int main(int argc, char** argv) {
  ios::sync_with_stdio(false);

  cmdline::parser p;
  p.add<string>("input_fn", 'i', "input file name of cws sketches", true);
  p.add<string>("output_fn", 'o', "output_fn file name of randomly sampled cws sketches", true);
  p.add<size_t>("num", 'n', "num vectors", false, 1000);
  p.parse_check(argc, argv);

  auto input_fn = p.get<string>("input_fn");
  auto output_fn = p.get<string>("output_fn");
  auto num = p.get<size_t>("num");

  ifstream ifs(input_fn);
  if (!ifs) {
    cerr << "open error: " << input_fn << endl;
    return 1;
  }

  ofstream ofs(output_fn);
  if (!ofs) {
    cerr << "open error: " << output_fn << endl;
    return 1;
  }

  auto dim = read_value<uint32_t>(ifs);
  write_value(dim, ofs);

  vector<uint8_t> buf(dim);
  vector<uint8_t> sketches;

  size_t N = 0;
  while (true) {
    ifs.read(reinterpret_cast<char*>(buf.data()), dim);
    if (ifs.eof()) {
      break;
    }
    copy(buf.data(), buf.data() + dim, back_inserter(sketches));
    ++N;
  }

  random_device rnd;
  for (size_t i = 0; i < num; ++i) {
    size_t n = rnd() % N;
    ofs.write(reinterpret_cast<char*>(sketches.data() + n * dim), dim);
  }

  return 0;
}