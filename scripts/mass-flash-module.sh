#!/usr/bin/env bash
module=$1
while true; do
    read
    ./flash-module.sh $module
done
