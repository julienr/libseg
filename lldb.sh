#!/bin/bash
LIB_PATH=out/Default/lib:third_party/figtree-0.9.3/lib/:third_party/_install/lib:$LD_LIBRARY_PATH
DYLD_LIBRARY_PATH=$LIB_PATH LD_LIBRARY_PATH=$LIB_PATH lldb -o run $@
