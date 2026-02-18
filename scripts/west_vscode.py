# SPDX-License-Identifier: MIT
from sys import path
from west.commands import WestCommand
from west import log
from west.util import west_topdir
from west.configuration import config
from pathlib import Path
from textwrap import dedent
import json
import yaml

def config_get(option, fallback):
    return config.get('vscode', option, fallback=fallback)

def config_getboolean(option, fallback):
    return config.getboolean('vscode', option, fallback=fallback)

def vscode_dir(app_dir):
    # if the vscode/dir west configuration option is set, use that
    # otherwise, use the application directory
    vscode_dir_opt = config_get('workspace', None)
    if vscode_dir_opt:
        return Path(west_topdir()) / vscode_dir_opt / '.vscode'
    return Path(west_topdir()) / app_dir / '.vscode'

def read_build_info(build_dir, key_list_to_read):
    build_info_yaml_path = Path(build_dir) / 'build_info.yml'
    if not build_info_yaml_path.exists():
        log.die(f"Error: build_info.yml not found in {build_dir}.")
    with open(build_info_yaml_path, 'r') as f:
        build_info_yaml = yaml.safe_load(f)
        # use the keys in sequence to get to the nested value
        value = build_info_yaml
        for key in key_list_to_read:
            if key in value:
                value = value[key]
            else:
                return None
        return value

