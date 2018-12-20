#include "cmdline.h"
#include "common.hpp"

namespace {

using namespace cws;

auto load_random_data(std::ifstream& ifs, uint64_t samples, uint64_t data_dim) {
  std::vector<float> mats[3];
  uint64_t size = samples * data_dim;

  for (auto& mat : mats) {
    mat.resize(size);
    for (uint64_t i = 0; i < size; ++i) {
      mat[i] = read_value<float>(ifs);
    }
    if (ifs.eof()) {
      std::cerr << "error: ifs for random data reaches EOF\n";
      exit(1);
    }
  }

  return std::make_tuple(std::move(mats[0]), std::move(mats[1]), std::move(mats[2]));
}

void run_cws(const std::string& data_fn, const std::string& random_fn, uint32_t data_dim,
             uint32_t num_samples, uint32_t alph_bits, uint32_t progress) {
  std::cout << "Loading random data..." << std::endl;

  std::vector<float> mat_r, mat_c, mat_b;
  {
    std::ifstream ifs(random_fn);
    if (!ifs) {
      std::cerr << "open error: " << random_fn << "\n";
      exit(1);
    }
    std::tie(mat_r, mat_c, mat_b) = load_random_data(ifs, num_samples, data_dim);
  }

  {
    // Consume (4 * num_samples * data_dim) bytes for each matrix
    auto MiB = (sizeof(float) * mat_r.size() * 3) / (1024.0 * 1024.0);
    std::cout << "The data are consuming " << MiB << " MiB..." << std::endl;
  }

  std::cout << "Sampling..." << std::endl;

  std::ostringstream outfn_oss;
  outfn_oss << data_fn << "." << num_samples << "." << alph_bits << ".cws";
  std::ofstream ofs(outfn_oss.str());
  if (!ofs) {
    std::cerr << "open error: " << outfn_oss.str() << '\n';
    exit(1);
  }

  std::ifstream ifs(data_fn);
  if (!ifs) {
    std::cerr << "open error: " << data_fn << '\n';
    return;
  }

  uint32_t alph_mask = (1 << alph_bits) - 1;
  std::vector<float> data_vec(data_dim);

  uint32_t counter = 0;
  uint32_t num_sketches = 0;

  for (;; ++counter) {
    if (counter == progress) {
      num_sketches += counter;
      counter = 0;
      std::cout << num_sketches << " sketches generated now..." << std::endl;
    }

    uint32_t dim = 0;
    ifs.read(reinterpret_cast<char*>(&dim), sizeof(dim));
    if (ifs.eof()) {
      break;
    }
    if (dim != data_dim) {
      std::cerr << "error: dim != data_dim\n";
      return;
    }
    for (uint32_t j = 0; j < data_dim; ++j) {
      uint8_t c;
      ifs.read(reinterpret_cast<char*>(&c), sizeof(c));
      data_vec[j] = static_cast<float>(c);
    }
    for (uint64_t i = 0; i < num_samples; ++i) {
      const auto vec_r = &mat_r[i * data_dim];
      const auto vec_c = &mat_c[i * data_dim];
      const auto vec_b = &mat_b[i * data_dim];

      float min_a = std::numeric_limits<float>::max();
      uint32_t min_id = 0;

      for (uint32_t j = 0; j < data_dim; ++j) {
        float t = std::floor(std::log10(data_vec[j]) / vec_r[j] + vec_b[j]);
        float a = std::log10(vec_c[j]) - (vec_r[j] * (t + 1.0 - vec_b[j]));

        if (a < min_a) {
          min_a = a;
          min_id = j;
        }
      }

      if (data_dim <= min_id) {
        std::cerr << "error: min_id exceeds data_dim\n";
        exit(1);
      }

      ofs << (min_id & alph_mask);
      if (i + 1 < num_samples) {
        ofs << ' ';
      }
    }

    ofs << '\n';
  }

  num_sketches += counter;
  std::cout << num_sketches << " sketches generated!!" << std::endl;
}

}  // namespace

int main(int argc, char** argv) {
  cmdline::parser p;
  p.add<std::string>("data_fn", 'd', "input file name of data vectors", true);
  p.add<std::string>("random_fn", 'r', "input file name of random matricies", true);
  p.add<uint32_t>("dim", 'm', "dimension of the input data", true);
  p.add<uint32_t>("samples", 's', "#samples in CWS", false, 64);
  p.add<uint32_t>("bits", 'b', "#bits for each sample", false, 4);
  p.add<uint32_t>("progress", 'p', "step of progress for print", false, UINT32_MAX);
  p.parse_check(argc, argv);

  auto data_fn = p.get<std::string>("data_fn");
  auto random_fn = p.get<std::string>("random_fn");
  auto dim = p.get<uint32_t>("dim");
  auto samples = p.get<uint32_t>("samples");
  auto bits = p.get<uint32_t>("bits");
  auto progress = p.get<uint32_t>("progress");

  if (data_fn.substr(data_fn.find_last_of(".") + 1) != "bvecs") {
    std::cerr << "error: not bvecs file\n";
    return 1;
  }

  run_cws(data_fn, random_fn, dim, samples, bits, progress);
  return 0;
}
