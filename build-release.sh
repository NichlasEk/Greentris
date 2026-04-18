#!/usr/bin/env sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$SCRIPT_DIR"

. ./env.sh

make clean
make -j1

mkdir -p release
cp -f tetris.sfc release/Greentris.sfc
if [ -f tetris.sym ]; then
    cp -f tetris.sym release/Greentris.sym
fi
