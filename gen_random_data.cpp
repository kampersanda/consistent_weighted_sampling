#include <random>

#include "cmdline.h"
#include "common.hpp"

namespace {

using namespace cws;

template <typename t_dist>
void gen_random_matrix(t_dist&& dist, std::ofstream& ofs, uint64_t size) {
  std::random_device seed_gen;
  std::mt19937_64 engine(seed_gen());
  for (uint64_t i = 0; i < size; ++i) {
    write_value(static_cast<float>(dist(engine)), ofs);
  }
}

}  // namespace

int main(int argc, char** argv) {
  cmdline::parser p;
  p.add<std::string>("random_fn", 'r', "output file name of random data", true);
  p.add<uint64_t>("dim", 'm', "dimension of the input data", true);
  p.add<uint64_t>("samples", 's', "#samples in CWS", false, 64);
  p.add<bool>("generalized", 'g', "generalized?", false, false);
  p.parse_check(argc, argv);

  auto random_fn = p.get<std::string>("random_fn");
  auto dim = p.get<uint64_t>("dim");
  auto samples = p.get<uint64_t>("samples");
  auto generalized = p.get<bool>("generalized");

  if (generalized) {
    dim = dim * 2;
  }

  std::ofstream ofs(random_fn);
  if (!ofs) {
    std::cerr << "open error: " << random_fn << '\n';
    exit(1);
  }

  using gamma_t = std::gamma_distribution<float>;
  using uniform_t = std::uniform_real_distribution<float>;

  uint64_t size = dim * samples;

  std::cout << "generating random data with gamma_distribution...\n";
  gen_random_matrix(gamma_t(2.0, 1.0), ofs, size);
  std::cout << "generating random data with gamma_distribution...\n";
  gen_random_matrix(gamma_t(2.0, 1.0), ofs, size);
  std::cout << "generating random data with uniform_real_distribution...\n";
  gen_random_matrix(uniform_t(2.0, 1.0), ofs, size);

  return 0;
}
