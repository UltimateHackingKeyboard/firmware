#!/bin/env node

const fs = require('fs');
const csvString = fs.readFileSync('untitled.csv', {encoding:'utf8'});
const csvLines = csvString.split('\n');
let isTouchpad = false;
for (const line of csvLines) {
    const i2cAddressMatch = line.match(/\[(.+)\]/);
    const i2cAddress = i2cAddressMatch && i2cAddressMatch[1];
    if (i2cAddress) {
        isTouchpad = ['0x5A', '0x5B'].includes(i2cAddress);
    }
    if (isTouchpad) {
        console.log(line);
    }
}
