#!/bin/sh

set -e

make clean && make all -j

./csvparse -s testdata/voo_historical.csv -p

make clean && make sanitize -j

./csvparse -s testdata/voo_historical.csv -o outputfile.csv

echo "bad1"
./csvparse -s testdata/bad1.csv -s || true
EXIT_CODE=$?
if [ $EXIT_CODE != 1 ];
then
    exit $EXIT_CODE
fi

echo "bad2"
./csvparse -s testdata/bad2.csv || true
EXIT_CODE=$?
if [ $EXIT_CODE != 1 ]; then
    exit $EXIT_CODE
fi

./csvparse -s testdata/bad3.csv -s

make clean && make valgrind -j
