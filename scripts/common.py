import subprocess
import os
import json

def exec(cmd):
    return subprocess.check_output(cmd, shell=True).decode('utf-8').strip()

def is_git_repo():
    try:
        subprocess.check_output(['git', 'rev-parse', '--git-dir'], stderr=subprocess.DEVNULL)
        return True
    except subprocess.CalledProcessError:
        return False

def get_git_info():
    if not is_git_repo():
        return {
            'repo': 'unknown/unknown',
            'tag': 'unknown',
            'root': os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        }
    return {
        'repo': exec('git remote get-url origin').replace('https://github.com/', '').replace('git@github.com:', '').replace('.git', ''),
        'tag': exec('git tag --points-at HEAD') or exec('git rev-parse --short HEAD'),
        'root': exec('git rev-parse --show-toplevel')
    }

def read_package_json(path=None):
    if path is None:
        path = os.path.join(os.path.dirname(__file__), 'package.json')
    with open(path, 'r') as f:
        return json.load(f)
