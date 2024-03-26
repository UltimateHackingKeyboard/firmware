import {execSync} from 'child_process';

export function getGitInfo() {
    const result = {
        repo: '',
        tag: '',
    };

    result.repo = execSync('git remote get-url origin').toString().trim()
        .replace(/.*github.com./g, '')
        .replace(/.git$/, '');

    result.tag = execSync('git tag --points-at HEAD').toString().trim();
    if (result.tag.length === 0) {
        result.tag = execSync('git rev-parse --short HEAD').toString().trim();
    }

    result.root = execSync('git rev-parse --show-toplevel').toString().trim();

    return result;
}
