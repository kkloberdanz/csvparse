#!/bin/sh

make clean && make all -j || exit 1

./csvparse -s testdata/voo_historical.csv -p || exit 1

make clean && make sanitize -j || exit 1

./csvparse -s testdata/voo_historical.csv -o outputfile.csv || exit 1

echo "bad1"
./csvparse -s testdata/bad1.csv -s
EXIT_CODE=$?
if [ $EXIT_CODE != 1 ];
then
    exit 1
fi

echo "bad2"
./csvparse -s testdata/bad2.csv
EXIT_CODE=$?
if [ $EXIT_CODE != 1 ]; then
    exit 1
fi

./csvparse -s testdata/bad3.csv || exit 1

make clean && make valgrind -j || exit 1
