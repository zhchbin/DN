#!/bin/sh

git clone https://github.com/svn2github/gyp
./gyp/gyp --depth=. -I build/common.gypi
make
