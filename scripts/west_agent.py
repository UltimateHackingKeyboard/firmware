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
            'perform UHK module firmware update over right half USB',
            dedent('''
            This command finds the module binary file in the given build directory
            and perform UHK module firmware update over USB.
            ''')
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description)
        parser.add_argument(
            '-d', '--build-dir', required=True, metavar='DIR',
            help='Directory containing exactly UHK module binary file')
        return parser

    def do_run(self, args, unknown_args):
        build_dir = Path(args.build_dir)
        if not build_dir.is_dir():
            log.die(f'Build directory does not exist: {build_dir}')

        elf_files = list(build_dir.glob('*.elf'))
        if len(elf_files) == 0:
            log.die(f'No .elf file found in {build_dir}')
        if len(elf_files) > 1:
            log.die(f'More than one .elf file found in {build_dir}: {[f.name for f in elf_files]}')

        elf_file = elf_files[0]
        elf_name = elf_file.stem

        script_dir = Path(__file__).parent.resolve()
        tsx_path = (script_dir / '../lib/agent/node_modules/.bin/tsx').resolve()
        if elf_name == 'uhk-60-right':
            updater_path = (script_dir / '../lib/agent/packages/usb/update-device-firmware.ts').resolve()

            # For uhk-60-right, use .hex file
            hex_file = elf_file.with_suffix('.hex')
            if not hex_file.is_file():
                log.die(f'.hex file not found for {elf_file.name}')

            # Find the DEVICE_PID from CMakeCache.txt
            cmake_cache = build_dir / 'CMakeCache.txt'
            if not cmake_cache.is_file():
                log.die(f'CMakeCache.txt not found in {build_dir}')
            pid = None
            with cmake_cache.open() as f:
                for line in f:
                    if line.startswith('DEVICE_PID:'):
                        parts = line.strip().split('=')
                        if len(parts) == 2 and parts[1].isdigit():
                            pid = parts[1]
                            break
            if pid is None:
                log.die('DEVICE_PID not found or invalid in CMakeCache.txt')

            cmd = [
                str(tsx_path),
                str(updater_path),
                '--vid=14248',
                f'--pid={pid}',
                str(hex_file)
            ]
        else:
            # Figure out the module name for the script
            module_lookup = {
                'uhk-keycluster': 'leftModule',
                'uhk-60-left': 'leftHalf',
                'uhk-trackball': 'rightModule',
                'uhk-trackpoint': 'rightModule',
            }
            if elf_name not in module_lookup:
                log.die(f'Unknown module: {elf_name}')
            module = module_lookup[elf_name]

            # For modules, use .bin file
            bin_file = elf_file.with_suffix('.bin')
            if not bin_file.is_file():
                log.die(f'.bin file not found for {elf_file.name}')

            updater_path = (script_dir / '../lib/agent/packages/usb/update-module-firmware.ts').resolve()
            cmd = [
                str(tsx_path),
                str(updater_path),
                module,
                str(bin_file)
            ]

        # Log the command and run it
        log.inf(f'Running: {" ".join(cmd)}')
        try:
            subprocess.run(cmd, check=True)
        except subprocess.CalledProcessError as e:
            log.die(f'Module update failed: {e}')
