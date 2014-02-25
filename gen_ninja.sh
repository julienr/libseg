#!/bin/bash
#COMP=g++
#COMP="clang++"
COMP="ccache g++"
export CXX=$COMP
export CC=$COMP

# reset some environment variables
#export PYTHONPATH=
export CPATH=
export LIBRARY_PATH=
#export LD_LIBRARY_PATH=$PWD/libs/_install/lib/
#export LD_RUN_PATH=$PWD/libs/_install/lib/
#export PKG_CONFIG_PATH=$PWD/libs/_install/lib/pkgconfig:$PWD/libs/_install/share/pkgconfig
# TODO: Remove dependency
export PKG_CONFIG_PATH=/home/julien/tm/v2/libs/_install/lib/pkgconfig

gyp project.gyp --depth=. -f ninja -D ROOTDIR=${PWD}
