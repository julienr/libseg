#!/bin/bash
CCACHE_SLOPPINESS=time_macros ninja -C out/Default $@
