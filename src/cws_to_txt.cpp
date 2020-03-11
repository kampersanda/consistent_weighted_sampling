#include <numeric>

#include "cmdline.h"
#include "misc.hpp"

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);

    cmdline::parser p;
    p.add<string>("input_fn", 'i', "input file name of cws sketches (in bvecs format)", true);
    p.add<size_t>("num", 'n', "number of sketches printed", false, 5);
    p.parse_check(argc, argv);

    auto input_fn = p.get<string>("input_fn");
    auto num = p.get<size_t>("num");

    if (get_ext(input_fn) != "bvecs") {
        cerr << "Error: invalid format file" << endl;
        return 1;
    }

    ifstream ifs = make_ifstream(input_fn);

    for (size_t i = 0; i < num; ++i) {
        auto dim = read_value<uint32_t>(ifs);
        for (size_t j = 0; j < dim; ++j) {
            auto v = read_value<uint8_t>(ifs);
            cout << static_cast<uint32_t>(v) << ' ';
        }
        cout << '\n';
    }

    return 0;
}