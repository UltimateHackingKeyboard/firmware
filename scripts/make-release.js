#!/usr/bin/env node
const fs = require('fs');
require('shelljs/global');

config.fatal = true;
config.verbose = true;

const package = JSON.parse(fs.readFileSync(`${__dirname}/package.json`));
const version = package.firmwareVersion;
const releaseName = `uhk-firmware-${version}`;
const releaseDir = `${__dirname}/${releaseName}`;
const modulesDir = `${releaseDir}/modules`;
const releaseFile = `${__dirname}/${releaseName}.tar.bz2`;
const leftFirmwareFile = `${__dirname}/../left/build/uhk60-left_release/uhk-left.bin`;
const usbDir = `${__dirname}/../lib/agent/packages/usb`;

const deviceSourceFirmwares = package.devices.map(device => `${__dirname}/../${device.source}`);
const moduleSourceFirmwares = package.modules.map(module => `${__dirname}/../${module.source}`);
rm('-rf', releaseDir, releaseFile, deviceSourceFirmwares, moduleSourceFirmwares);

exec(`/opt/Freescale/KDS_v3/eclipse/kinetis-design-studio \
--launcher.suppressErrors \
-noSplash \
-application org.eclipse.cdt.managedbuilder.core.headlessbuild \
-import ${__dirname}/../left/build \
-import ${__dirname}/../right/build \
-cleanBuild uhk-left \
-cleanBuild uhk-right`
);

for (let device of package.devices) {
    const deviceDir = `${releaseDir}/devices/${device.name}`;
    const deviceSource = `${__dirname}/../${device.source}`;
    mkdir('-p', deviceDir);
    chmod(644, deviceSource);
    cp(deviceSource, `${deviceDir}/firmware.hex`);
    exec(`${usbDir}/user-config-json-to-bin.ts ${deviceDir}/config.bin`);
}

for (let module of package.modules) {
    const moduleDir = `${releaseDir}/modules`;
    const moduleSource = `${__dirname}/../${module.source}`;
    mkdir('-p', moduleDir);
    chmod(644, moduleSource);
    cp(moduleSource, `${moduleDir}/${module.name}.bin`);
}

cp(`${__dirname}/package.json`, releaseDir);
exec(`tar -cvjSf ${releaseFile} -C ${releaseDir} .`);
