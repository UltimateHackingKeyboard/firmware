#!/usr/bin/env python3
"""Compile the production send function with fault-injected link/Zephyr calls.

Run with Python 3 and a host C compiler. --source accepts an older uart_bridge.c
to check that the same assertions fail without the timeout fix.
"""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


root = Path(__file__).resolve().parents[2]
args = argparse.ArgumentParser(description=__doc__)
args.add_argument("--source", type=Path, default=root / "device/src/keyboard/uart_bridge.c")
options = args.parse_args()
source = options.source.read_text()
start = source.index("int UartBridge_SendMessage(")
opening = source.index("{", start)
depth = 1
end = opening + 1
while depth:
    depth += (source[end] == "{") - (source[end] == "}")
    end += 1
timeout = re.search(r"^#define UART_FOREVER_TIMEOUT\s+\d+$", source, re.MULTILINE).group()

with tempfile.TemporaryDirectory(prefix="uart-send-timeout-") as work:
    work = Path(work)
    (work / "send_under_test.inc").write_text(timeout + "\n" + source[start:end] + "\n")
    binary = work / "test"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-g", "-I", str(work),
        str(Path(__file__).with_suffix(".c")), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
