#!/usr/bin/env node

function getGitInfo() {
    var result = {
        repo: '',
        tag: '',
    };

    const execSync = require('child_process').execSync

    result.repo = execSync('git remote get-url origin').toString().trim()
        .replace(/.*github.com./g, '')
        .replace(/.git$/, '');

    result.tag = execSync('git tag --points-at HEAD').toString().trim();

    if (result.tag.length === 0) {
        result.tag = execSync('git rev-parse --short HEAD').toString().trim();
    }

    return result;
}

exports.getGitInfo = getGitInfo;

