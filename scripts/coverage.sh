#!/usr/bin/env bash

set -eu
set -o pipefail

# http://clang.llvm.org/docs/UsersManual.html#profiling-with-instrumentation
# https://www.bignerdranch.com/blog/weve-got-you-covered/

MASON_BASE=$(pwd)
.mason/mason install clang++ 3.9.0
.mason/mason link clang++ 3.9.0
.mason/mason install llvm-cov 3.9.0
.mason/mason link llvm-cov 3.9.0
export CXX="${MASON_BASE}/mason_packages/.link/bin/clang++"
export CXXFLAGS="-fprofile-instr-generate -fcoverage-mapping"
export LDFLAGS="-fprofile-instr-generate"
export LLVM_PROFILE_FILE="code-%p.profraw"
make debug
rm -f *profdata
echo "merging results"
${MASON_BASE}/mason_packages/.link/bin/llvm-profdata merge -output=code.profdata code-*.profraw
${MASON_BASE}/mason_packages/.link/bin/llvm-cov report ./fixture-tester -instr-profile=code.profdata -use-color
${MASON_BASE}/mason_packages/.link/bin/llvm-cov report ./test -instr-profile=code.profdata -use-color
${MASON_BASE}/mason_packages/.link/bin/llvm-cov show ./fixture-tester -instr-profile=code.profdata include/mapbox/geometry/wagyu/*hpp -filename-equivalence -use-color --format html > /tmp/fixture-tester-coverage.html
open /tmp/fixture-tester-coverage.html
${MASON_BASE}/mason_packages/.link/bin/llvm-cov show ./test -instr-profile=code.profdata include/mapbox/geometry/wagyu/*hpp -filename-equivalence -use-color --format html > /tmp/test-coverage.html
open /tmp/test-coverage.html
echo "open /tmp/fixture-tester-coverage.html and /tmp/test-coverate.html for HTML version of this report"
rm -f *profraw
rm -f *gcov
