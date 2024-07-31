#!/usr/bin/env node
import fs from 'fs';
import path from 'path';
import shell from 'shelljs';
import {getGitInfo, readPackageJson} from './common.mjs';
import {generateVersions} from './generate-versions-util.mjs';
import {fileURLToPath} from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

shell.config.fatal = true;
shell.config.verbose = true;

const gitInfo = getGitInfo();
const packageJson = readPackageJson();

function build(buildTarget, step) {
    const buildDir = path.dirname(`${__dirname}/../${buildTarget.source}`);
    if (step === 1) {
        shell.rm('-rf', buildDir);
        shell.mkdir('-p', buildDir);
    }

    if (buildTarget.platform === 'kinetis') {
        if (step === 1) {
            shell.exec(`cd ${buildDir}/..; make clean; make -j8`);
        } else if (step === 2) {
            shell.exec(`cd ${buildDir}/..; make -j8`);
        }
    } else if (buildTarget.platform === 'nordic') {
        shell.exec(`ZEPHYR_TOOLCHAIN_VARIANT=zephyr west build \
            --build-dir ${gitInfo.root}/device/build/${buildTarget.name} \
            ${gitInfo.root}/device \
            --pristine \
            --board ${buildTarget.name} \
            --no-sysbuild \
            -- \
            -DNCS_TOOLCHAIN_VERSION=NONE \
            -DEXTRA_CONF_FILE=prj.conf.overlays/${buildTarget.name}.prj.conf \
            -DBOARD_ROOT=${gitInfo.root} \
            -Dmcuboot_OVERLAY_CONFIG="${gitInfo.root}/device/child_image/mcuboot.conf;${gitInfo.root}/device/child_image/${buildTarget.name}.mcuboot.conf"`
        );
    }
}

generateVersions({packageJson, gitInfo, useRealShas:false, useZeroVersions:true});

const version = packageJson.firmwareVersion;
const releaseName = `uhk-firmware-${version}`;
const releaseDir = `${__dirname}/${releaseName}`;
const agentDir = `${__dirname}/../lib/agent`;
let releaseFile = `${__dirname}/${releaseName}.tar.gz`;

if (gitInfo.tag !== `v${version}` && !process.argv.includes('--allowSha') && !process.argv.includes('--buildTest')) {
    console.error(`Git tag '${gitInfo.tag}' !~ 'v{version}'. Please run with '--allowSha' if this is intentional.`);
    process.exit(1);
}

if (gitInfo.tag !== `v${version}`) {
    releaseFile = `${__dirname}/${releaseName}-${gitInfo.tag}.tar.gz`;
}

shell.rm('-rf', releaseDir, releaseFile);

const buildTargets = [...packageJson.devices, ...packageJson.modules];
for (const buildTarget of buildTargets) {
    build(buildTarget, 1);
}

if (process.argv.includes('--buildTest')) {
    process.exit(0);
}

const {devices, modules} = generateVersions({packageJson, gitInfo, useRealShas:true, useZeroVersions:false});
packageJson.devices = devices;
packageJson.modules = modules;
for (const buildTarget of buildTargets) {
    build(buildTarget, 2);
}

shell.exec(`npm ci; npm run build`, {cwd: agentDir});

for (const device of packageJson.devices) {
    const deviceDir = `${releaseDir}/devices/${device.name}`;
    const deviceSource = `${__dirname}/../${device.source}`;
    const deviceMMap = `${__dirname}/../${device.mmap}`;
    shell.mkdir('-p', deviceDir);
    shell.chmod(644, deviceSource);
    shell.cp(deviceSource, `${deviceDir}/firmware${path.extname(device.source)}`);
    shell.cp(deviceMMap, `${deviceDir}/firmware.map`);
    shell.exec(`npm run convert-user-config-to-bin -- ${deviceDir}/config.bin`, { cwd: agentDir });

    if (device.kboot) {
      const kbootPath = path.join(__dirname, '..', device.kboot);
      shell.cp(kbootPath, path.join(deviceDir, 'kboot.hex'))
    }

    if (device.merged) {
      const kbootPath = path.join(__dirname, '..', device.merged);
      shell.cp(kbootPath, path.join(deviceDir, 'merged.hex'))
    }
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

// Restore development values
generateVersions({packageJson, gitInfo, useRealShas:false, useZeroVersions:false});
