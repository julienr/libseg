#!/bin/bash
# Download and build the required third-party libraries

# -- figtree
wget http://sourceforge.net/projects/figtree/files/figtree/0.9.3/figtree-0.9.3.zip/download -O figtree-0.9.3.zip
unzip -o -u figtree-0.9.3.zip
pushd figtree-0.9.3
make
popd

# -- googlemock
wget https://googlemock.googlecode.com/files/gmock-1.7.0.zip -O gmock-1.7.0.zip
unzip -o -u gmock-1.7.0.zip
pushd gmock-1.7.0
make -j5
popd
