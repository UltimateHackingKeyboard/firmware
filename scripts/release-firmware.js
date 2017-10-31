#!/usr/bin/env node
const fs = require('fs');
require('shelljs/global');

const version = JSON.parse(fs.readFileSync('package.json')).version;
const releaseName = `uhk-firmware-${version}`;
const releaseDir = `/tmp/${releaseName}`;
const rightFirmwareFile = '../right/build/uhk60-right_release/uhk-right.hex';
const leftFirmwareFile = '../left/build/uhk60-left_release/uhk-left.bin';

exec(`/opt/Freescale/KDS_v3/eclipse/kinetis-design-studio \
--launcher.suppressErrors \
-noSplash \
-application org.eclipse.cdt.managedbuilder.core.headlessbuild \
-import ../left/build \
-import ../right/build \
-cleanBuild uhk-left \
-cleanBuild uhk-right`);

chmod(644, rightFirmwareFile, leftFirmwareFile);
ls('-l', rightFirmwareFile, leftFirmwareFile);
rm('-r', releaseDir);
mkdir(releaseDir);
cp(rightFirmwareFile, releaseDir);
cp(leftFirmwareFile, releaseDir);
cp('package.json', releaseDir);
exec(`tar -cvjSf ${releaseName}.tar.bz2 -C ${releaseDir} .`);
