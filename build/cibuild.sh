#!/bin/sh

git clone https://github.com/svn2github/gyp
./gyp/gyp --depth . p-ninja.gyp
make
