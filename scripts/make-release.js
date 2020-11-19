#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
require('shelljs/global');

config.fatal = true;
config.verbose = true;

exec(`${__dirname}/generate-versions-h.js`);

const package = JSON.parse(fs.readFileSync(`${__dirname}/package.json`));
const version = package.firmwareVersion;
const releaseName = `uhk-firmware-${version}`;
const releaseDir = `${__dirname}/${releaseName}`;
const releaseFile = `${__dirname}/${releaseName}.tar.gz`;
const agentDir = `${__dirname}/../lib/agent`;

const deviceSourceFirmwares = package.devices.map(device => `${__dirname}/../${device.source}`);
const moduleSourceFirmwares = package.modules.map(module => `${__dirname}/../${module.source}`);
rm('-rf', releaseDir, releaseFile, deviceSourceFirmwares, moduleSourceFirmwares);

const sourcePaths = [
    ...package.devices.map(device => device.source),
    ...package.modules.map(module => module.source),
];
for (sourcePath of sourcePaths) {
    const buildDir = path.dirname(`${__dirname}/../${sourcePath}`);
    mkdir('-p', buildDir);
    exec(`cd ${buildDir}/..; make clean; make -j8`);
}

// --skip-agent: don't build Agent, just get minimal dependencies
if (process.argv.slice(2).includes("--skip-agent")) {
    exec('npm ci', { cwd: agentDir });
} else {
    exec(`git pull origin master; git checkout master; npm ci; npm run build`, { cwd: agentDir });
}

for (const device of package.devices) {
    const deviceDir = `${releaseDir}/devices/${device.name}`;
    const deviceSource = `${__dirname}/../${device.source}`;
    mkdir('-p', deviceDir);
    chmod(644, deviceSource);
    cp(deviceSource, `${deviceDir}/firmware.hex`);
    exec(`npm run convert-user-config-to-bin -- ${deviceDir}/config.bin`, { cwd: agentDir });
}

for (const module of package.modules) {
    const moduleDir = `${releaseDir}/modules`;
    const moduleSource = `${__dirname}/../${module.source}`;
    mkdir('-p', moduleDir);
    chmod(644, moduleSource);
    cp(moduleSource, `${moduleDir}/${module.name}.bin`);
}

cp(`${__dirname}/package.json`, releaseDir);
exec(`tar -czvf ${releaseFile} -C ${releaseDir} .`);
