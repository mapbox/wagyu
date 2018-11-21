#include <benchmark/benchmark.h>
#include "fixtures.hpp"

int main(int argc, char* argv[])
{
    register_fixtures<std::int64_t>();
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();

    return 0;
}
