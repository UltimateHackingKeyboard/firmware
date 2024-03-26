import {execSync} from 'child_process';

function exec(cmd) {
    return execSync(cmd).toString().trim();
}

export function getGitInfo() {
    return {
        repo: exec('git remote get-url origin').replace(/.*github.com./g, '').replace(/.git$/, ''),
        tag: exec('git tag --points-at HEAD') || exec('git rev-parse --short HEAD'),
        root: exec('git rev-parse --show-toplevel'),
    };
}
