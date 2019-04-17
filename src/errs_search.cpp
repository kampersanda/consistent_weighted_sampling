#include "cmdline.h"
#include "misc.hpp"

using namespace texmex_format;

uint32_t get_hamdist(const uint8_t* v1, const uint8_t* v2, uint32_t dim, uint32_t max_errs) {
  uint32_t errs = 0;
  for (uint32_t i = 0; i < dim; ++i) {
    if (v1[i] != v2[i]) {
      ++errs;
      if (max_errs < errs) {
        break;
      }
    }
  }
  return errs;
}

int main(int argc, char** argv) {
  ios::sync_with_stdio(false);

  cmdline::parser p;
  p.add<string>("base_fn", 'i', "input file name of database of CWS-sketches (in bvecs format)", true);
  p.add<string>("query_fn", 'q', "input file name of queries of CWS-sketches (in bvecs format)", true);
  p.add<string>("score_fn", 'o', "output file name of ranked score data", true);
  p.add<uint32_t>("bits", 'b', "number of bits evaluated (<= 8)", false, 8);
  p.add<uint32_t>("dim", 'd', "dimension of CWS-sketches evaluated", false, 64);
  p.add<uint32_t>("max_errs", 'e', "e-neighbor search", false, 10);
  p.parse_check(argc, argv);

  auto base_fn = p.get<string>("base_fn");
  auto query_fn = p.get<string>("query_fn");
  auto score_fn = p.get<string>("score_fn");
  auto bits = p.get<uint32_t>("bits");
  auto dim = p.get<uint32_t>("dim");
  auto max_errs = p.get<uint32_t>("max_errs");

  if (bits == 0 or bits > 8) {
    cerr << "error: invalid bits" << endl;
    return 1;
  }

  vector<uint8_t> base_codes = load_vecs<uint8_t, uint8_t>(base_fn, dim);
  size_t N = base_codes.size() / dim;

  vector<uint8_t> query_codes = load_vecs<uint8_t, uint8_t>(query_fn, dim);
  size_t M = query_codes.size() / dim;

  if (bits < 8) {
    uint8_t mask = uint8_t((1 << bits) - 1);
    for_each(base_codes.begin(), base_codes.end(), [mask](uint8_t& v) { v &= mask; });
    for_each(query_codes.begin(), query_codes.end(), [mask](uint8_t& v) { v &= mask; });
  }

  struct id_errs_t {
    uint32_t id;
    uint32_t errs;
  };
  vector<id_errs_t> ranked_scores;

  {
    ostringstream oss;
    oss << score_fn << ".errs." << bits << "x" << dim << ".txt";
    score_fn = oss.str();
  }

  ofstream ofs(score_fn);
  if (!ofs) {
    cerr << "open error: " << score_fn << endl;
    return 1;
  }
  ofs << M << '\n' << max_errs << '\n';

  for (size_t j = 0; j < M; ++j) {
    const uint8_t* query = &query_codes[j * dim];
    for (size_t i = 0; i < N; ++i) {
      const uint8_t* base = &base_codes[i * dim];
      uint32_t errs = get_hamdist(base, query, dim, max_errs);
      if (errs <= max_errs) {
        ranked_scores.push_back(id_errs_t{uint32_t(i), errs});
      }
    }

    std::sort(ranked_scores.begin(), ranked_scores.end(), [](const id_errs_t& a, const id_errs_t& b) {
      if (a.errs != b.errs) {
        return a.errs < b.errs;
      }
      return a.id < b.id;
    });

    for (size_t i = 0; i < ranked_scores.size(); ++i) {
      ofs << ranked_scores[i].id << ':' << ranked_scores[i].errs << ',';
    }
    ofs << '\n';
  }

  cout << "Output " << score_fn << endl;

  return 0;
}