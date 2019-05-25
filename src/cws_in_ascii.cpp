#include <chrono>
#include <numeric>

#include "cmdline.h"
#include "misc.hpp"

using namespace ascii_format;

template <int Flags>
int run(const cmdline::parser& p) {
    using data_loader_type = data_loader<Flags>;

    auto base_fn = p.get<string>("base_fn");
    auto rand_fn = p.get<string>("rand_fn");
    auto cws_fn = p.get<string>("cws_fn");
    auto dat_dim = p.get<size_t>("dat_dim");
    auto cws_dim = p.get<size_t>("cws_dim");
    auto progress = p.get<size_t>("progress");
    auto begin_id = p.get<uint32_t>("begin_id");

    if (is_generalized<Flags>()) {
        dat_dim *= 2;
    }

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

    data_loader_type base_loader(base_fn, begin_id);

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

        if (!base_loader.next()) {
            break;
        }
        auto& data_vec = base_loader.get();

        write_value(ofs, static_cast<uint32_t>(cws_dim));

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
                cerr << "error: min_id exceeds dat_dim" << endl;
                return 1;
            }

            // Write the lowest 8 bits for samples
            write_value(ofs, static_cast<uint8_t>(min_id & UINT8_MAX));
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
    p.add<string>("base_fn", 'i', "input file name of database vectors (in ASCII format)", true);
    p.add<string>("rand_fn", 'r', "input file name of random matrix data", true);
    p.add<string>("cws_fn", 'o', "output file name of CWS-sketches (in bvecs format)", true);
    p.add<size_t>("dat_dim", 'd', "dimension of the input data", true);
    p.add<size_t>("cws_dim", 'D', "dimension of the output CWS-sketches", false, 64);
    p.add<uint32_t>("begin_id", 'b', "beginning ID of data column", false, 0);
    p.add<bool>("weighted", 'w', "Does the input data have weight?", false, false);
    p.add<bool>("generalized", 'g', "Does the input data need to be generalized?", false, false);
    p.add<bool>("labeled", 'l', "Does each input vector have a label at the head?", false, false);
    p.add<size_t>("progress", 'p', "step of printing progress", false, numeric_limits<size_t>::max());
    p.parse_check(argc, argv);

    auto weighted = p.get<bool>("weighted");
    auto generalized = p.get<bool>("generalized");
    auto labeled = p.get<bool>("labeled");

    auto flags = make_flags(weighted, generalized, labeled);
    return run_with_flags(flags, p);
}