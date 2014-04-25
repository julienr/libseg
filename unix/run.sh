#!/bin/bash
LIB_PATH=out/Default/lib:$FIGTREE/unix/:third_party/_install/lib:$LD_LIBRARY_PATH
DYLD_LIBRARY_PATH=$LIB_PATH LD_LIBRARY_PATH=$LIB_PATH $@
