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

for filename in $(ls ./tests/geometry-test-data/input-polyjson)
do
    for type in union difference x_or intersection
    do
        for filltype in even_odd non_zero positive negative
        do
            #echo $TESTER -t $type -f $filltype -i 10 ./tests/geometry-test-data/input-polyjson/$filename ./tests/fixtures/clip-clockwise-square.json 
            $TESTER -t $type -f $filltype -i 10 \
                ./tests/geometry-test-data/input-polyjson/$filename \
                ./tests/fixtures/clip-clockwise-square.json;

            # Check exit code of last command
            if [ "$?" -eq "0" ]; then
                PASSES=$((PASSES + 1))
            else
                FAILS=$((FAILS + 1))
            fi
        done
    done
done

TOTAL=$((PASSES + FAILS))
echo -e "Wagyu Faster On: \033[1;32m ✓ $PASSES/$TOTAL \033[0;31m ✗ $FAILS/$TOTAL \033[0m"

