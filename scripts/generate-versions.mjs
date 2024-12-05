#!/usr/bin/env node
import {getGitInfo, readPackageJson} from './common.mjs';
import {generateVersions} from './generate-versions-util.mjs';

const useRealData = process.argv.includes('--withMd5Sums')
const useZeroVersions = process.argv.includes('--withZeroVersions')
const packageJson = readPackageJson()

const gitInfo = getGitInfo();

generateVersions({packageJson, gitInfo, useRealData, useZeroVersions});
