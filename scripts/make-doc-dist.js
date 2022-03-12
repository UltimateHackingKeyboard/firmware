#!/usr/bin/env node
const path = require('path');
require('shelljs/global');

config.fatal = true;
config.verbose = true;

const docFiles = [
    'index.html',
    'style.css',
    'agent-onInit.png',
    'node_modules/bootstrap/dist/css/bootstrap.min.css',
];

for (const docFile of docFiles) {
    const src = `${__dirname}/../doc/${docFile}`;
    const dst = `${__dirname}/../doc/dist/${docFile}`;
    const dstDir = path.dirname(dst);
    mkdir('-p', dstDir);
    cp(src, dst);
}
