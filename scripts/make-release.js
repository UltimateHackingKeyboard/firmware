#!/usr/bin/env node
const fs = require('fs');
require('shelljs/global');

config.fatal = true;

const version = JSON.parse(fs.readFileSync(`${__dirname}/package.json`)).version;
const releaseName = `uhk-firmware-${version}`;
const releaseDir = `${__dirname}/${releaseName}`;
const releaseFile = `${__dirname}/${releaseName}.tar.bz2`;
const rightFirmwareFile = `${__dirname}/../right/build/uhk60-right_release/uhk-right.hex`;
const leftFirmwareFile = `${__dirname}/../left/build/uhk60-left_release/uhk-left.bin`;

rm('-rf', releaseDir, releaseFile, leftFirmwareFile, rightFirmwareFile);

exec(`/opt/Freescale/KDS_v3/eclipse/kinetis-design-studio \
--launcher.suppressErrors \
-noSplash \
-application org.eclipse.cdt.managedbuilder.core.headlessbuild \
-import ${__dirname}/../left/build \
-import ${__dirname}/../right/build \
-cleanBuild uhk-left \
-cleanBuild uhk-right`);

chmod(644, rightFirmwareFile, leftFirmwareFile);
mkdir(releaseDir);
cp(rightFirmwareFile, releaseDir);
cp(leftFirmwareFile, releaseDir);
cp(`${__dirname}/package.json`, releaseDir);
exec(`tar -cvjSf ${releaseFile} -C ${releaseDir} .`);
