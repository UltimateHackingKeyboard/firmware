# SPDX-License-Identifier: MIT
from west.commands import WestCommand
from west import log
import git
from west.manifest import Manifest
from pathlib import Path
from textwrap import dedent

# https://github.com/zephyrproject-rtos/zephyr/issues/71137#issuecomment-2042447526
class WestPatch(WestCommand):
    def __init__(self):
        super().__init__(
            'patch',
            'manage patches across multiple repositories',
            dedent('''
            This command manages patches across multiple repositories.
            It can apply, list, show diffs, and commit patches.
            Reverting patches is done by "west update" command.
            ''')
        )
        self.manifest = Manifest.from_file()

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description)
        parser.add_argument('--list', action='store_true',
                            help='list all patches')
        parser.add_argument('--diff', action='store_true',
                            help='show current uncommitted changes')
        parser.add_argument('--commit', action='store_true',
                            help='commit changes and create a new patch file')
        parser.add_argument('--stash', action='store_true',
                            help='stash uncommitted changes')
        return parser

    def do_run(self, args, unknown_args):
        if args.list:
            self.list_patches()
        elif args.diff:
            self.show_diff()
        elif args.commit:
            self.commit_patches()
        elif args.stash:
            self.stash_changes()
        else:
            self.apply_patches()

    def get_git_projects(self):
        projects = []
        for project in self.manifest.projects:
            if project.name == 'manifest':
                continue # Skip the manifest project itself
            try:
                project_repo = git.Repo(project.abspath)
            except git.exc.NoSuchPathError as e:
                continue # Skip projects that aren't dependencies
            projects.append(project)
        return projects

    def get_patch_dir(self, project):
        return Path(self.manifest.repo_abspath) / 'patches' / project.name

    def apply_patches(self):
        for project in self.get_git_projects():
            patch_dir = self.get_patch_dir(project)
            if patch_dir.exists():
                for patch_file in sorted(patch_dir.glob('*.patch')):
                    try:
                        project_repo = git.Repo(project.abspath)
                        project_repo.git.am(patch_file.as_posix())
                        log.inf(f'Applied {patch_file.name} to {project.name}')
                    except git.exc.GitCommandError as e:
                        project_repo.git.am('--abort')
                        log.die(f'Error applying {patch_file.name} to {project.name}: {e}')

    def list_patches(self):
        for project in self.get_git_projects():
            patch_dir = self.get_patch_dir(project)
            if patch_dir.exists():
                patches = sorted(patch_file.name for patch_file in patch_dir.glob('*.patch'))
                if patches:
                    log.inf(f'{project.name} has patches:')
                    for patch in patches:
                        log.inf(f'  - {patch}')
                else:
                    log.inf(f'{project.name} has no patches.')

    def show_diff(self):
        for project in self.get_git_projects():
            project_repo = git.Repo(project.abspath)
            diff = project_repo.git.diff()
            if diff:
                log.inf(f'Uncommitted changes in {project.name}:')
                log.inf(diff)
            else:
                log.inf(f'No uncommitted changes in {project.name}')

    def commit_patches(self):
        for project in self.get_git_projects():
            project_repo = git.Repo(project.abspath)
            if project_repo.is_dirty(untracked_files=True):
                patch_dir = self.get_patch_dir(project)
                patch_dir.mkdir(exist_ok=True)
                commit_message = input(f'Enter commit message for {project.name}: ')
                project_repo.git.add(A=True)
                project_repo.git.commit('-m', commit_message)
                ret = project_repo.git.format_patch('-1', '-o', patch_dir.as_posix())
                log.inf(f'Committed changes and created patch {ret} for {project.name}')
            else:
                log.inf(f'No changes to commit for {project.name}')

    def stash_changes(self):
        for project in self.get_git_projects():
            project_repo = git.Repo(project.abspath)
            if project_repo.is_dirty(untracked_files=True):
                log.inf(f'Stashing changes for {project.name}')
                project_repo.git.stash()
