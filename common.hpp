#pragma once

#include <cxxabi.h>
#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace cws {

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
struct elem_t;

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

  data_iterator(const std::string& fn, uint32_t begin_id) : ifs_(fn), begin_id_(begin_id) {
    if (!ifs_) {
      std::cerr << "open error: " << fn << '\n';
      exit(1);
    }
  }

  bool next(std::vector<elem_type>& vec, bool needs_clear = true) {
    if (needs_clear) {
      vec.clear();
    }

    if (!std::getline(ifs_, line_)) {
      return false;
    }

    std::istringstream iss(line_);
    if constexpr (is_labeled<Flags>()) {
      iss.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
    }

    if constexpr (is_weighted<Flags>()) {
      for (std::string taken; iss >> taken;) {
        auto pos = taken.find(':');
        if (pos == std::string::npos) {
          std::cerr << "format error\n";
          exit(1);
        }

        auto id = static_cast<uint32_t>(std::stoul(taken.substr(0, pos))) - begin_id_;
        auto weight = std::stof(taken.substr(pos + 1));

        if constexpr (is_generalized<Flags>()) {
          if (weight > 0) {
            id = id * 2;
          } else {
            id = id * 2 + 1;
            weight = -1 * weight;
          }
        } else {
          if (weight < 0) {
            std::cerr << "need GENERALIZED\n";
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
  std::ifstream ifs_;
  std::string line_;
  uint32_t begin_id_ = 0;
};

template <int Flags>
inline auto load_data(const std::string& fn, uint32_t begin_id) {
  using data_iterator_type = data_iterator<Flags>;
  using elem_type = typename data_iterator_type::elem_type;

  std::vector<elem_type> data_vecs;
  std::vector<size_t> begins = {0};
  {
    data_iterator_type data_it(fn, begin_id);
    while (data_it.next(data_vecs, false)) {
      begins.push_back(data_vecs.size());
    }
  }

  return std::make_pair(std::move(data_vecs), std::move(begins));
}

template <class ElemContainer>
inline float compute_minmax(ElemContainer&& lhs, ElemContainer&& rhs) {
  return compute_minmax(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
}

template <typename T>
inline void write_value(const T val, std::ostream& os) {
  os.write(reinterpret_cast<const char*>(&val), sizeof(val));
}

template <typename T>
inline T read_value(std::istream& is) {
  T val;
  is.read(reinterpret_cast<char*>(&val), sizeof(val));
  return val;
}

// http://yuta1402.hatenablog.jp/entry/2017/12/15/233615
template <class Iterator, class Function, size_t MaxWorkers = 8>
inline Function parallel_for_each(Iterator begin, Iterator end, Function f) {
  if (begin == end) {
    return std::move(f);
  }

  std::size_t num_threads = std::thread::hardware_concurrency();
  if (num_threads == 0 or MaxWorkers < num_threads) {
    num_threads = MaxWorkers;
  }

  std::cout << "Use " << num_threads << " threads\n";
  std::size_t step = std::max<std::size_t>(1, std::distance(begin, end) / num_threads);

  std::vector<std::thread> threads;

  for (; begin < end - step; begin += step) {
    threads.emplace_back([=, &f]() { std::for_each(begin, begin + step, f); });
  }
  threads.emplace_back([=, &f]() { std::for_each(begin, end, f); });

  for (auto&& t : threads) {
    t.join();
  }

  return std::move(f);
}

template <class Container, class Function>
inline Function parallel_for_each(Container&& c, Function f) {
  return parallel_for_each(std::begin(c), std::end(c), f);
}

}  // namespace cws
