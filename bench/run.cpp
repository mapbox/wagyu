#include "fixtures.hpp"
#include <benchmark/benchmark.h>

int main(int argc, char* argv[]) {
    register_fixtures();
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();

    return 0;
}
