#!/bin/bash
repo_dirs=`realpath ../../../nrfconnect/{bootloader/mcuboot,nrf,zephyr}`
for repo_dir in $repo_dirs; do
    git stash
done
