#!/usr/bin/env node
import { getGitInfo } from './common.mjs';
import { generateVersionsH } from './generate-versions-h-util.mjs';
import { readPackageJson } from './read-package-json.mjs';

const useRealData = process.argv.includes('--withMd5Sums')
const packageJson = readPackageJson()

const gitInfo = getGitInfo();

generateVersionsH({packageJson, gitInfo, useRealData});
