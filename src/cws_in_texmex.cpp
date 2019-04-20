#include <chrono>
#include <numeric>

#include "cmdline.h"
#include "misc.hpp"

using namespace texmex_format;

template <typename InType, bool Generalized>
int run(const cmdline::parser& p) {
  auto base_fn = p.get<string>("base_fn");
  auto rand_fn = p.get<string>("rand_fn");
  auto cws_fn = p.get<string>("cws_fn");
  auto dat_dim = p.get<size_t>("dat_dim");
  auto cws_dim = p.get<size_t>("cws_dim");
  auto progress = p.get<size_t>("progress");

  cout << "Loading random matrix data..." << endl;
  rand_matrcies mats = load_rand_matrcies(rand_fn, dat_dim, cws_dim);
  {
    // Consume (4 * num_samples * data_dim) bytes for each matrix
    auto MiB = (sizeof(float) * mats.R.size() * 3) / (1024.0 * 1024.0);
    cout << "The random matrix data consumes " << MiB << " MiB" << endl;
  }

  cws_fn += ".bvecs";
  ofstream ofs(cws_fn);
  if (!ofs) {
    cerr << "open error: " << cws_fn << endl;
    return 1;
  }

  if constexpr (Generalized) {
    dat_dim *= 2;
  }
  data_loader<InType, float, Generalized> base_loader(base_fn, dat_dim);

  size_t counter = 0;
  size_t num_vecs = 0;
  auto start_tp = chrono::system_clock::now();

  for (;; ++counter) {
    if (counter == progress) {
      num_vecs += counter;
      counter = 0;
      auto cur_tp = chrono::system_clock::now();
      auto dur_cnt = chrono::duration_cast<chrono::seconds>(cur_tp - start_tp).count();
      cout << num_vecs << " vecs processed in ";
      cout << dur_cnt / 3600 << "h" << dur_cnt / 60 % 60 << "m" << dur_cnt % 60 << "s..." << endl;
    }

    const float* base_vec = base_loader.next();
    if (base_vec == nullptr) {
      break;
    }

    write_value(ofs, static_cast<uint32_t>(cws_dim));

    for (size_t i = 0; i < cws_dim; ++i) {
      size_t offset = i * dat_dim;
      const float* vec_R = &mats.R[offset];
      const float* vec_C = &mats.C[offset];
      const float* vec_B = &mats.B[offset];

      float min_a = numeric_limits<float>::max();
      size_t min_id = 0;

      for (size_t j = 0; j < dat_dim; ++j) {
        float t = floor(log10(base_vec[j]) / vec_R[j] + vec_B[j]);
        float a = log10(vec_C[j]) - (vec_R[j] * (t + 1.0 - vec_B[j]));

        if (a < min_a) {
          min_a = a;
          min_id = j;
        }
      }

      // Write the lowest 8 bits for samples
      write_value(ofs, static_cast<uint8_t>(min_id & UINT8_MAX));
    }
  }

  auto cur_tp = chrono::system_clock::now();
  auto dur_cnt = chrono::duration_cast<chrono::seconds>(cur_tp - start_tp).count();
  cout << "Completed!! --> " << num_vecs + counter << " vecs processed in ";
  cout << dur_cnt / 3600 << "h" << dur_cnt / 60 % 60 << "m" << dur_cnt % 60 << "s!!" << endl;

  cout << "Output " << cws_fn << endl;

  return 0;
}

int main(int argc, char** argv) {
  ios::sync_with_stdio(false);

  cmdline::parser p;
  p.add<string>("base_fn", 'i', "input file name of database vectors (in fvecs/bvecs format)", true);
  p.add<string>("rand_fn", 'r', "input file name of random matrix data", true);
  p.add<string>("cws_fn", 'o', "output file name of CWS-sketches (in bvecs format)", true);
  p.add<size_t>("dat_dim", 'd', "dimension of the input data", true);
  p.add<size_t>("cws_dim", 'D', "dimension of the output CWS-sketches", false, 64);
  p.add<bool>("generalized", 'g', "Does the input data need to be generalized?", false, false);
  p.add<size_t>("progress", 'p', "step of printing progress", false, numeric_limits<size_t>::max());
  p.parse_check(argc, argv);

  auto base_fn = p.get<string>("base_fn");
  auto generalized = p.get<bool>("generalized");
  auto ext = get_ext(base_fn);

  if (ext == "fvecs") {
    if (generalized) {
      return run<float, true>(p);
    } else {
      return run<float, false>(p);
    }
  }
  if (ext == "bvecs") {
    return run<uint8_t, false>(p);
  }

  cerr << "error: invalid extension" << endl;
  return 1;
}