#include "cmdline.h"
#include "misc.hpp"

using namespace texmex_format;

template <class InType>
int run(const cmdline::parser& p) {
    auto base_fn = p.get<string>("base_fn");
    auto query_fn = p.get<string>("query_fn");
    auto groundtruth_fn = p.get<string>("groundtruth_fn");
    auto dim = p.get<uint32_t>("dim");
    auto topk = p.get<uint32_t>("topk");
    auto progress = p.get<size_t>("progress");

    vector<float> base_vecs = load_vecs<InType, float>(base_fn, dim);
    size_t N = base_vecs.size() / dim;

    vector<float> query_vecs = load_vecs<InType, float>(query_fn, dim);
    size_t M = query_vecs.size() / dim;

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

    auto start_tp = chrono::system_clock::now();
    size_t progress_point = progress;

    for (size_t j = 0; j < M; ++j) {
        if (j == progress_point) {
            progress_point += progress;
            auto cur_tp = chrono::system_clock::now();
            auto dur_cnt = chrono::duration_cast<chrono::seconds>(cur_tp - start_tp).count();
            cout << j << " queries processed in ";
            cout << dur_cnt / 3600 << "h" << dur_cnt / 60 % 60 << "m" << dur_cnt % 60 << "s..." << endl;
        }

        const float* query = &query_vecs[j * dim];
        for (size_t i = 0; i < N; ++i) {
            const float* base = &base_vecs[i * dim];
            id_sims[i].id = uint32_t(i);
            id_sims[i].sim = calc_minmax_sim(base, query, dim);
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

    auto cur_tp = chrono::system_clock::now();
    auto dur_cnt = chrono::duration_cast<chrono::seconds>(cur_tp - start_tp).count();
    cout << "Completed!! --> " << M << " queries processed in ";
    cout << dur_cnt / 3600 << "h" << dur_cnt / 60 % 60 << "m" << dur_cnt % 60 << "s!!" << endl;

    cout << "Output " << groundtruth_fn << endl;

    return 0;
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);

    cmdline::parser p;
    p.add<string>("base_fn", 'i', "input file name of database vectors (in fvecs/bvecs format)", true);
    p.add<string>("query_fn", 'q', "input file name of query vectors (in fvecs/bvecs format)", true);
    p.add<string>("groundtruth_fn", 'o', "output file name of the groundtruth", true);
    p.add<uint32_t>("dim", 'd', "dimension of the input data", true);
    p.add<uint32_t>("topk", 'k', "k-nearest neighbors", false, 100);
    p.add<size_t>("progress", 'p', "step of printing progress", false, numeric_limits<size_t>::max());
    p.parse_check(argc, argv);

    auto base_fn = p.get<string>("base_fn");
    auto query_fn = p.get<string>("query_fn");

    auto base_ext = get_ext(base_fn);
    auto query_ext = get_ext(query_fn);

    if (base_ext != query_ext) {
        cerr << "error: base_ext != query_ext" << endl;
        return 1;
    }

    if (base_ext == "fvecs") {
        return run<float>(p);
    }
    if (base_ext == "bvecs") {
        return run<uint8_t>(p);
    }

    cerr << "error: invalid extension" << endl;
    return 1;
}