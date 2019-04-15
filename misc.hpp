#pragma once

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

template <typename T>
inline void write_value(const T val, ostream& os) {
  os.write(reinterpret_cast<const char*>(&val), sizeof(val));
}

template <typename T>
inline T read_value(istream& is) {
  T val;
  is.read(reinterpret_cast<char*>(&val), sizeof(val));
  return val;
}

struct rand_matrcies {
  vector<float> R;
  vector<float> C;
  vector<float> B;
};

inline vector<float> load_rand_matrix(istream& is, size_t size) {
  vector<float> ret(size);
  for (size_t i = 0; i < size; ++i) {
    ret[i] = read_value<float>(is);
  }
  if (is.eof()) {
    cerr << "error: ifs for random data reaches EOF" << endl;
    exit(1);
  }
  return ret;
}

inline rand_matrcies load_rand_matrcies(const string& fn, size_t dat_dim, size_t cws_dim) {
  ifstream ifs(fn);
  if (!ifs) {
    cerr << "open error: " << fn << endl;
    exit(1);
  }

  rand_matrcies ret;
  size_t size = dat_dim * cws_dim;
  ret.R = load_rand_matrix(ifs, size);
  ret.C = load_rand_matrix(ifs, size);
  ret.B = load_rand_matrix(ifs, size);
  return ret;
}

inline string get_ext(const string& fn) {
  return fn.substr(fn.find_last_of(".") + 1);
}