#!/bin/bash

NCS_VERSION=v2.6.1

BUILD_SESSION_NAME="buildsession"
UART_SESSION_NAME="uartsession"

function help() {
    cat << END
usage: ./build DEVICE1 DEVICE2 ... ACTION1 ACTION2 ...

    DEVICE is in { uhk-80-left | uhk-80-right | uhk-60-right | uhk-dongle }
             there are also these aliases: { left | right | dongle | all }
    ACTION is in { clean | setup | update | build | make | flash | flashUsb | shell | uart | addrline <address> }

    setup    initialize submodules and set up zephyr environment
    clean    removes zephyr libraries
    update   check out submodules after a git checkout
    build    full clean build with cmake execution
    make     just recompile / relink the binary
    flash    make and then flash
    flashUsb just flash via USB
    shell    open build shell
    uart     open uhk shell

    Optionally run with "--dev-id 123" to select device for flashing.

    Or you can also specify dev ids in a .devices file, e.g.:

        DEVICEID_UHK80_LEFT=69660648
        DEVICEID_UHK80_RIGHT=69660578
        DEVICEID_UHK_DONGLE=683150769

    You can also set a command that will be executed after the builds are finished,
    and a command that will be executed whenever this script is invoked:

        POSTBUILD=mplayer ring.mp3
        PREBUILD=./fixStuff.sh

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
            clean|setup|update|build|make|flash|flashUsb|shell|release)
                ACTIONS="$ACTIONS $1"
                TARGET_TMUX_SESSION=$BUILD_SESSION_NAME
                shift
                ;;
            addrline)
                shift
                ADDR=$1
                shift
                for device in $DEVICES
                do
                    echo "addrline for $ADDR:"
                    printf "    "
                    # addr2line -e device/build/$device/zephyr/zephyr.elf $ADDR
                    arm-none-eabi-addr2line -e device/build/$device/zephyr/zephyr.elf $ADDR
                done
                exit 0
                ;;
            uart)
                ACTIONS="$ACTIONS $1"
                TARGET_TMUX_SESSION=$UART_SESSION_NAME
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

mutex() {
  local lockfile="/tmp/uhk_build_lockfile"
  if [ "$1" == "lock" ]; then
    exec 200>"$lockfile"  # Open file descriptor 200 for the lockfile
    flock 200             # Block until lock is acquired
  elif [ "$1" == "unlock" ]; then
    flock -u 200          # Unlock the file descriptor
    rm -f "$lockfile"     # Remove lockfile
  fi
}

function determineUsbDeviceArg() {
    DEVICE=$1
    DEVICEUSBID=""

    case $DEVICE in
        uhk-80-left)
            DEVICEUSBID="--vid=0x37a8 --pid=7 --usb-interface=2"
            ;;
        uhk-80-right)
            DEVICEUSBID="--vid=0x37a8 --pid=9 --usb-interface=2"
            ;;
        uhk-dongle)
            DEVICEUSBID="--vid=0x37a8 --pid=5 --usb-interface=2"
            ;;
        uhk-60)
            ;;
    esac

    echo $DEVICEUSBID
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
        echo ""
    else
        echo "--dev-id $DEVICEID"
    fi
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

    tmux if-shell '[ "$(tmux display-message -p "#{pane_index}")" -gt '"$NUM_PANES"' ]' 'select-pane -t 1'
}

function setupUartMonitor() {
    SESSION_NAME=$TARGET_TMUX_SESSION
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

function createCentralCompileCommands() {
    TEMP_COMMANDS=`mktemp`

    echo creating central compile_commands.json

    jq -s 'add' $ROOT/device/build/*/compile_commands.json $ROOT/right/uhk60v2/compile_commands.json $ROOT/*/compile_commands.json > $TEMP_COMMANDS

    mv $TEMP_COMMANDS $ROOT/compile_commands.json
}

function performAction() {
    DEVICE=$1
    ACTION=$2
    ROOT=`realpath .`

    case $ACTION in
        update)
            git submodule update --init --recursive
            west update
            west patch
            west config --local build.cmake-args -- "-Wno-dev"
            cd "$ROOT/scripts"
            npm ci
            ./generate-versions.mjs
            ;;
        clean)
            rm -rf device/build .west bootloader modules nrf nrfxlib test tools zephyr
            ;;
        setup)
            # update this according to README
            git submodule init
            git submodule update --init --recursive
            cd "$ROOT/.."
            west init -l "$ROOT"
            west update
            west patch
            west config --local build.cmake-args -- "-Wno-dev"
            cd "$ROOT/scripts"
            npm i
            ./generate-versions.mjs
            ;;
        build)
            # reference version of the build process is to be found in scripts/make-release.mjs
            nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION << END
                unset PYTHONPATH
                unset PYTHONHOME
                ZEPHYR_TOOLCHAIN_VARIANT=zephyr west build --build-dir "$ROOT/device/build/$DEVICE" "$ROOT/device" --pristine --board "$DEVICE" --no-sysbuild -- -DNCS_TOOLCHAIN_VERSION=NONE -DEXTRA_CONF_FILE=prj.conf.overlays/$DEVICE.prj.conf -DBOARD_ROOT="$ROOT" -Dmcuboot_OVERLAY_CONFIG="$ROOT/device/child_image/mcuboot.conf;$ROOT/device/child_image/$DEVICE.mcuboot.conf"

END
            createCentralCompileCommands
            ;;
        make)
            nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION << END
                west build --build-dir $ROOT/device/build/$DEVICE device
END
            ;;
        flash)
            DEVICEARG=`determineDevIdArg $DEVICE`
            nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION << END
                west flash -d $ROOT/device/build/$DEVICE $DEVICEARG $OTHER_ARGS < /dev/tty
END
            ;;
        flashUsb)
            USBDEVICEARG=`determineUsbDeviceArg $DEVICE`
            USB_SCRIPT_DIR=$ROOT/lib/agent/packages/usb/
            cd $USB_SCRIPT_DIR
            echo "running $USB_SCRIPT_DIR$ ./update-device-firmware.ts $USBDEVICEARG $ROOT/device/build/$DEVICE/zephyr/app_update.bin $OTHER_ARGS"
            mutex lock
            ./update-device-firmware.ts $USBDEVICEARG $ROOT/device/build/$DEVICE/zephyr/app_update.bin $OTHER_ARGS
            mutex unlock
            cd $ROOT
            ;;
        release)
            nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION << END
                scripts/make-release.mjs --allowSha
END
            ;;
        shell)
            nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION
            ;;
        uart)
            setupUartMonitor
            ;;
        addrline)
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
    eval $POSTBUILD
}

function runPerDevice() {
    SESSION_NAME=$TARGET_TMUX_SESSION
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
        eval $PREBUILD
    fi

    processArguments $@

    echo "DEVICES = $DEVICES"
    echo "ACTIONS = $ACTIONS"

    if [ `echo $DEVICES | wc -w` -gt 1 ]
    then
        runPerDevice
    elif [ `echo $DEVICES | wc -w` -eq 0 ] 
    then
        performActions $DEVICES
    else
        tmux has-session -t $TARGET_TMUX_SESSION 2>/dev/null
        SESSION_EXISTS=$?
        if [ $SESSION_EXISTS == 0 -a "$TMUX" == "" ] 
        then
            runPerDevice
        else
            performActions $DEVICES
        fi
    fi

}

run $@



