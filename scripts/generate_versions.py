#!/usr/bin/env python3
import sys
from common import get_git_info, read_package_json
from generate_versions_utils import generate_versions

if __name__ == "__main__":
    use_real_data = '--withMd5Sums' in sys.argv
    use_zero_versions = '--withZeroVersions' in sys.argv
    package_json = read_package_json()
    git_info = get_git_info()
    generate_versions(package_json, git_info, use_real_data, use_zero_versions)
