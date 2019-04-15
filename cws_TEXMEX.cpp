#include <chrono>
#include <numeric>

#include "cmdline.h"
#include "misc.hpp"

template <typename T>
int run(const cmdline::parser& p) {
  auto base_fn = p.get<string>("base_fn");
  auto rand_fn = p.get<string>("rand_fn");
  auto cws_fn = p.get<string>("cws_fn");
  auto dat_dim = p.get<size_t>("dat_dim");
  auto cws_dim = p.get<size_t>("cws_dim");
  auto progress = p.get<size_t>("progress");
  auto as_text = p.get<bool>("as_text");

  cout << "Loading random matrix data..." << endl;
  rand_matrcies mats = load_rand_matrcies(rand_fn, dat_dim, cws_dim);
  {
    // Consume (4 * num_samples * data_dim) bytes for each matrix
    auto MiB = (sizeof(float) * mats.R.size() * 3) / (1024.0 * 1024.0);
    cout << "The data are consuming " << MiB << " MiB..." << endl;
  }

  ifstream ifs(base_fn);
  if (!ifs) {
    cerr << "open error: " << base_fn << endl;
    return 1;
  }

  {
    ostringstream oss;
    oss << cws_fn;
    if (!as_text) {
      oss << ".cws";
    } else {
      oss << ".txt";
    }
    cws_fn = oss.str();
  }

  ofstream ofs(cws_fn);
  if (!ofs) {
    cerr << "open error: " << cws_fn << endl;
    return 1;
  }

  if (!as_text) {
    write_value(static_cast<uint32_t>(cws_dim), ofs);
  } else {
    ofs << cws_dim << '\n';
  }

  vector<float> base_vec(dat_dim);

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

    uint32_t this_dim = 0;
    ifs.read(reinterpret_cast<char*>(&this_dim), sizeof(this_dim));
    if (ifs.eof()) {
      break;
    }
    if (this_dim != dat_dim) {
      cerr << "Error: this_dim != dat_dim" << endl;
      return 1;
    }
    for (size_t j = 0; j < dat_dim; ++j) {
      T c;
      ifs.read(reinterpret_cast<char*>(&c), sizeof(c));
      base_vec[j] = static_cast<float>(c);
    }
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

      if (dat_dim <= min_id) {
        cerr << "Error: min_id exceeds data_dim" << endl;
        return 1;
      }

      if (!as_text) {
        write_value(static_cast<uint8_t>(min_id & UINT8_MAX), ofs);
      } else {
        ofs << static_cast<uint32_t>(min_id & UINT8_MAX) << ' ';
      }
    }

    if (as_text) {
      ofs << '\n';
    }
  }

  auto cur_tp = chrono::system_clock::now();
  auto dur_cnt = chrono::duration_cast<chrono::seconds>(cur_tp - start_tp).count();
  cout << "Done!! --> " << num_vecs + counter << " vecs processed in ";
  cout << dur_cnt / 3600 << "h" << dur_cnt / 60 % 60 << "m" << dur_cnt % 60 << "s!!" << endl;

  cout << "Output " << cws_fn << endl;

  return 0;
}

int main(int argc, char** argv) {
  std::ios::sync_with_stdio(false);

  cmdline::parser p;
  p.add<string>("base_fn", 'b', "input file name of base vectors on TEXMEX format (fvecs/bvecs)", true);
  p.add<string>("rand_fn", 'r', "input file name of random matrix data", true);
  p.add<string>("cws_fn", 'o', "output file name of cws sketches", true);
  p.add<size_t>("dat_dim", 'd', "dimension of the input data", true);
  p.add<size_t>("cws_dim", 'D', "dimension of the cws sketches", false, 64);
  p.add<size_t>("progress", 'p', "step of progress for print", false, numeric_limits<size_t>::max());
  p.add<bool>("as_text", 't', "as text?", false);
  p.parse_check(argc, argv);

  auto base_fn = p.get<string>("base_fn");
  auto ext = get_ext(base_fn);

  if (ext == "fvecs") {
    return run<float>(p);
  }
  if (ext == "bvecs") {
    return run<uint8_t>(p);
  }

  cerr << "Error: invalid extension" << endl;
  return 1;
}