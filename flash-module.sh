#!/usr/bin/env bash
module=$1
JLinkExe -ExitOnError 1 -If SWD << EOF
power on
speed 4000
device MKL03Z32xxx4
connect
loadfile /home/laci/projects/firmware/${module}/build_make/uhk_${module}.bin
exit
EOF
