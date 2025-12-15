#!/bin/bash

NCS_VERSION=v2.8.0

ROOT_HASH=`realpath . | md5sum | sed 's/ .*//g'`
BUILD_SESSION_NAME="buildsession_$ROOT_HASH"
UART_SESSION_NAME="uartsession_$ROOT_HASH"
BAUD_RATE=`cat boards/ugl/uhk-80/shared.dtsi | grep "current-speed" | grep -o '[0-9][0-9]*'`

function help() {
    cat << END
usage: ./build DEVICE1 DEVICE2 ... ACTION1 ACTION2 ...

    DEVICE is in { uhk-80-left | uhk-80-right | uhk-60-right | uhk-dongle }
             there are also these aliases: { left | right | dongle | all }
    ACTION is in { clean | setup | update | build | make | flash | flashUsb | shell | uart | addrline <address> }

    setup         initialize submodules and set up zephyr environment
    clean         removes zephyr libraries
    update        check out submodules after a git checkout
    build         full clean build with cmake execution
    make          just recompile / relink the binary
    flash         make and then flash
    flashUsb      just flash via USB
    shell         open build shell
    uart          open uhk shell
    switchMcux    switch to UHK60 build environment
    switchZephyr  switch to UHK80 build environment

    Optionally run with "--dev-id 123" to select device for flashing.

    Or you can also specify dev ids in a .devices file, e.g.:

        DEVICEID_UHK80_LEFT=69660648
        DEVICEID_UHK80_RIGHT=69660578
        DEVICEID_UHK_DONGLE=683150769
        DEVICEID_UHK_MODULE=69665303

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
            uhk-80-left|uhk-80-right|uhk-60v1-right|uhk-60v2-right|uhk-dongle|trackball|trackpoint|keycluster)
                DEVICES="$DEVICES $1"
                shift
                ;;
            left|right|dongle|rightv1|rightv2)
                DEVICES="$DEVICES $1"
                shift
                ;;
            all)
                DEVICES="$DEVICES left right dongle"
                shift
                ;;
            both)
                DEVICES="$DEVICES left right"
                shift
                ;;
            clean|setup|update|build|make|flash|shell|release)
                MULTIPLEXED_ACTIONS="$MULTIPLEXED_ACTIONS $1"
                TARGET_TMUX_SESSION=$BUILD_SESSION_NAME
                shift
                ;;
            flashUsb)
                MULTIPLEXED_ACTIONS="$MULTIPLEXED_ACTIONS make $1"
                TARGET_TMUX_SESSION=$BUILD_SESSION_NAME
                shift
                ;;
            switchMcux|switchZephyr)
                SINGLEPLEXED_ACTIONS="$SINGLEPLEXED_ACTIONS $1"
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
                    arm-none-eabi-addr2line -e device/build/$device/device/zephyr/zephyr.elf $ADDR
                done
                exit 0
                ;;
            uart)
                TERMINAL_ACTIONS="$TERMINAL_ACTIONS $1"
                TARGET_TMUX_SESSION=$UART_SESSION_NAME
                shift
                ;;
            help|--help|-h)
                help
                exit 0
                ;;
            *)
                OTHER_ARGS="$OTHER_ARGS $1"
                shift
                ;;
        esac
    done

    if [ "$SINGLEPLEXED_ACTIONS $MULTIPLEXED_ACTIONS $TERMINAL_ACTIONS" == "  " ]
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

function dealiasDeviceZephyr() {
    case $DEVICE in
        uhk-80-left|left)
            DEVICE="uhk-80-left"
            USBDEVICEARG="--vid=0x37a8 --pid=7"
            DEVICEARG="--dev-id $DEVICEID_UHK80_LEFT"
            BUILD_DIR="device/build/$DEVICE"
            ;;
        uhk-80-right|right)
            DEVICE="uhk-80-right"
            USBDEVICEARG="--vid=0x37a8 --pid=9"
            DEVICEARG="--dev-id $DEVICEID_UHK80_RIGHT"
            BUILD_DIR="device/build/$DEVICE"
            ;;
        uhk-dongle|dongle)
            DEVICE="uhk-dongle"
            USBDEVICEARG="--vid=0x37a8 --pid=1"
            DEVICEARG="--dev-id $DEVICEID_UHK_DONGLE"
            BUILD_DIR="device/build/$DEVICE"
            ;;
        *)
            echo "$DEVICE is not a valid zephyr device name!"
            exit 1
    esac
}

