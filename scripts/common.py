import subprocess
import os
import json

def exec(cmd):
    return subprocess.check_output(cmd, shell=True).decode('utf-8').strip()

def get_git_info():
    return {
        'repo': exec('git remote get-url origin').replace('https://github.com/', '').replace('git@github.com:', '').replace('.git', ''),
        'tag': exec('git tag --points-at HEAD') or exec('git rev-parse --short HEAD'),
        'root': exec('git rev-parse --show-toplevel')
    }

def read_package_json():
    with open(os.path.join(os.path.dirname(__file__), 'package.json'), 'r') as f:
        return json.load(f)
