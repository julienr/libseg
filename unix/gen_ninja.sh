#!/bin/bash
#COMP=g++
#COMP="clang++"
COMP="ccache g++"
export CXX=$COMP
export CC=$COMP

gyp project.gyp --depth=. -f ninja -G output_dir=$PWD/out -D ROOTDIR=${PWD} \
  -D SRCDIR=../src -D INCDIR=../include -D FIGTREE=$FIGTREE
