#!/bin/env node
import fs from 'fs';
import moment from 'moment';
import {SerialPort} from 'serialport';
import {ReadlineParser} from '@serialport/parser-readline';
const startTime = Date.now();

const args = process.argv.slice(2);
const half = args[0];

const devFilename = `/dev/ttyUSB-${half}`;
const port = new SerialPort({path: devFilename, baudRate: 115200})

const now = moment();
const dateString = now.format('YYYY-MM-DD_HH:mm:ss');
const csvFilename = `battery-stats/${dateString}_${half}.csv`;
const csvFile = fs.createWriteStream(csvFilename);
csvFile.write('minutes,charger_en,stat,ts_raw,ts_mV,vbat_raw,vbat_mV\n');

const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }))

port.on('error', function(err) {
  console.log('Error: ', err.message);
})

parser.on('data', (data) => {
  console.log('Read data: ', data.toString());
  const regex = /CHARGER_EN: (\d) \| STAT: (\d) \| TS: (\d+) = (\d+) mV \| VBAT: (\d+) = (\d+) mV/;
  const match = data.match(regex);
  if (match) {
    const parsedData = {
      charger_en: parseInt(match[1], 10),
      stat: parseInt(match[2], 10),
      ts: {
        raw: parseInt(match[3], 10),
        mV: parseInt(match[4], 10)
      },
      vbat: {
        raw: parseInt(match[5], 10),
        mV: parseInt(match[6], 10)
      }
    };
    console.log(parsedData);

    const elapsedTime = Math.round((Date.now() - startTime) / 1000 / 60);
    csvFile.write(`${elapsedTime},${parsedData.charger_en},${parsedData.stat},${parsedData.ts.raw},${parsedData.ts.mV},${parsedData.vbat.raw},${parsedData.vbat.mV}\n`);
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

while (true) {
  sendCommand('uhk charger');
  await sleep(60000);
}
