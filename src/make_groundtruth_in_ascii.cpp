#include "cmdline.h"
#include "misc.hpp"

using namespace ascii_format;

template <int Flags>
int run(const cmdline::parser& p) {
  auto base_fn = p.get<string>("base_fn");
  auto query_fn = p.get<string>("query_fn");
  auto groundtruth_fn = p.get<string>("groundtruth_fn");
  auto begin_id = p.get<uint32_t>("begin_id");
  auto topk = p.get<uint32_t>("topk");

  const auto base_vecs = load_vecs<Flags>(base_fn, begin_id);
  size_t N = base_vecs.size();

  const auto query_vecs = load_vecs<Flags>(query_fn, begin_id);
  size_t M = query_vecs.size();

  struct id_sim_t {
    uint32_t id;
    float sim;
  };
  vector<id_sim_t> id_sims(N);

  groundtruth_fn += ".txt";
  ofstream ofs(groundtruth_fn);
  if (!ofs) {
    cerr << "open error: " << groundtruth_fn << endl;
    return 1;
  }
  ofs << M << '\n' << topk << '\n';

  for (size_t j = 0; j < M; ++j) {
    const auto& query = query_vecs[j];
    for (size_t i = 0; i < N; ++i) {
      const auto& base = base_vecs[i];
      id_sims[i].id = uint32_t(i);
      id_sims[i].sim = calc_minmax_sim<Flags>(base, query);
    }

    sort(id_sims.begin(), id_sims.end(), [](const id_sim_t& a, const id_sim_t& b) {
      if (a.sim != b.sim) {
        return a.sim > b.sim;
      }
      return a.id < b.id;
    });

    for (uint32_t i = 0; i < topk; ++i) {
      ofs << id_sims[i].id << ':' << id_sims[i].sim << ',';
    }
    ofs << '\n';
  }

  cout << "Output " << groundtruth_fn << endl;

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
  p.add<string>("base_fn", 'i', "input file name of database vectors (in ASCII format)", true);
  p.add<string>("query_fn", 'q', "input file name of query vectors (in ASCII format)", true);
  p.add<string>("groundtruth_fn", 'o', "output file name of the groundtruth", true);
  p.add<uint32_t>("begin_id", 'b', "beginning ID of data column", false, 0);
  p.add<bool>("weighted", 'w', "Does the input data have weight?", false, false);
  p.add<bool>("generalized", 'g', "Does the input data need to be generalized?", false, false);
  p.add<bool>("labeled", 'l', "Does each input vector have a label at the head?", false, false);
  p.add<uint32_t>("topk", 'k', "k-nearest neighbors", false, 100);
  p.parse_check(argc, argv);

  auto weighted = p.get<bool>("weighted");
  auto generalized = p.get<bool>("generalized");
  auto labeled = p.get<bool>("labeled");

  auto flags = make_flags(weighted, generalized, labeled);
  return run_with_flags(flags, p);
}