#include <chrono>
#include <numeric>

#include "cmdline.h"
#include "misc.hpp"

constexpr int WEIGHTED_FLAG = 1;
constexpr int GENERALIZED_FLAG = 2;
constexpr int LABELED_FLAG = 4;
constexpr int FLAGS_MAX = WEIGHTED_FLAG | GENERALIZED_FLAG | LABELED_FLAG;

constexpr int make_flags(bool weighted, bool generalized, bool labeled) {
  int flags = 0;
  if (weighted) flags |= WEIGHTED_FLAG;
  if (generalized) flags |= GENERALIZED_FLAG;
  if (labeled) flags |= LABELED_FLAG;
  return flags;
}

template <int Flags>
constexpr bool is_weighted() {
  return (Flags & WEIGHTED_FLAG) != 0;
}
template <int Flags>
constexpr bool is_generalized() {
  return (Flags & GENERALIZED_FLAG) != 0;
}
template <int Flags>
constexpr bool is_labeled() {
  return (Flags & LABELED_FLAG) != 0;
}

template <bool Weighted>
class elem_t;

template <>
class elem_t<true> {
 public:
  elem_t() = default;
  elem_t(uint32_t id, float weight) : id_(id), weight_(weight) {}

  uint32_t id() const {
    return id_;
  }
  float weight() const {
    return weight_;
  }

 private:
  uint32_t id_;
  float weight_;
};

template <>
class elem_t<false> {
 public:
  elem_t() = default;
  elem_t(uint32_t id) : id_(id) {}

  uint32_t id() const {
    return id_;
  }
  float weight() const {
    return 1.0;
  }

 private:
  uint32_t id_;
};

template <int Flags>
class data_iterator {
 public:
  using elem_type = elem_t<is_weighted<Flags>()>;

 public:
  data_iterator() = default;

  data_iterator(const string& fn, uint32_t begin_id) : ifs_(fn), begin_id_(begin_id) {
    if (!ifs_) {
      cerr << "open error: " << fn << '\n';
      exit(1);
    }
  }

  bool next(vector<elem_type>& vec, bool needs_clear = true) {
    if (needs_clear) {
      vec.clear();
    }

    if (!getline(ifs_, line_)) {
      return false;
    }

    istringstream iss(line_);
    if constexpr (is_labeled<Flags>()) {
      iss.ignore(numeric_limits<streamsize>::max(), ' ');
    }

    if constexpr (is_weighted<Flags>()) {
      for (string taken; iss >> taken;) {
        auto pos = taken.find(':');
        if (pos == string::npos) {
          cerr << "format error\n";
          exit(1);
        }

        auto id = static_cast<uint32_t>(stoul(taken.substr(0, pos))) - begin_id_;
        auto weight = stof(taken.substr(pos + 1));

        if constexpr (is_generalized<Flags>()) {
          if (weight > 0) {
            id = id * 2;
          } else {
            id = id * 2 + 1;
            weight = -1 * weight;
          }
        } else {
          if (weight < 0) {
            cerr << "need GENERALIZED\n";
            exit(1);
          }
        }
        vec.push_back({id, weight});
      }
    } else {
      for (uint32_t id = 0; iss >> id;) {
        vec.push_back({id});
      }
    }

    return true;
  }

 private:
  ifstream ifs_;
  string line_;
  uint32_t begin_id_ = 0;
};

template <int Flags>
int run(const cmdline::parser& p) {
  using data_iterator_type = data_iterator<Flags>;
  using elem_type = typename data_iterator_type::elem_type;

  auto dat_fn = p.get<string>("dat_fn");
  auto rand_fn = p.get<string>("rand_fn");
  auto cws_fn = p.get<string>("cws_fn");
  auto dat_dim = p.get<size_t>("dat_dim");
  auto cws_dim = p.get<size_t>("cws_dim");
  auto progress = p.get<size_t>("progress");
  auto as_text = p.get<bool>("as_text");
  auto begin_id = p.get<uint32_t>("begin_id");

  cout << "Loading random matrix data..." << endl;
  rand_matrcies mats = load_rand_matrcies(rand_fn, dat_dim, cws_dim);
  {
    // Consume (4 * num_samples * data_dim) bytes for each matrix
    auto MiB = (sizeof(float) * mats.R.size() * 3) / (1024.0 * 1024.0);
    cout << "The data are consuming " << MiB << " MiB..." << endl;
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
    cerr << "Open error: " << cws_fn << endl;
    return 1;
  }

  if (!as_text) {
    write_value(static_cast<uint32_t>(cws_dim), ofs);
  } else {
    ofs << cws_dim << '\n';
  }

  data_iterator_type data_it(dat_fn, begin_id);
  vector<elem_type> data_vec;

  size_t counter = 0;
  size_t num_vecs = 0;
  auto start_tp = chrono::system_clock::now();

  for (; data_it.next(data_vec); ++counter) {
    if (counter == progress) {
      num_vecs += counter;
      counter = 0;
      auto cur_tp = chrono::system_clock::now();
      auto dur_cnt = chrono::duration_cast<chrono::seconds>(cur_tp - start_tp).count();
      cout << num_vecs << " vecs processed in ";
      cout << dur_cnt / 3600 << "h" << dur_cnt / 60 % 60 << "m" << dur_cnt % 60 << "s..." << endl;
    }

    for (size_t i = 0; i < cws_dim; ++i) {
      size_t offset = i * dat_dim;
      const float* vec_R = &mats.R[offset];
      const float* vec_C = &mats.C[offset];
      const float* vec_B = &mats.B[offset];

      float min_a = numeric_limits<float>::max();
      size_t min_id = 0;

      for (const auto& feat : data_vec) {
        uint32_t j = feat.id();
        float t = floor(log10(feat.weight()) / vec_R[j] + vec_B[j]);
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

template <int Flags = 0>
int run_with_flags(int flags, const cmdline::parser& p) {
  if constexpr (Flags > FLAGS_MAX) {
    cerr << "Error: invalid flags\n";
    return 1;
  } else {
    if (flags == Flags) {
      return run<Flags>(p);
    }
    return run_with_flags<Flags + 1>(flags, p);
  }
}

int main(int argc, char** argv) {
  ios::sync_with_stdio(false);

  cmdline::parser p;
  p.add<string>("dat_fn", 'i', "input file name of data vectors of BIGANN format", true);
  p.add<string>("rand_fn", 'r', "input file name of random matrix data", true);
  p.add<string>("cws_fn", 'o', "output file name of cws sketches", true);
  p.add<size_t>("dat_dim", 'd', "dimension of the input data", true);
  p.add<size_t>("cws_dim", 'D', "dimension of the cws sketches", false, 64);
  p.add<size_t>("progress", 'p', "step of progress for print", false, numeric_limits<size_t>::max());
  p.add<bool>("as_text", 't', "as text?", false);
  p.add<uint32_t>("begin_id", 'b', "beginning ID of data column", false, 0);
  p.add<bool>("weighted", 'w', "Does the input data have weight?", false, false);
  p.add<bool>("generalized", 'g', "Does the input data need to be generalized?", false, false);
  p.add<bool>("labeled", 'l', "Does each input vector have a label at the head?", false, false);
  p.parse_check(argc, argv);

  auto weighted = p.get<bool>("weighted");
  auto generalized = p.get<bool>("generalized");
  auto labeled = p.get<bool>("labeled");

  auto flags = make_flags(weighted, generalized, labeled);
  return run_with_flags(flags, p);
}