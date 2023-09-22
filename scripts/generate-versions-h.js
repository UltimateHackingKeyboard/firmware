#!/usr/bin/env node
const common = require('./common.js');
const generateVersionsH = require('./generate-versions-h-util.js');
const readPackageJson = require('./read-package-json.js');

const useRealData = process.argv.includes('--withMd5Sums')
const packageJson = readPackageJson()

const gitInfo = common.getGitInfo();

generateVersionsH({ packageJson, gitInfo, useRealData});
