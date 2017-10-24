#!/usr/bin/env node
const program = require('commander');
require('shelljs/global');

config.fatal = true;

program
    .usage('update-slave-firmware <firmware-image>')
    .parse(process.argv)

const firmwareImage = program.args[0];

if (!firmwareImage) {
    echo('No firmware image specified');
    exit(1);
}

if (!firmwareImage.endsWith('.bin')) {
    echo('Firmware image extension is not .bin');
    exit(1);
}

if (!test('-f', firmwareImage)) {
    echo('Firmware image does not exist');
    exit(1);
}

var usbDir = '../../../lib/agent/packages/usb';
var usbBinding = usbDir + '/node_modules/usb/build/Release/usb_bindings.node';

let blhostPath;
switch (process.platform) {
    case 'linux':
        blhostPath = 'linux/amd64/blhost';
        break;
    case 'darwin':
        blhostPath = 'mac/blhost';
        break;
    case 'win32':
        blhostPath = 'win/blhost.exe';
        break;
    default:
        echo('Your operating system is not supported');
        exit(1);
        break;
}

var blhostUsb = `../../../lib/bootloader/bin/Tools/blhost/${blhostPath} --usb 0x1d50,0x6121`;
var blhostBuspal = blhostUsb + ' --buspal i2c,0x10,100k';

config.verbose = true;

exec(`${usbDir}/send-kboot-command-to-slave.js ping 0x10`);
exec(`${usbDir}/jump-to-slave-bootloader.js`);
exec(`${usbDir}/reenumerate.js buspal`);
exec(`${blhostBuspal} get-property 1`);
exec(`${blhostBuspal} flash-erase-all-unsecure`);
exec(`${blhostBuspal} write-memory 0x0 ${firmwareImage}`);
exec(`${blhostUsb} reset`);
exec(`${usbDir}/reenumerate.js normalKeyboard`);
exec(`${usbDir}/send-kboot-command-to-slave.js reset 0x10`);
