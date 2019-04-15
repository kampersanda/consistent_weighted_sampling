#include <fstream>
#include <random>

#include "cmdline.h"
#include "misc.hpp"

template <typename Distribution>
void gen_random_matrix(Distribution&& dist, ofstream& ofs, size_t size) {
  random_device seed_gen;
  mt19937_64 engine(seed_gen());
  for (size_t i = 0; i < size; ++i) {
    write_value(static_cast<float>(dist(engine)), ofs);
  }
}

int main(int argc, char** argv) {
  std::ios::sync_with_stdio(false);

  cmdline::parser p;
  p.add<string>("rand_fn", 'r', "output file name of random matrix data", true);
  p.add<size_t>("dat_dim", 'd', "dimension of the input data", true);
  p.add<size_t>("cws_dim", 'D', "dimension of the cws sketches", false, 64);
  p.add<bool>("generalized", 'g', "generalized?", false, false);
  p.parse_check(argc, argv);

  auto rand_fn = p.get<string>("rand_fn");
  auto dat_dim = p.get<size_t>("dat_dim");
  auto cws_dim = p.get<size_t>("cws_dim");
  auto generalized = p.get<bool>("generalized");

  {
    ostringstream oss;
    oss << rand_fn << '.' << dat_dim << 'x' << cws_dim;
    if (generalized) {
      oss << "x2";
    }
    oss << ".rnd";
    rand_fn = oss.str();
  }

  if (generalized) {
    dat_dim = dat_dim * 2;
  }

  ofstream ofs(rand_fn);
  if (!ofs) {
    cerr << "Open error: " << rand_fn << endl;
    exit(1);
  }

  using gamma_t = gamma_distribution<float>;
  using uniform_t = uniform_real_distribution<float>;

  size_t size = dat_dim * cws_dim;

  cout << "Generating random data with gamma_distribution..." << endl;
  gen_random_matrix(gamma_t(2.0, 1.0), ofs, size);
  cout << "Generating random data with gamma_distribution..." << endl;
  gen_random_matrix(gamma_t(2.0, 1.0), ofs, size);
  cout << "Generating random data with uniform_real_distribution..." << endl;
  gen_random_matrix(uniform_t(2.0, 1.0), ofs, size);

  cout << "Output " << rand_fn << endl;

  return 0;
}
