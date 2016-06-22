for filename in $(ls ./tests/geometry-test-data/input-polyjson):
do
    echo $filename
    ./fixture-tester ./tests/fixtures/clip-clockwise-square.json ./tests/geometry-test-data/input-polyjson/$filename > ./tests/output-polyjson/$filename
done
