#!/bin/bash

NCS_VERSION=v2.6.0

function help() {
    cat << END
usage: ./build [ uhk-80-left | uhk-80-right | uhk-60-right | uhk-dongle ] { setup | update | build | make | flash | shell | uart }"

    setup    initialize submodules and set up zephyr environment
    update   check out submodules after a git checkout
    build    full clean build with cmake execution
    make     just recompile / relink the binary
    flash    make and then flash
    shell    open build shell
    uart     open uhk shell
END
}

DEVICE=$1

case $DEVICE in 
    uhk-80-left|uhk-80-right|uhk-60-right|uhk-dongle)
        shift
        ;;
    *)
        DEVICE=uhk-80-right
        ;;
esac

ACTION=$1
PWD=`pwd`

if [ "$ACTION" == "" -o "$DEVICE" == "" ] 
then
    help
fi

case $ACTION in
    update)
        git submodule update --init --recursive
        ;;
    setup)
        rm -rf device/build .west bootloader modules nrf nrfxlib test tools zephyr
        git submodule init
        git submodule update --init --recursive
        nrfutil install toolchain-manager 
        nrfutil toolchain-manager install --ncs-version $NCS_VERSION
        nrfutil toolchain-manager launch --shell << END
            west init -m https://github.com/nrfconnect/sdk-nrf --mr $NCS_VERSION
            west update
            west zephyr-export
END
        ;;
    build)
        nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION << END
            unset PYTHONPATH
            unset PYTHONHOME
            west build --build-dir $PWD/device/build/$DEVICE $PWD/device --pristine --board $DEVICE --no-sysbuild -- -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DNCS_TOOLCHAIN_VERSION=NONE -DCONF_FILE=$PWD/device/prj.conf -DOVERLAY_CONFIG=$PWD/device/prj.conf.overlays/$DEVICE.prj.conf -DBOARD_ROOT=$PWD
END
        ;;
    make)
        nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION << END
            west build --build-dir $PWD/device/build/$DEVICE device
END
        ;;
    flash)
        nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION << END
            west flash -d $PWD/device/build/$DEVICE < /dev/tty
END
        ;;
    shell)
        nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION
        ;;
    uart)
        tmux new-session -d -s mysession \; \
            send-keys "screen /dev/ttyUSB0 115200" C-m \; \
            split-window -v \; \
            send-keys "screen /dev/ttyUSB1 115200" C-m
        tmux attach-session -t mysession
        ;;
    *)
        help
        ;;
esac







