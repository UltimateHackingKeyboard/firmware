from west.commands import WestCommand
from west import log
from textwrap import dedent
from pathlib import Path
import subprocess
import sys

class WestAgent(WestCommand):
    def __init__(self):
        super().__init__(
            'agent',
            'perform UHK device firmware update over USB',
            dedent('''
            This command finds the device binary file in the given build directory
            and perform UHK device firmware update over USB.
            ''')
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description)
        parser.add_argument(
            '-d', '--build-dir', required=True, metavar='DIR',
            help='Build directory of the device firmware (must be in the structure {app}/build/{preset})')
        return parser

    def find_single_file(self, directory: Path, extension: str) -> Path:
        files = list(directory.glob(f'*{extension}'))
        if len(files) == 0:
            log.die(f'No {extension} file found in {directory}')
        if len(files) > 1:
            log.die(f'More than one {extension} file found in {directory}: {[f.name for f in files]}')
        return files[0]

    def do_run(self, args, unknown_args):
        build_dir = Path(args.build_dir)
        if not build_dir.is_dir():
            log.die(f'Build directory does not exist: {build_dir}')

        # The application name is always two levels above the build dir
        try:
            app_name = build_dir.parents[1].name
        except IndexError:
            log.die(f'Cannot determine application name from build directory: {build_dir}')

        pid = None
        module = None
        if app_name == 'device':
            dotconfig = build_dir / 'device' / 'zephyr' / '.config'
            with dotconfig.open() as f:
                for line in f:
                    if line.startswith('CONFIG_USB_DEVICE_PID='):
                        pid = line[len('CONFIG_USB_DEVICE_PID='):].strip()
                        break
            if pid is None:
                log.die('CONFIG_USB_DEVICE_PID not found or invalid in .config')

            fw_file = build_dir / 'device' / 'zephyr' / 'zephyr.signed.bin'
            if not fw_file.is_file():
                log.die(f'Firmware file not found: {fw_file}')

        elif app_name == 'right':
            cmake_cache = build_dir / 'CMakeCache.txt'
            with cmake_cache.open() as f:
                for line in f:
                    if line.startswith('DEVICE_PID:'):
                        parts = line.strip().split('=')
                        if len(parts) == 2 and parts[1].isdigit():
                            pid = parts[1]
                            break
            if pid is None:
                log.die('DEVICE_PID not found or invalid in CMakeCache.txt')

            fw_file = self.find_single_file(build_dir, '.hex')
        else:
            module_lookup = {
                'keycluster': 'leftModule',
                'left': 'leftHalf',
                'trackball': 'rightModule',
                'trackpoint': 'rightModule',
            }
            if app_name not in module_lookup:
                log.die(f'Unknown module: {app_name}')
            module = module_lookup[app_name]
            fw_file = self.find_single_file(build_dir, '.bin')

        script_dir = Path(__file__).parent.resolve()
        tsx_path = (script_dir / '../lib/agent/node_modules/.bin/tsx').resolve()
        if pid:
            updater_path = (script_dir / '../lib/agent/packages/usb/update-device-firmware.ts').resolve()
            cmd = [
                str(tsx_path),
                str(updater_path),
                '--vid=14248',
                f'--pid={pid}',
                str(fw_file)
            ]
        else:
            updater_path = (script_dir / '../lib/agent/packages/usb/update-module-firmware.ts').resolve()
            cmd = [
                str(tsx_path),
                str(updater_path),
                module,
                str(fw_file)
            ]

        # Log the command and run it
        log.inf(f'Running: {" ".join(cmd)}')
        try:
            subprocess.run(cmd, check=True)
        except FileNotFoundError as e:
            log.die(f'Agent not built or file not found: {e.filename}')
        except subprocess.CalledProcessError as e:
            log.die(f'Device update failed: {e}')
