#!/bin/sh
cd $(dirname "$0")

export LD_LIBRARY_PATH=$(dirname "$0")/lib:$LD_LIBRARY_PATH
./reader >log.txt 2>&1
