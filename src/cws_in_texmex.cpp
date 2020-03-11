#include <chrono>
#include <numeric>

#include "cmdline.h"
#include "misc.hpp"
#include "splitmix.hpp"

using namespace texmex_format;

constexpr size_t BUFFER_VECS = 100'000;

template <typename InType, bool Generalized>
int run(const cmdline::parser& p) {
    auto input_fn = p.get<string>("input_fn");
    auto output_fn = p.get<string>("output_fn");
    auto dat_dim = p.get<size_t>("dat_dim");
    auto cws_dim = p.get<size_t>("cws_dim");
    auto seed = p.get<size_t>("seed");

    if constexpr (Generalized) {
        dat_dim *= 2;
    }

    cout << "1) Generate random matrix data..." << endl;

    vector<float> R(dat_dim * cws_dim);
    vector<float> C(dat_dim * cws_dim);
    vector<float> B(dat_dim * cws_dim);

    splitmix64 seeder(seed);
    const size_t seed_R = seeder.next();
    const size_t seed_C = seeder.next();
    const size_t seed_B = seeder.next();

    auto start_tp = chrono::system_clock::now();

#pragma omp parallel sections
    {
#pragma omp section
        generate_random_matrix(gamma_t(2.0, 1.0), R, seed_R);
#pragma omp section
        generate_random_matrix(gamma_t(2.0, 1.0), C, seed_C);
#pragma omp section
        generate_random_matrix(uniform_t(2.0, 1.0), B, seed_B);
    }

    auto dur_cnt = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - start_tp).count();
    cout << "Elapsed time: " << dur_cnt / 3600 << "h" << dur_cnt / 60 % 60 << "m" << dur_cnt % 60 << "s" << endl;

    {
        // Consume (4 * num_samples * data_dim) bytes for each matrix
        auto MiB = (sizeof(float) * R.size() * 3) / (1024.0 * 1024.0);
        cout << "The random matrix data consumes " << MiB << " MiB" << endl;
    }

    cout << "2) Do consistent weighted sampling..." << endl;

    data_loader<InType, float, Generalized> in(input_fn, dat_dim);
    ofstream out = make_ofstream(output_fn + ".bvecs");

    vector<float> in_buffer(BUFFER_VECS * dat_dim);
    vector<uint8_t> out_buffer(BUFFER_VECS * cws_dim);

    size_t processed = 0;
    start_tp = chrono::system_clock::now();

    while (true) {
        // Bulk Loading
        size_t num_vecs = 0;
        while (num_vecs < BUFFER_VECS) {
            const float* data_vec = in.next();
            if (data_vec == nullptr) {
                break;
            }
            std::copy(data_vec, data_vec + dat_dim, &in_buffer[num_vecs * dat_dim]);
            num_vecs += 1;
        }

        if (num_vecs == 0) {
            break;
        }

        // Sampling
#pragma omp parallel for
        for (size_t id = 0; id < num_vecs; ++id) {
            const float* data_vec = &in_buffer[id * dat_dim];
            uint8_t* cws_vec = &out_buffer[id * cws_dim];

            for (size_t i = 0; i < cws_dim; ++i) {
                const float* vec_R = &R[i * dat_dim];
                const float* vec_C = &C[i * dat_dim];
                const float* vec_B = &B[i * dat_dim];

                float min_a = numeric_limits<float>::max();
                size_t min_id = 0;

                for (size_t j = 0; j < dat_dim; ++j) {
                    float t = floor(log10(data_vec[j]) / vec_R[j] + vec_B[j]);
                    float a = log10(vec_C[j]) - (vec_R[j] * (t + 1.0 - vec_B[j]));

                    if (a < min_a) {
                        min_a = a;
                        min_id = j;
                    }
                }

                // Write the lowest 8 bits for samples
                cws_vec[i] = static_cast<uint8_t>(min_id & UINT8_MAX);
            }
        }

        // Write
        for (size_t id = 0; id < num_vecs; ++id) {
            write_value(out, static_cast<uint32_t>(cws_dim));
            write_vec(out, &out_buffer[id * cws_dim], cws_dim);
        }

        processed += num_vecs;
        dur_cnt = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - start_tp).count();

        cout << processed << " vecs processed in ";
        cout << dur_cnt / 3600 << "h" << dur_cnt / 60 % 60 << "m" << dur_cnt % 60 << "s" << endl;
    }

    dur_cnt = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - start_tp).count();
    cout << "Completed!! --> " << processed << " vecs processed in ";
    cout << dur_cnt / 3600 << "h" << dur_cnt / 60 % 60 << "m" << dur_cnt % 60 << "s!!" << endl;

    cout << "Output " << output_fn << ".bvecs" << endl;
    return 0;
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cout << "num threads: " << omp_get_max_threads() << endl;

    cmdline::parser p;
    p.add<string>("input_fn", 'i', "input file name of database vectors (in fvecs/bvecs format)", true);
    p.add<string>("output_fn", 'o', "output file name of CWS-sketches (in bvecs format)", true);
    p.add<string>("format", 'f', "format of the input file (fvecs/bvecs); if empty, use the extension", false, "");
    p.add<size_t>("dat_dim", 'd', "dimension of the input data", true);
    p.add<size_t>("cws_dim", 'D', "dimension of the output CWS-sketches", false, 64);
    p.add<bool>("generalized", 'g', "Does the input data need to be generalized?", false, false);
    p.add<size_t>("seed", 's', "seed for random matrix data", false, 114514);
    p.parse_check(argc, argv);

    auto input_fn = p.get<string>("input_fn");
    auto format = p.get<string>("format");
    auto generalized = p.get<bool>("generalized");

    if (format.empty()) {
        format = get_ext(input_fn);
    }

    if (format == "fvecs") {
        if (generalized) {
            return run<float, true>(p);
        } else {
            return run<float, false>(p);
        }
    } else if (format == "bvecs") {
        return run<uint8_t, false>(p);
    }

    cerr << "error: invalid extension" << endl;
    return 1;
}