function dealiasDeviceMcux() {
    case $DEVICE in
        uhk-60v1-right|rightv1|right)
            DEVICE="uhk-60v1-right"
            VARIANT="v1-release"
            BUILD_DIR="right/build/$VARIANT"
            DEVICE_DIR="right"
            USBDEVICEARG="--vid=0x37a8 --pid=1"
            ;;
        uhk-60v2-right|rightv2|right)
            DEVICE="uhk-60v2-right"
            VARIANT="v2-release"
            BUILD_DIR="right/build/$VARIANT"
            DEVICE_DIR="right"
            USBDEVICEARG="--vid=0x37a8 --pid=3"
            ;;
        trackpoint|trackball|keycluster)
            DEVICE="$DEVICE"
            VARIANT="release"
            BUILD_DIR="$DEVICE/build/$VARIANT"
            DEVICE_DIR="$DEVICE"
            DEVICEARG="-SelectEmuBySN $DEVICEID_UHK_MODULE"
            ;;
        *)
            echo "$DEVICE is not a valid mcux device name!"
            exit 1
    esac
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
        IDX=`echo $TTY | grep -o '[0-9][0-9]*'`
        INNER_COMMAND="screen $TTY $BAUD_RATE"
        COMMAND="script -f log.$IDX.txt -c \"$INNER_COMMAND\""
        tmux send-keys -t $SESSION_NAME.$i "$COMMAND" C-m
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

    local existing_jsons=` $ROOT/device/build/ $ROOT/right/build/ $ROOT/trackball/build $ROOT/trackpoint/build $ROOT/keycluster/build -name "compile_commands.json" 2>/dev/null`

    if [ "$existing_jsons" != "" ] 
    then 
        jq -s 'add' $existing_jsons > $TEMP_COMMANDS
        mv $TEMP_COMMANDS $ROOT/compile_commands.json
    fi
}

function upgradeEnv() {
    git submodule update --init --recursive
    cd "$ROOT/.."
    west update
    west patch
    cd "$ROOT"
}

function setFallbackArmGccEnv() {
    if [ "$ARM_GCC_DIR" == "" ]
    then
        if [[ "$OSTYPE" == "linux-gnu"* ]]; then
            export ARM_GCC_DIR="/usr"
        elif [[ "$OSTYPE" == "darwin"* ]]; then
            export ARM_GCC_DIR="/opt/homebrew"
        fi
    fi
}

function performMcuxAction() {
    DEVICE=$1
    ACTION=$2
    ROOT=`realpath .`

    dealiasDeviceMcux $DEVICE

    case $ACTION in
        build)
            rm -rf $BUILD_DIR
            west build --build-dir "$BUILD_DIR" "$DEVICE_DIR" --pristine -- --preset "$VARIANT"
            exitOnFail $?

            createCentralCompileCommands
            ;;
        make)
            west build --build-dir "$BUILD_DIR" "$DEVICE_DIR" -- --preset "$VARIANT"
            exitOnFail $?
            ;;

        flash)
            mutex lock
            JLinkExe -if SWD -speed 4000 -device MKL03Z32xxx4 -autoconnect 1 $DEVICEARG -CommanderScript "scripts/flash-$DEVICE.jlink"
            mutex unlock
            exitOnFail $?
            ;;
        flashUsb)
            mutex lock
            west agent --build-dir $BUILD_DIR
            mutex unlock
            exitOnFail $?
            ;;
    esac
}

function exitOnFail() {
    if [ $1 -ne 0 ]
    then
        echo "Previous command failed with exit code $1"
        exit $1
    fi
}

function performZephyrAction() {
    DEVICE=$1
    ACTION=$2
    ROOT=`realpath .`

    dealiasDeviceZephyr

    case $ACTION in
        build)
            # reference version of the build process is to be found in scripts/make-release.mjs
            nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION << END
                unset PYTHONPATH
                unset PYTHONHOME
                ZEPHYR_TOOLCHAIN_VARIANT=zephyr west build \
                    --build-dir "$ROOT/device/build/$DEVICE" "$ROOT/device" \
                    --pristine \
                    -- \
                    --preset $DEVICE
                exit $?
