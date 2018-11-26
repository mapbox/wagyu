#include <benchmark/benchmark.h>
#include "fixtures.hpp"

int main(int argc, char* argv[])
{
    register_fixtures();
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();

    return 0;
}
