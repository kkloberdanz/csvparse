#!/bin/bash

docker build . -t csvparse-builder
docker run -v $(pwd):/dist --rm csvparse-builder /work/docker/build-release.sh
