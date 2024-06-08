#!/bin/bash

NCS_VERSION=v2.6.1

function help() {
    cat << END
usage: ./build DEVICE1 DEVICE2 ... ACTION1 ACTION2 ...

    DEVICE is in { uhk-80-left | uhk-80-right | uhk-60-right | uhk-dongle }
             there are also these aliases: { left | right | dongle | all }
    ACTION is in { clean | setup | update | build | make | flash | shell | uart }

    setup    initialize submodules and set up zephyr environment
    clean    removes zephyr libraries
    update   check out submodules after a git checkout
    build    full clean build with cmake execution
    make     just recompile / relink the binary
    flash    make and then flash
    shell    open build shell
    uart     open uhk shell

    Optionally run with "--dev-id 123" to select device for flashing.

    Or you can also specify dev ids in a .devices file, e.g.:

        DEVICEID_UHK80_LEFT=69660648
        DEVICEID_UHK80_RIGHT=69660578
        DEVICEID_UHK_DONGLE=683150769

    You can also set a command that will be executed after the builds are finished:

        BELL=mplayer ring.mp3

    Examples:
        ./build uhk-80-right flash --dev-id 123
        ./build all build flash
END
}

function processArguments() {
    DEVICES=""
    OTHER_ARGS=""

    while [ -n "$1" ]
    do
        case $1 in
            uhk-80-left|uhk-80-right|uhk-60-right|uhk-dongle)
                DEVICES="$DEVICES $1"
                shift
                ;;
            left)
                DEVICES="$DEVICES uhk-80-left"
                shift
                ;;
            right)
                DEVICES="$DEVICES uhk-80-right"
                shift
                ;;
            dongle)
                DEVICES="$DEVICES uhk-dongle"
                shift
                ;;
            all)
                DEVICES="$DEVICES uhk-80-left uhk-80-right uhk-dongle"
                shift
                ;;
            clean|setup|update|build|make|flash|shell|uart)
                ACTIONS="$ACTIONS $1"
                shift
                ;;
            *)
                OTHER_ARGS="$OTHER_ARGS $1"
                shift
                ;;
        esac
    done

    if [ "$ACTIONS" == "" ]
    then
        help
    fi
}

function determineDevIdArg() {
    DEVICE=$1
    DEVICEID=""

    case $DEVICE in
        uhk-80-left)
            DEVICEID="$DEVICEID_UHK80_LEFT"
            ;;
        uhk-80-right)
            DEVICEID="$DEVICEID_UHK80_RIGHT"
            ;;
        uhk-dongle)
            DEVICEID="$DEVICEID_UHK_DONGLE"
            ;;
        uhk-60)
            DEVICEID="$DEVICEID_UHK60"
            ;;
    esac


    if [ "$DEVICEID" == "" ]
    then
        cat > /dev/tty << END
        "In order to make your life comfortable, you can define your device
        ids in file .devices. Example content:

        DEVICEID_UHK80_LEFT=69660648
        DEVICEID_UHK80_RIGHT=69660578
        DEVICEID_UHK_DONGLE=683150769
END
    fi

    echo "--dev-id $DEVICEID"
}

function establishSession() {
    SESSION_NAME=$1
    NUM_PANES=$2

    tmux has-session -t $SESSION_NAME 2>/dev/null
    if [ $? != 0 ]; then
        tmux new-session -d -s $SESSION_NAME
        pane_count=1
        is_new_session=true
    else
        pane_count=$(tmux list-panes -t $SESSION_NAME | wc -l)
        is_new_session=false
    fi

    for ((i = $pane_count; i < $NUM_PANES; i++))
    do
        tmux split-window -t $SESSION_NAME
        tmux select-layout -t $SESSION_NAME tiled
    done
}

function setupUartMonitor() {
    SESSION_NAME="uartsession"
    establishSession $SESSION_NAME `ls /dev/ttyUSB* | wc -l`

    i=0
    for TTY in `ls /dev/ttyUSB*`
    do
        tmux send-keys -t $SESSION_NAME.$i "screen $TTY 115200" C-m
        i=$(( $i + 1 ))
    done

    if [ "$is_new_session" == "true" ]
    then
        tmux attach -t $SESSION_NAME
    fi
}

function performAction() {
    DEVICE=$1
    ACTION=$2
    PWD=`pwd`

    case $ACTION in
        update)
            git submodule update --init --recursive
            ;;
        clean)
            rm -rf device/build .west bootloader modules nrf nrfxlib test tools zephyr
            ;;
        setup)
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
            DEVICEARG=`determineDevIdArg $DEVICE`
            nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION << END
                west flash -d $PWD/device/build/$DEVICE $DEVICEARG $OTHER_ARGS < /dev/tty
END
            ;;
        shell)
            nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION
            ;;
        uart)
            setupUartMonitor
            ;;
        *)
            help
            ;;
    esac
}

function performActions() {
    DEVICE=$1
    for ACTION in $ACTIONS
    do
        performAction "$DEVICE" $ACTION
    done
    eval $BELL
}


function runPerDevice() {
    SESSION_NAME="buildsession"
    establishSession $SESSION_NAME `echo $DEVICES | wc -w`

    i=0
    for DEVICE in $DEVICES
    do
        tmux send-keys -t $SESSION_NAME.$i "$0 $DEVICE $ACTIONS" C-m
        i=$(( $i + 1 ))
    done

    if [ "$is_new_session" == "true" ]
    then
        tmux attach -t $SESSION_NAME
    fi
}

function run() {
    if [ -f .devices ]
    then
        source .devices
    fi

    processArguments $@

    echo "DEVICES = $DEVICES"
    echo "ACTIONS = $ACTIONS"

    if [ `echo $DEVICES | wc -w` -gt 1 ]
    then
        runPerDevice
    else
        performActions $DEVICES
    fi

}

run $@



