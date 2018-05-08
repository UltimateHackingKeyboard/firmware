#!/usr/bin/env node
const fs = require('fs');
require('shelljs/global');

config.fatal = true;
config.verbose = true;

exec(`${__dirname}/generate-versions-h.js`);

const package = JSON.parse(fs.readFileSync(`${__dirname}/package.json`));
const version = package.firmwareVersion;
const releaseName = `uhk-firmware-${version}`;
const releaseDir = `${__dirname}/${releaseName}`;
const modulesDir = `${releaseDir}/modules`;
const releaseFile = `${__dirname}/${releaseName}.tar.bz2`;
const leftFirmwareFile = `${__dirname}/../left/build_make/uhk_left.bin`;
const usbDir = `${__dirname}/../lib/agent/packages/usb`;

const deviceSourceFirmwares = package.devices.map(device => `${__dirname}/../${device.source}`);
const moduleSourceFirmwares = package.modules.map(module => `${__dirname}/../${module.source}`);
rm('-rf', releaseDir, releaseFile, deviceSourceFirmwares, moduleSourceFirmwares);

exec(`cd ${__dirname}/../left; make clean; make -j8 -B`);
exec(`cd ${__dirname}/../right; make clean; make -j8 -B`);

for (const device of package.devices) {
    const deviceDir = `${releaseDir}/devices/${device.name}`;
    const deviceSource = `${__dirname}/../${device.source}`;
    mkdir('-p', deviceDir);
    chmod(644, deviceSource);
    cp(deviceSource, `${deviceDir}/firmware.hex`);
    exec(`cd ${usbDir}; git pull origin master; git checkout master`);
    exec(`${usbDir}/user-config-json-to-bin.ts ${deviceDir}/config.bin`);
}

for (const module of package.modules) {
    const moduleDir = `${releaseDir}/modules`;
    const moduleSource = `${__dirname}/../${module.source}`;
    mkdir('-p', moduleDir);
    chmod(644, moduleSource);
    cp(moduleSource, `${moduleDir}/${module.name}.bin`);
}

cp(`${__dirname}/package.json`, releaseDir);
exec(`tar -cvjSf ${releaseFile} -C ${releaseDir} .`);
