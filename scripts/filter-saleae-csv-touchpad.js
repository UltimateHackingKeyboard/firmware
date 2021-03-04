#!/bin/env node

const fs = require('fs');
const csvString = fs.readFileSync('untitled.csv', {encoding:'utf8'});
const csvLines = csvString.split('\n');

const touchpadOperation = {'0x5A':'write', '0x5B':'read'};
let prevOp;
let op;
let buffer = [];
let getCoordinates = false;

for (const line of csvLines) {
    const i2cAddressMatch = line.match(/([0123456789\.]+).*\[(.+)\]/);
    const time = i2cAddressMatch && i2cAddressMatch[1];
    const i2cAddress = i2cAddressMatch && i2cAddressMatch[2];
    if (i2cAddress) {
        op = touchpadOperation[i2cAddress];
        if (op) {
            if (getCoordinates) {
                const y = (buffer[1] | buffer[0]<<8) <<16 >>16;
                const x = (buffer[3] | buffer[2]<<8) <<16 >>16;
                console.log(time, x, y)
            }
            getCoordinates = prevOp === 'write' && JSON.stringify(buffer) === JSON.stringify(['0x00', '0x12']);
            buffer = [];
            prevOp = op;
        }
    }
    const i2cValueMatch = line.match(/(0x[0123456789ABCDEF][0123456789ABCDEF]) \+ (ACK|NAK)/);
    const i2cValue = i2cValueMatch && i2cValueMatch[1];
    if (i2cValue && op) {
        buffer.push(i2cValue);
    }
}
