#!/usr/bin/env node
const program = require('commander');
require('shelljs/global');
require('./shared')

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

const usbDir = '../../../lib/agent/packages/usb';
const blhostUsb = getBlhostCmd();
const blhostBuspal = blhostUsb + ' --buspal i2c,0x10,100k';

config.verbose = true;
exec(`${usbDir}/send-kboot-command-to-slave.js ping 0x10`);
exec(`${usbDir}/jump-to-slave-bootloader.js`);
exec(`${usbDir}/reenumerate.js buspal`);
execRetry(`${blhostBuspal} get-property 1`);
exec(`${blhostBuspal} flash-erase-all-unsecure`);
exec(`${blhostBuspal} write-memory 0x0 ${firmwareImage}`);
exec(`${blhostUsb} reset`);
exec(`${usbDir}/reenumerate.js normalKeyboard`);
execRetry(`${usbDir}/send-kboot-command-to-slave.js reset 0x10`);
config.verbose = false;

echo('Firmware updated successfully');
