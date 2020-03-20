#!/bin/bash

set -e

cd /work

make clean && make -j WARN_FLAGS="-Wall -Wextra"

mkdir -p /work/dist/csvparse

cp *.a /work/dist/csvparse
cp *.so /work/dist/csvparse
cp csvparse /work/dist/csvparse
cp LICENSE.txt /work/dist/csvparse
cp csvparse.h /work/dist/csvparse

cd /work/dist

zip -r csvparse.zip csvparse

mv csvparse.zip /dist/
