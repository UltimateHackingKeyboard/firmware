import {execSync} from 'child_process';
import {readFileSync} from 'fs';

import path from 'path';
import {fileURLToPath} from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

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

export function readPackageJson() {
  return JSON.parse(readFileSync(path.join(__dirname, 'package.json'), 'utf8'));
}
