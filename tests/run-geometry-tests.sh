#!/usr/bin/env bash

# ./tests/run-geometry-tests.sh ./fixture-tester
TESTER="$1"
PASSES=0
FAILS=0

if [ -z "$TESTER" ]; then
    echo "Error: path to a fixture-tester binary must be supplied"
    exit 1
elif [ ! -e "$TESTER" ]; then
    echo "Error: path to fixture-tester binary is invalid"
    exit 1
fi


mkdir -p ./tests/output-polyjson

for filename in $(ls ./tests/geometry-test-data/input-polyjson)
do
    for type in union difference x_or intersection
    do
        $TESTER -t $type \
            ./tests/fixtures/clip-clockwise-square.json \
            ./tests/geometry-test-data/input-polyjson/$filename \
            1>./tests/output-polyjson/$type-$filename;

        # Check exit code of last command
        if [ "$?" -eq "0" ]; then
            PASSES=$((PASSES + 1))
        else
            echo --- Test failure: $type $filename
            echo $TESTER -t $type ./tests/fixtures/clip-clockwise-square.json ./tests/geometry-test-data/input-polyjson/$filename
            FAILS=$((FAILS + 1))
        fi
    done
done

TOTAL=$((PASSES + FAILS))
echo -e "\033[1;32m ✓ $PASSES/$TOTAL \033[0;31m ✗ $FAILS/$TOTAL \033[0m"
if [ "$FAILS" -gt "0" ]; then 
    exit 1;
fi

