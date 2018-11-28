#!/bin/bash
# cloredis test

ulimit -c unlimited

cd `dirname $0`
dir=$(pwd)
export LD_LIBRARY_PATH=${dir}/../lib

./cloredis_test
