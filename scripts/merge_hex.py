#!/usr/bin/env python3

import argparse
from pathlib import Path


def checksum(record_bytes):
    return ((-sum(record_bytes)) & 0xFF)


def parse_hex(path: Path):
    memory = {}
    base_address = 0

    with path.open("r", encoding="utf-8") as file:
        for line_number, raw_line in enumerate(file, start=1):
            line = raw_line.strip()
            if not line:
                continue
            if not line.startswith(":"):
                raise ValueError(f"{path}:{line_number}: Invalid Intel HEX line: {line}")

            hex_data = line[1:]
            if len(hex_data) < 10 or (len(hex_data) % 2) != 0:
                raise ValueError(f"{path}:{line_number}: Malformed Intel HEX record")

            record = bytes.fromhex(hex_data)
            length = record[0]
            address = (record[1] << 8) | record[2]
            record_type = record[3]
            data = record[4:4 + length]
            record_checksum = record[4 + length]

            if len(data) != length or (4 + length + 1) != len(record):
                raise ValueError(f"{path}:{line_number}: Record length mismatch")

            if checksum(record[:-1]) != record_checksum:
                raise ValueError(f"{path}:{line_number}: Record checksum mismatch")

            if record_type == 0x00:
                absolute = base_address + address
                for offset, value in enumerate(data):
                    memory[absolute + offset] = value
            elif record_type == 0x01:
                break
            elif record_type == 0x02:
                if length != 2:
                    raise ValueError(f"{path}:{line_number}: Invalid extended segment address record")
                segment = (data[0] << 8) | data[1]
                base_address = segment << 4
            elif record_type == 0x04:
                if length != 2:
                    raise ValueError(f"{path}:{line_number}: Invalid extended linear address record")
                upper = (data[0] << 8) | data[1]
                base_address = upper << 16
            else:
                # Ignore unsupported records for this merge use case.
                continue

    return memory


def emit_record(length, address, record_type, data_bytes):
    body = bytes([length, (address >> 8) & 0xFF, address & 0xFF, record_type]) + data_bytes
    return ":" + body.hex().upper() + f"{checksum(body):02X}"


def write_hex(path: Path, memory):
    addresses = sorted(memory.keys())

    with path.open("w", encoding="utf-8", newline="\n") as file:
        if addresses:
            current_upper = None
            run_start = addresses[0]
            run_data = [memory[run_start]]
            previous = run_start

            def flush_run(start_addr, data):
                nonlocal current_upper
                index = 0
                while index < len(data):
                    absolute = start_addr + index
                    upper = (absolute >> 16) & 0xFFFF
                    if current_upper != upper:
                        file.write(emit_record(2, 0, 0x04, bytes([(upper >> 8) & 0xFF, upper & 0xFF])) + "\n")
                        current_upper = upper

                    chunk_address = absolute & 0xFFFF
                    max_chunk = min(16, len(data) - index)

                    if chunk_address + max_chunk > 0x10000:
                        max_chunk = 0x10000 - chunk_address

                    chunk = bytes(data[index:index + max_chunk])
                    file.write(emit_record(len(chunk), chunk_address, 0x00, chunk) + "\n")
                    index += max_chunk

            for address in addresses[1:]:
                if address == previous + 1:
                    run_data.append(memory[address])
                else:
                    flush_run(run_start, run_data)
                    run_start = address
                    run_data = [memory[address]]
                previous = address

            flush_run(run_start, run_data)

        file.write(":00000001FF\n")


def merge_hex_files(first: Path, second: Path, output: Path, app_start_address: int):
    bootloader = parse_hex(first)
    application = parse_hex(second)

    merged = {}

    for address, value in bootloader.items():
        if address < app_start_address:
            merged[address] = value

    for address, value in application.items():
        if address >= app_start_address:
            merged[address] = value

    write_hex(output, merged)


def main():
    parser = argparse.ArgumentParser(description="Merge two Intel HEX files.")
    parser.add_argument("--first", required=True, type=Path, help="Base Intel HEX file")
    parser.add_argument("--second", required=True, type=Path, help="Overlay Intel HEX file")
    parser.add_argument("--out", required=True, type=Path, help="Merged output Intel HEX file")
    parser.add_argument(
        "--app-start-address",
        default="0xC000",
        help="First flash address owned by application HEX (default: 0xC000)",
    )
    args = parser.parse_args()

    merge_hex_files(args.first, args.second, args.out, int(args.app_start_address, 0))


if __name__ == "__main__":
    main()
