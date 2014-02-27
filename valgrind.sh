#!/bin/bash
LD_LIBRARY_PATH=out/Default/lib:third_party/figtree-0.9.3/lib/:$LD_LIBRARY_PATH valgrind $@
