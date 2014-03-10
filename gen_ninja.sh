#!/bin/bash
#COMP=g++
#COMP="clang++"
COMP="ccache g++"
export CXX=$COMP
export CC=$COMP

gyp project.gyp --depth=. -f ninja -D ROOTDIR=${PWD}
