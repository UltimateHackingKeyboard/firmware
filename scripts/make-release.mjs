#!/usr/bin/env node
import fs from 'fs';
import path from 'path';
import shell from 'shelljs';
import {getGitInfo, readPackageJson} from './common.mjs';
import {generateChecksums} from './generate-checksums.mjs';
import {fileURLToPath} from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

shell.config.fatal = true;
shell.config.verbose = true;

const gitInfo = getGitInfo();
const packageJson = readPackageJson();

function build(buildTarget, step) {
    const buildDir = path.dirname(`${__dirname}/../${buildTarget.source}`);
    const projectName = buildTarget.source.split(path.sep)[0];
    const buildDirName = path.basename(buildDir);
    if (step === 1) {
        shell.rm('-rf', buildDir);
        shell.mkdir('-p', buildDir);
    }
    const pristineFlag = (step === 1) ? '--pristine' : '';
    const checksumsMode = (step === 1) ? '-DUHK_USE_CHECKSUMS=FALSE' : '-DUHK_USE_CHECKSUMS=TRUE';

    if (buildTarget.platform === 'kinetis') {
        shell.exec(`west config manifest.file west_mcuxsdk.yml && \
            west build --force --build-dir ${projectName}/build/${buildDirName} ${projectName} \
            ${pristineFlag} \
            -- \
            --preset ${buildDirName} \
            ${checksumsMode}`
        );
    } else if (buildTarget.platform === 'nordic') {
        shell.exec(`ZEPHYR_TOOLCHAIN_VARIANT=zephyr west config manifest.file west.yml && \
            west build --build-dir ${projectName}/build/${buildTarget.name} ${projectName} \
            --no-sysbuild \
            ${pristineFlag} \
            -- \
            --preset ${buildTarget.name} \
            ${checksumsMode}`
        );
    }
}

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

const {devices, modules} = generateChecksums({packageJson});
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

    if (device.userConfigType) {
        shell.exec(`npm run convert-user-config-to-bin -- ${device.userConfigType} ${deviceDir}/config.bin`, { cwd: agentDir });
    }

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