class WestVsCode(WestCommand):
    def __init__(self):
        super().__init__(
            'vscode',
            'generate VSCode configuration files',
            dedent('''
            This command generates VSCode configuration files for given builds,
            for code indexing, building and debugging.
            ''')
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description)
        parser.add_argument(
            '-d', '--build-dir', required=True, metavar='DIR',
            help='Build directory of the device firmware')
        parser.add_argument(
            '--build', metavar='BUILD_NAME',
            help='Name of the build configuration to generate task for')
        parser.add_argument(
            '--debug', metavar='DEBUG_NAME',
            help='Name of the debug configuration to generate launch for')
        parser.add_argument(
            '-r', '--runner', metavar='RUNNER',
            help='Override default launch runner from --build-dir')
        return parser

    def do_run(self, args, unknown_args):
        root_build_dir = Path(args.build_dir)
        build_dir = root_build_dir

        # if sysbuild is used, find application's build directory
        domains_yaml_path = root_build_dir / "domains.yaml"
        is_sysbuild = domains_yaml_path.exists()
        if is_sysbuild:
            with open(domains_yaml_path, 'r') as f:
                domains_yaml = yaml.safe_load(f)
                default_domain = None
                for domain in domains_yaml['domains']:
                    if domain['name'] == domains_yaml['default']:
                        default_domain = domain
                        break
                if not default_domain:
                    log.die(f"Could not find default domain {domains_yaml['default']} in {domains_yaml_path}.")
                build_dir = Path(default_domain['build_dir'])

        # prepare the output directory
        app_dir = read_build_info(build_dir, ['cmake', 'application', 'source-dir'])
        if not app_dir:
            log.die(f"Could not read application source directory from build_info.yml in {build_dir}.")
        vscode_dotdir = vscode_dir(app_dir)
        vscode_dotdir.mkdir(parents=True, exist_ok=True)
        ws_build_dir = (Path.cwd() / Path(build_dir)).resolve().relative_to(vscode_dotdir.parent)

        # generate c_cpp_properties.json
        compile_commands_path = Path(build_dir) / 'compile_commands.json'
        if not compile_commands_path.exists():
            log.wrn(f"Warning: compile_commands.json not found in {build_dir}, skipping c_cpp_properties.json generation.")
        else:
            c_cpp_properties_json_path = vscode_dotdir / 'c_cpp_properties.json'
            c_cpp_json = {
                "version": 4,
                "configurations": [
                    {
                    "name": "IntelliSense Configuration",
                    "compileCommands": [
                        f"${{workspaceFolder}}/{ws_build_dir}/compile_commands.json"
                    ],
                    "intelliSenseMode": "gcc-arm"
                    }
                ]
            }
            with open(c_cpp_properties_json_path, 'w') as f:
                json.dump(c_cpp_json, f, indent=4)

        if args.build:
            # generate tasks.json entry for the given build
            build_name = args.build
            west_cmd = read_build_info(root_build_dir, ['west', 'command'])
            if not west_cmd:
                log.die(f"Could not read west command from build_info.yml in {root_build_dir}.")
            west_cmd_args = west_cmd.split()[1:]

            tasks_json_path = vscode_dotdir / 'tasks.json'
            if not tasks_json_path.exists():
                tasks_json = {
                    "version": "2.0.0",
                    "tasks": []
                }
            else:
                with open(tasks_json_path, 'r') as f:
                    tasks_json = json.load(f)
                    if not tasks_json or "tasks" not in tasks_json:
                        tasks_json = {
                            "version": "2.0.0",
                            "tasks": []
                        }

            new_task = {
                "label": f"Build {build_name}",
                "group": {
                    "kind": "build",
                    "isDefault": True
                },
                "options": {
                    "cwd": "${workspaceFolder}"
                },
                "type": "process",
                "command": "west",
                "args": west_cmd_args,
                "problemMatcher": [
                    "$gcc"
                ]
            }

            # Replace if label matches, else append
            replaced = False
            for idx, task in enumerate(tasks_json["tasks"]):
                if task.get("label") == new_task["label"]:
                    tasks_json["tasks"][idx] = new_task
                    replaced = True
                    break
            if not replaced:
                tasks_json["tasks"].append(new_task)

            with open(tasks_json_path, 'w') as f:
                json.dump(tasks_json, f, indent=4)

        if args.debug:
            # generate launch.json entry for the given debug configuration
            debug_name = args.debug
            runners_yaml_path = build_dir / 'zephyr' / 'runners.yaml'
            if not runners_yaml_path.exists():
                log.die(f"zephyr/runners.yaml not found in {build_dir}.")
            with open(runners_yaml_path, 'r') as f:
                runners_yaml = yaml.safe_load(f)

                if args.runner and args.runner in runners_yaml['runners']:
                    servertype = args.runner
                else:
                    servertype = runners_yaml['debug-runner']

                serverArgs = runners_yaml['args'][servertype]
                gdbPath = runners_yaml['config']['gdb']
                executable = runners_yaml['config']['elf_file']
                # if the executable path is relative, make it absolute
                if not Path(executable).is_absolute():
                    executable = str(ws_build_dir / 'zephyr' / executable)

            launch_json_path = vscode_dotdir / 'launch.json'
            if not launch_json_path.exists():
                launch_json = {
                    "version": "0.2.0",
                    "configurations": []
                }
            else:
                with open(launch_json_path, 'r') as f:
                    launch_json = json.load(f)
                    if not launch_json or "configurations" not in launch_json:
                        launch_json = {
                            "version": "0.2.0",
                            "configurations": []
                        }

            new_launch = {
                "name": f"Launch {debug_name}",
                "cwd": "${workspaceFolder}",
                "executable": executable,
                "request": "launch",
                "type": "cortex-debug",
                "runToEntryPoint": "main",
                "servertype": servertype,
                "serverArgs": serverArgs,
                "gdbPath": gdbPath,
                "rtos": "/opt/SEGGER/JLink/GDBServer/RTOSPlugin_Zephyr.so"
            }
            # workaround for jlink: add separate device argument
            for arg in serverArgs:
                if arg.startswith('--device='):
                    new_launch['device'] = arg.split('=')[1]
                    break

            # in some extensions, svd path is provided
            svdPath = read_build_info(build_dir, ['cmake', 'vendor-specific', 'nordic', 'svdfile'])
            if svdPath:
                new_launch['svdPath'] = svdPath

            # Replace if name matches, else append
            replaced = False
            for idx, config in enumerate(launch_json["configurations"]):
                if config.get("name") == new_launch["name"]:
                    launch_json["configurations"][idx] = new_launch
                    replaced = True
                    break
            if not replaced:
                launch_json["configurations"].append(new_launch)

            with open(launch_json_path, 'w') as f:
                json.dump(launch_json, f, indent=4)
        log.inf(f"VSCode configuration files generated in {vscode_dotdir}.")
