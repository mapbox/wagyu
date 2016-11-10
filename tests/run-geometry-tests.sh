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
        for filltype in even_odd non_zero positive negative
        do
            if [ "$type" = "union" ]; then
                $TESTER -t $type -f $filltype \
                    ./tests/geometry-test-data/input-polyjson/$filename \
                    1>./tests/output-polyjson/$type-$filename;

                # Check exit code of last command
                if [ "$?" -eq "0" ]; then
                    PASSES=$((PASSES + 1))
                else
                    echo --- Test failure: $type $filltype $filename
                    echo $TESTER -t $type -f $filltype ./tests/geometry-test-data/input-polyjson/$filename
                    FAILS=$((FAILS + 1))
                fi
            fi
            $TESTER -t $type -f $filltype \
                ./tests/geometry-test-data/input-polyjson/$filename \
                ./tests/fixtures/clip-clockwise-square.json \
                1>./tests/output-polyjson/$type-$filename;

            # Check exit code of last command
            if [ "$?" -eq "0" ]; then
                PASSES=$((PASSES + 1))
            else
                echo --- Test failure: $type $filltype $filename
                echo $TESTER -t $type -f $filltype ./tests/geometry-test-data/input-polyjson/$filename ./tests/fixtures/clip-clockwise-square.json 
                FAILS=$((FAILS + 1))
            fi
        done
    done
done

TOTAL=$((PASSES + FAILS))
echo -e "\033[1;32m ✓ $PASSES/$TOTAL \033[0;31m ✗ $FAILS/$TOTAL \033[0m"
if [ "$FAILS" -gt "0" ]; then 
    exit 1;
fi

