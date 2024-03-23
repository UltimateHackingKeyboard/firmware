#!/usr/bin/env node
import fs from 'fs';
import path from 'path';
import {getGitInfo} from './common.mjs';
import shell from 'shelljs';
import {generateVersionsH} from './generate-versions-h-util.mjs';
import {readPackageJson} from './read-package-json.mjs';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

shell.config.fatal = true;
shell.config.verbose = true;

const gitInfo = getGitInfo();
const packageJson = readPackageJson();

generateVersionsH({ packageJson, gitInfo, useRealData: false });

const version = packageJson.firmwareVersion;
const releaseName = `uhk-firmware-${version}`;
const releaseDir = `${__dirname}/${releaseName}`;
const agentDir = `${__dirname}/../lib/agent`;
var releaseFile = `${__dirname}/${releaseName}.tar.gz`;
var mkArgs = '';

if (gitInfo.tag !== `v${version}` && !process.argv.includes('--allowSha')) {
    console.error(`Git tag '${gitInfo.tag}' !~ 'v{version}'. Please run with '--allowSha' if this is intentional.`);
    process.exit(1);
}

if (gitInfo.tag !== `v${version}`) {
    releaseFile = `${__dirname}/${releaseName}-${gitInfo.tag}.tar.gz`;
}

const deviceSourceFirmwares = packageJson.devices.map(device => `${__dirname}/../${device.source}`);
const moduleSourceFirmwares = packageJson.modules.map(module => `${__dirname}/../${module.source}`);
shell.rm('-rf', releaseDir, releaseFile, deviceSourceFirmwares, moduleSourceFirmwares);

const sourcePaths = [
    ...packageJson.devices.map(device => device.source),
    ...packageJson.modules.map(module => module.source),
];
for (const sourcePath of sourcePaths) {
    const buildDir = path.dirname(`${__dirname}/../${sourcePath}`);
    shell.mkdir('-p', buildDir);
    if (!buildDir.includes('zephyr')) {
        shell.exec(`cd ${buildDir}/..; make clean; make -j8 ${mkArgs}`);
    }
}

const { devices, modules } = generateVersionsH({ packageJson, gitInfo, useRealData: true });
packageJson.devices = devices;
packageJson.modules = modules;

for (const sourcePath of sourcePaths) {
    const buildDir = path.dirname(`${__dirname}/../${sourcePath}`);
    if (!buildDir.includes('zephyr')) {
        shell.exec(`cd ${buildDir}/..; make -j8 ${mkArgs}`);
    }
}


shell.exec(`npm ci; npm run build`, { cwd: agentDir });

for (const device of packageJson.devices) {
    const deviceDir = `${releaseDir}/devices/${device.name}`;
    const deviceSource = `${__dirname}/../${device.source}`;
    const deviceMMap = `${__dirname}/../${device.mmap}`;
    shell.mkdir('-p', deviceDir);
    shell.chmod(644, deviceSource);
    shell.cp(deviceSource, `${deviceDir}/firmware.hex`);
    shell.cp(deviceMMap, `${deviceDir}/firmware.map`);
    shell.exec(`npm run convert-user-config-to-bin -- ${deviceDir}/config.bin`, { cwd: agentDir });
}

for (const module of packageJson.modules) {
    const moduleDir = `${releaseDir}/modules`;
    const moduleSource = `${__dirname}/../${module.source}`;
    const moduleMMap = `${__dirname}/../${module.mmap}`;
    shell.mkdir('-p', moduleDir);
    shell.chmod(644, moduleSource);
    shell.cp(moduleSource, `${moduleDir}/${module.name}.bin`);
    shell.cp(moduleMMap, `${moduleDir}/${module.name}.map`);
}

const updatedPackage = Object.assign({}, packageJson, { gitInfo: gitInfo });
fs.writeFileSync(`${releaseDir}/package.json`, JSON.stringify(updatedPackage, null, 2));

shell.cp('-RL', `${__dirname}/../doc/dist`, `${releaseDir}/doc`);
shell.mkdir('-p', `${releaseDir}/doc-dev`);
shell.cp('-RL', `${__dirname}/../doc-dev/reference-manual.md`, `${releaseDir}/doc-dev/reference-manual.md`);
shell.exec(`tar -czvf ${releaseFile} -C ${releaseDir} .`);
