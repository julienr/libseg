#!/bin/bash
#COMP=g++
#COMP="clang++"
COMP="ccache g++"
export CXX=$COMP
export CC=$COMP

export PKG_CONFIG_PATH=$PWD/third_party/_install/lib/pkgconfig

gyp project.gyp --depth=. -f ninja -D ROOTDIR=${PWD}
