#pragma once

#include <omp.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

template <typename T>
inline void write_value(ostream& os, T val) {
    os.write(reinterpret_cast<const char*>(&val), sizeof(val));
}
template <typename T>
inline void write_vec(ostream& os, const T* vec, size_t size) {
    os.write(reinterpret_cast<const char*>(vec), size * sizeof(T));
}

template <typename T>
inline T read_value(istream& is) {
    T val;
    is.read(reinterpret_cast<char*>(&val), sizeof(val));
    return val;
}
template <typename T>
inline void read_vec(istream& is, T* vec, size_t size) {
    is.read(reinterpret_cast<char*>(vec), size * sizeof(T));
}

inline ifstream make_ifstream(const string& filepath) {
    ifstream ifs(filepath);
    if (!ifs) {
        cerr << "open error: " << filepath << endl;
        exit(1);
    }
    return ifs;
}
inline ofstream make_ofstream(const string& filepath) {
    ofstream ofs(filepath);
    if (!ofs) {
        cerr << "open error: " << filepath << endl;
        exit(1);
    }
    return ofs;
}

using gamma_t = gamma_distribution<float>;
using uniform_t = uniform_real_distribution<float>;

template <typename Dist>
void generate_random_matrix(Dist&& dist, vector<float>& out, size_t seed) {
    mt19937_64 engine(seed);
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<float>(dist(engine));
    }
}

inline string get_ext(const string& fn) {
    return fn.substr(fn.find_last_of(".") + 1);
}

namespace texmex_format {

template <class InType, class OutType = float, bool Generalized = false>
class data_loader {
  public:
    data_loader() = default;

    // if Generalized = trie, dim needs to be set twice
    data_loader(const string& fn, uint32_t dim) : ifs_(fn), vec_(dim) {
        if constexpr (Generalized) {
            static_assert(is_same_v<OutType, float>);
        }
        if (!ifs_) {
            cerr << "open error: " << fn << '\n';
            exit(1);
        }
    }

    const OutType* next() {
        auto dim = read_value<uint32_t>(ifs_);
        if (ifs_.eof()) {
            return nullptr;
        }

        if constexpr (Generalized) {
            if (vec_.size() < dim * 2) {
                vec_.resize(dim * 2);
            }
            for (uint32_t j = 0; j < dim; ++j) {
                auto v = static_cast<OutType>(read_value<InType>(ifs_));
                if (v >= 0.0) {
                    vec_[j * 2] = v;
                    vec_[j * 2 + 1] = 0.0;
                } else {
                    vec_[j * 2] = 0.0;
                    vec_[j * 2 + 1] = -v;
                }
            }
        } else {
            if (vec_.size() < dim) {
                vec_.resize(dim);
            }
            for (uint32_t j = 0; j < dim; ++j) {
                vec_[j] = static_cast<OutType>(read_value<InType>(ifs_));
            }
        }

        return vec_.data();
    }

  private:
    ifstream ifs_;
    vector<OutType> vec_;
};

template <class InType, class OutType = float, bool Generalized = false>
inline vector<OutType> load_vecs(const string& fn, uint32_t dim) {
    vector<OutType> vecs;
    data_loader<InType, OutType, Generalized> loader(fn, dim);

    while (true) {
        auto vec = loader.next();
        if (vec == nullptr) {
            break;
        }
        copy(vec, vec + dim, back_inserter(vecs));
    }
    return vecs;
}

inline float calc_minmax_sim(const float* x, const float* y, uint32_t dim) {
    float min_sum = 0.0;
    float max_sum = 0.0;
    for (uint32_t i = 0; i < dim; ++i) {
        if (x[i] < y[i]) {
            min_sum += x[i];
            max_sum += y[i];
        } else {
            min_sum += y[i];
            max_sum += x[i];
        }
    }
    return min_sum / max_sum;
}

}  // namespace texmex_format

/****
 *  For ASCII format like libsvm datasets
 */
namespace ascii_format {

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
using elem_type = elem_t<is_weighted<Flags>()>;

template <int Flags>
class data_loader {
  public:
    data_loader() = default;

    data_loader(const string& fn, uint32_t begin_id) : ifs_(fn), begin_id_(begin_id) {
        if (!ifs_) {
            cerr << "open error: " << fn << '\n';
            exit(1);
        }
    }

    bool next() {
        vec_.clear();

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
                vec_.push_back({id, weight});
            }
        } else {
            for (uint32_t id = 0; iss >> id;) {
                vec_.push_back({id});
            }
        }

        return true;
    }

    const vector<elem_type<Flags>>& get() const {
        return vec_;
    }

  private:
    ifstream ifs_;
    vector<elem_type<Flags>> vec_;
    string line_;
    uint32_t begin_id_ = 0;
};

template <int Flags>
inline vector<vector<elem_type<Flags>>> load_vecs(const string& fn, uint32_t begin_id) {
    vector<vector<elem_type<Flags>>> vecs;
    data_loader<Flags> loader(fn, begin_id);
    while (loader.next()) {
        auto& vec = loader.get();
        vecs.push_back(vec);  // copy
    }
    return vecs;
}

template <int Flags>
inline float calc_minmax_sim(const vector<elem_type<Flags>>& x, const vector<elem_type<Flags>>& y) {
    float min_sum = 0.0;
    float max_sum = 0.0;
    size_t i = 0, j = 0;
    while (i < x.size() and j < y.size()) {
        if (x[i].id() == y[j].id()) {
            if (x[i].weight() < y[j].weight()) {
                min_sum += x[i].weight();
                max_sum += y[j].weight();
            } else {
                min_sum += y[j].weight();
                max_sum += x[i].weight();
            }
            ++i;
            ++j;
        } else if (x[i].id() < y[j].id()) {
            max_sum += x[i].weight();
            ++i;
        } else {
            max_sum += y[j].weight();
            ++j;
        }
    }
    for (; i < x.size(); ++i) {
        max_sum += x[i].weight();
    }
    for (; j < y.size(); ++j) {
        max_sum += y[j].weight();
    }

    return min_sum / max_sum;
}

}  // namespace ascii_format
