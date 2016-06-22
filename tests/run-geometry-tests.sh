#!/usr/bin/env bash

# ./tests/run-geometry-tests.sh ./fixture-tester

for filename in $(ls ./tests/geometry-test-data/input-polyjson)
do
    for type in union difference x_or intersection
    do
        echo $type $filename
        $1 -t $type ./tests/fixtures/clip-clockwise-square.json ./tests/geometry-test-data/input-polyjson/$filename > ./tests/output-polyjson/$type-$filename
    done
done
