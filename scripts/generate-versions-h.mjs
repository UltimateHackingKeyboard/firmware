#!/usr/bin/env node
import {getGitInfo, readPackageJson} from './common.mjs';
import {generateVersionsH} from './generate-versions-h-util.mjs';

const useRealData = process.argv.includes('--withMd5Sums')
const useZeroVersions = process.argv.includes('--withZeroVersions')
const packageJson = readPackageJson()

const gitInfo = getGitInfo();

generateVersionsH({packageJson, gitInfo, useRealData, useZeroVersions});
