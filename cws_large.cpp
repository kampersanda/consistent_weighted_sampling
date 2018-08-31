
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

template <int t_flags>
void run_cws(const std::string& data_list, const std::string& random_fn, uint32_t data_dim,
             uint32_t num_samples, uint32_t alph_bits, uint32_t begin_id) {
  using data_iterator_type = data_iterator<t_flags>;
  using elem_type = typename data_iterator_type::elem_type;

  if constexpr (is_generalized<t_flags>()) {
    data_dim = data_dim * 2;
  }

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
    auto GiB = (sizeof(float) * mat_r.size() * 3) / (1024.0 * 1024.0 * 1024.0);
    std::cout << "The data are consuming " << GiB << " GiB..." << std::endl;
  }

  std::cout << "Sampling for each data file..." << std::endl;

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

  const uint32_t alph_mask = (1 << alph_bits) - 1;

  auto sample = [&](const std::string& data_fn) {
    data_iterator_type data_it(data_fn, begin_id);

    auto output_fn = data_fn + ".cws";
    std::ofstream ofs(output_fn);
    if (!ofs) {
      std::cerr << "open error: " << output_fn << '\n';
      return;
    }

    ofs << num_samples << ' ' << alph_bits << '\n';  // header

    for (std::vector<elem_type> data_vec; data_it.next(data_vec);) {
      for (uint64_t i = 0; i < num_samples; ++i) {
        uint64_t j = i * data_dim;
        const auto vec_r = &mat_r[j];
        const auto vec_c = &mat_c[j];
        const auto vec_b = &mat_b[j];

        float min_a = std::numeric_limits<float>::max();
        uint32_t min_id = 0;

        for (const auto& feat : data_vec) {
          float t = std::floor(std::log10(feat.weight()) / vec_r[feat.id()] + vec_b[feat.id()]);
          float a = std::log10(vec_c[feat.id()]) - (vec_r[feat.id()] * (t + 1.0 - vec_b[feat.id()]));

          if (a < min_a) {
            min_a = a;
            min_id = feat.id();
          }
        }

        if (data_dim <= min_id) {
          std::cerr << "error: min_id exceeds data_dim\n";
          exit(1);
        }

        ofs << (min_id & alph_mask) << ' ';
      }

      ofs << '\n';
    }

    std::cout << output_fn << " is done...\n";
  };

  parallel_for_each(data_fns, sample);
}

template <int t_flags, typename... t_args>
void run_cws_with_flags(int flags, const t_args&... args) {
  if constexpr (t_flags > FLAGS_MAX) {
    std::cerr << "error: invalid flags\n";
    return;
  } else {
    if (flags == t_flags) {
      return run_cws<t_flags>(args...);
    } else {
      run_cws_with_flags<t_flags + 1>(flags, args...);
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  cmdline::parser p;
  p.add<std::string>("data_list", 'd', "list of input file names of data vectors", true);
  p.add<std::string>("random_fn", 'r', "input file name of random matricies", true);
  p.add<uint32_t>("dim", 'm', "dimension of the input data", true);
  p.add<uint32_t>("samples", 's', "#samples in CWS", false, 16);
  p.add<uint32_t>("bits", 'b', "#bits for each sample", false, 3);
  p.add<uint32_t>("begin_id", 'i', "beginning ID of data column", false, 0);
  p.add<bool>("weighted", 'w', "Does the input data have weight?", false, false);
  p.add<bool>("generalized", 'g', "Does the input data need to be generalized?", false, false);
  p.add<bool>("labeled", 'l', "Does each input vector have a label at the head?", false, false);
  p.parse_check(argc, argv);

  auto data_list = p.get<std::string>("data_list");
  auto random_fn = p.get<std::string>("random_fn");
  auto dim = p.get<uint32_t>("dim");
  auto samples = p.get<uint32_t>("samples");
  auto bits = p.get<uint32_t>("bits");
  auto begin_id = p.get<uint32_t>("begin_id");
  auto weighted = p.get<bool>("weighted");
  auto generalized = p.get<bool>("generalized");
  auto labeled = p.get<bool>("labeled");

  auto flags = make_flags(weighted, generalized, labeled);
  run_cws_with_flags<0>(flags, data_list, random_fn, dim, samples, bits, begin_id);

  return 0;
}
