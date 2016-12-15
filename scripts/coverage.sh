#!/usr/bin/env bash

set -eu
set -o pipefail

./.mason/mason install clang++ 4.0.0
./.mason/mason install llvm-cov 4.0.0

export CXX="$(./.mason/mason prefix clang++ 4.0.0)/bin/clang++"
export PROF=$(./.mason/mason prefix llvm-cov 4.0.0)/bin/llvm-profdata
export COV=$(./.mason/mason prefix llvm-cov 4.0.0)/bin/llvm-cov
export CXXFLAGS="-fprofile-instr-generate -fcoverage-mapping"
export LDFLAGS="-fprofile-instr-generate"
export LLVM_PROFILE_FILE="code-%p.profraw"

function run_cov() {
    local target=$1
    local binary=$2
    local cmd=$3
    local name=$4

    echo "** cleaning"
    make clean
    make ${target}
    rm -f code-*.profraw
    rm -f *profdata
    echo "** running '${name}'"
    ${cmd}
    echo "** merging '${name}' results"
    ${PROF} merge -output="${name}.profdata" code-*.profraw
    # note: demangling is not working - https://llvm.org/bugs/show_bug.cgi?id=31394
    ${COV} report ${binary} -instr-profile="${name}.profdata" -use-color
    ${COV} show ${binary} -instr-profile="${name}.profdata" include/mapbox/geometry/wagyu/*hpp -use-color --format html > /tmp/${name}.html
    rm -f code-*.profraw
    open /tmp/${name}.html
    echo "** open /tmp/${name}.html"
}

# unit test report
run_cov build-debug ./test ./test "wagyu-unit-test-coverage"
run_cov build-fixture-tester ./fixture-tester "./tests/run-geometry-tests.sh ./fixture-tester" "wagyu-fixture-test-coverage"
