#!/bin/env node

// This script logs battery stats and STAT pin transitions to CSV files.
// Before running it, mkdir battery-stats and stat-changes directories.
// Use `npm i` to install the required dependencies.
//
// (Optional) create fixed dev points:
//
//   cp 99-usb-serial.rules /etc/udev/rules.d/
//   Modify the udev file to reflect the serial numbers of your adapters.
//   Run sudo udevadm control --reload-rules
//   Afterward, your serial to USB adapters should be available at /dev/ttyUSB-{left,right,dongle}
//
// Finally, run the script with log-battery.mjs left|right|dongle
// Or to use raw /dev/ttyUSBx, run just log-battery.mjs 0

import fs from 'fs';
import moment from 'moment';
import {SerialPort} from 'serialport';
import {ReadlineParser} from '@serialport/parser-readline';
const startTime = Date.now();

const args = process.argv.slice(2);
const half = /^[a-zA-Z]/.test(args[0]) ? `-${args[0]}` : args[0];

const devFilename = `/dev/ttyUSB${half}`;
const port = new SerialPort({path: devFilename, baudRate: 115200})

const now = moment();
const dateString = now.format('YYYY-MM-DD_HH:mm:ss');

const csvFilename = `battery-stats/${dateString}_${half}.csv`;
const csvFile = fs.createWriteStream(csvFilename);
csvFile.write('minutes,charger_en,stat,vbat_raw,vbat_mV\n');

const statFilename = `stat-changes/${dateString}_${half}.csv`;
const statFile = fs.createWriteStream(statFilename);
statFile.write('minutes,charger_en,stat,vbat_raw,vbat_mV,statChangedTo\n');

const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }))

port.on('error', function(err) {
  console.log('Error: ', err.message);
})

let parsedData = {};
parser.on('data', (data) => {
  const elapsedTime = Math.round((Date.now() - startTime) / 1000 / 60);
  console.log('Read data: ', data.toString());

  const chargerRegex = /CHARGER_EN: (\d) \| STAT: (\d) \| VBAT: (\d+) = (\d+) mV/;
  let match = data.match(chargerRegex);
  if (match) {
    parsedData = {
      charger_en: parseInt(match[1], 10),
      stat: parseInt(match[2], 10),
      vbat: {
        raw: parseInt(match[3], 10),
        mV: parseInt(match[4], 10)
      }
    };
    console.log(parsedData);
    csvFile.write(`${elapsedTime},${parsedData.charger_en},${parsedData.stat},${parsedData.vbat.raw},${parsedData.vbat.mV}\n`);
  }

  const statRegex = /STAT changed to (\d)/;
  match = data.match(statRegex);
  if (match) {
    const statChangedTo = parseInt(match[1], 10);
    console.log(match[0], statChangedTo);
    statFile.write(`${elapsedTime},${parsedData.charger_en},${parsedData.stat},${parsedData.vbat.raw},${parsedData.vbat.mV},${statChangedTo}\n`);
  }
});

function sendCommand(command) {
  port.write(command + '\r', function(err) {
    if (err) {
      return console.log('Error on write: ', err.message);
    }
    console.log('command sent:', command);
  });
}

async function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

sendCommand('uhk charger');
sendCommand('uhk statlog 1');
while (true) {
  sendCommand('uhk charger');
  await sleep(60000);
}