END
            exitOnFail $?
            createCentralCompileCommands
            ;;
        make)
            nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION << END
                west build --build-dir $ROOT/device/build/$DEVICE device
                exit $?
END
            exitOnFail $?
            ;;
        flash)
            export BUILD_DIR="$ROOT/device/build/$DEVICE"
            export DEVICE="$DEVICE"
            nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION << END
                west flash --softreset --build-dir $BUILD_DIR $DEVICEARG $OTHER_ARGS
                exit $?
END
            exitOnFail $?
            ;;
        flashUsb)
            mutex lock
            west agent --build-dir $BUILD_DIR
            mutex unlock
            ;;
    esac
}

function performAction() {
    DEVICE=$1
    ACTION=$2
    ROOT=`realpath .`

    case $ACTION in
        update)
            upgradeEnv
            ;;
        clean)
            rm -rf ../bootloader  ../c2usb  ../hal_nxp  ../modules  ../nrf  ../nrfxlib  ../zephyr ../.west ../mcuxsdk
            ;;
        setup)
            # basic dependencies
            if ! command -v nrfutil &> /dev/null;
            then
                echo "Going to download nrfutil into /usr/local/bin. Please, either authorize it, or install nrfutil independently according to https://docs.nordicsemi.com/bundle/nrfutil/page/guides/installing.html"
                sudo curl https://files.nordicsemi.com/artifactory/swtools/external/nrfutil/executables/x86_64-unknown-linux-gnu/nrfutil -o /usr/local/bin/nrfutil
                sudo chmod +x /usr/local/bin/nrfutil
            fi
            pip3 install -r scripts/requirements.txt
            nrfutil install toolchain-manager
            nrfutil toolchain-manager install --ncs-version $NCS_VERSION
            # update following according to README
            git submodule init
            git submodule update --init --recursive
            npm install
            cd "$ROOT/scripts"
            npm install
            cd "$ROOT/.."
            rm -rf "$ROOT/../.west"
            west init -l "$ROOT" --mf west_nrfsdk.yml
            west config --local build.cmake-args -- "-Wno-dev"
            upgradeEnv
            ;;
        switchMcux)
            west config manifest.file west_mcuxsdk.yml
            upgradeEnv
            ;;
        switchZephyr)
            west config manifest.file west_nrfsdk.yml
            upgradeEnv
            ;;
        make|build|flash|flashUsb)
            if [ `west config manifest.file` == "west_nrfsdk.yml" ]
            then
                performZephyrAction $DEVICE $ACTION
            else
                performMcuxAction $DEVICE $ACTION
            fi
            ;;
        release)
            scripts/make-release.mjs --allowSha
            # nrfutil toolchain-manager launch --shell --ncs-version $NCS_VERSION << END
            #     scripts/make-release.mjs --allowSha
# END
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

    setFallbackArmGccEnv

    processArguments $@

    echo "DEVICES = $DEVICES"
    echo "ACTIONS = $ACTIONS"
    
    if [ "$TERMINAL_ACTIONS" != "" ]
    then
        ACTIONS="$TERMINAL_ACTIONS"
        performActions $DEVICES
        exit 0
    fi

    if [ "$SINGLEPLEXED_ACTIONS" != "" ]
    then
        ACTIONS="$SINGLEPLEXED_ACTIONS"
        performActions "none"
    fi

    if [ "$MULTIPLEXED_ACTIONS" != "" ]
    then
        ACTIONS="$MULTIPLEXED_ACTIONS"

        if [ `echo $DEVICES | wc -w` -gt 1 ]
        then
            runPerDevice
        elif [ `echo $DEVICES | wc -w` -le 1 ] 
        then
            performActions $DEVICES
        fi
        # else
        #     tmux has-session -t $TARGET_TMUX_SESSION 2>/dev/null
        #     SESSION_EXISTS=$?
        #     if [ $SESSION_EXISTS == 0 -a "$TMUX" == "" ] 
        #     then
        #         runPerDevice
        #     else
        #         performActions $DEVICES
        #     fi
        # fi
    fi


}

run $@
