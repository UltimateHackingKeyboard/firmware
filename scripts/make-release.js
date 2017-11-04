#!/usr/bin/env node
const fs = require('fs');
require('shelljs/global');

config.fatal = true;
config.verbose = true;

const package = JSON.parse(fs.readFileSync(`${__dirname}/package.json`));
const version = package.version;
const releaseName = `uhk-firmware-${version}`;
const releaseDir = `${__dirname}/${releaseName}`;
const slavesDir = `${releaseDir}/slaves`;
const releaseFile = `${__dirname}/${releaseName}.tar.bz2`;
const leftFirmwareFile = `${__dirname}/../left/build/uhk60-left_release/uhk-left.bin`;
const usbDir = `${__dirname}/../lib/agent/packages/usb`;

const masterSourceFirmwares = package.masters.map(master => `${__dirname}/../${master.source}`);
const slaveSourceFirmwares = package.slaves.map(slave => `${__dirname}/../${slave.source}`);
rm('-rf', releaseDir, releaseFile, masterSourceFirmwares, slaveSourceFirmwares);

exec(`/opt/Freescale/KDS_v3/eclipse/kinetis-design-studio \
--launcher.suppressErrors \
-noSplash \
-application org.eclipse.cdt.managedbuilder.core.headlessbuild \
-import ${__dirname}/../left/build \
-import ${__dirname}/../right/build \
-cleanBuild uhk-left \
-cleanBuild uhk-right`
);

for (let master of package.masters) {
    const masterDir = `${releaseDir}/masters/${master.name}`;
    const masterSource = `${__dirname}/../${master.source}`;
    mkdir('-p', masterDir);
    chmod(644, masterSource);
    cp(masterSource, `${masterDir}/firmware.hex`);
    exec(`${usbDir}/user-config-json-to-bin.ts ${masterDir}/config.bin`);
}

for (let slave of package.slaves) {
    const slaveDir = `${releaseDir}/slaves`;
    const slaveSource = `${__dirname}/../${slave.source}`;
    mkdir('-p', slaveDir);
    chmod(644, slaveSource);
    cp(slaveSource, `${slaveDir}/${slave.name}.bin`);
}

cp(`${__dirname}/package.json`, releaseDir);
exec(`tar -cvjSf ${releaseFile} -C ${releaseDir} .`);
