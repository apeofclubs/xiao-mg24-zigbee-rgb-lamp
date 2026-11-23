#!/usr/bin/env python3
"""Refuse a firmware image that would write outside the application area.

    scripts/check_image.py [image.hex]

Default image: build/gen/build/debug/xiao_mg24_rgb_lamp.hex

The XIAO MG24 (EFR32MG24B220F1536IM48) flash is laid out as (docs/flashing.md):

    0x08000000-0x08005FFF  bootloader            never written
    0x08006000-0x08175FFF  application           the only area an image may use
    0x08176000-0x0817DFFF  NVM3 (Zigbee keys, settings)  never written
    0x0817E000-0x0817FFFF  end of flash          never written

The check fails (exit 1) if any byte of the Intel HEX lies outside the application area, or if the vector table
(initial stack pointer and reset vector) is missing at 0x08006000. It prints the contiguous segments of the image.
Python standard library only.
"""
import sys
from pathlib import Path

APP_START = 0x08006000
APP_END = 0x08176000  # exclusive: NVM3 starts here
FORBIDDEN = (
    (0x08000000, 0x08006000, "bootloader"),
    (0x08176000, 0x0817E000, "NVM3"),
    (0x0817E000, 0x08180000, "end of flash"),
)
DEFAULT = Path(__file__).resolve().parent.parent / "build/gen/build/debug/xiao_mg24_rgb_lamp.hex"


def parse_hex(path):
    """Return {address: byte} for every data byte of an Intel HEX file."""
    data = {}
    base = 0
    eof = False
    for lineno, raw in enumerate(path.read_text(encoding="ascii").splitlines(), 1):
        line = raw.strip()
        if not line:
            continue
        if eof:
            raise ValueError(f"line {lineno}: data after the end-of-file record")
        if not line.startswith(":"):
            raise ValueError(f"line {lineno}: not an Intel HEX record")
        rec = bytes.fromhex(line[1:])
        if len(rec) < 5 or len(rec) != rec[0] + 5:
            raise ValueError(f"line {lineno}: bad record length")
        if sum(rec) & 0xFF:
            raise ValueError(f"line {lineno}: bad checksum")
        count, offset, rtype, payload = rec[0], (rec[1] << 8) | rec[2], rec[3], rec[4:-1]
        if rtype == 0x00:
            for i, b in enumerate(payload):
                addr = base + offset + i
                if addr in data and data[addr] != b:
                    raise ValueError(f"line {lineno}: conflicting data at 0x{addr:08X}")
                data[addr] = b
        elif rtype == 0x01:
            eof = True
        elif rtype == 0x02 and count == 2:
            base = ((payload[0] << 8) | payload[1]) << 4
        elif rtype == 0x04 and count == 2:
            base = ((payload[0] << 8) | payload[1]) << 16
        elif rtype in (0x03, 0x05) and count == 4:
            pass  # start address: not written to flash
        else:
            raise ValueError(f"line {lineno}: unsupported record type 0x{rtype:02X}")
    if not eof:
        raise ValueError("no end-of-file record (truncated image?)")
    return data


def segments(addresses):
    """Contiguous [start, end) ranges of a sorted address list."""
    out = []
    for a in addresses:
        if out and a == out[-1][1]:
            out[-1][1] = a + 1
        else:
            out.append([a, a + 1])
    return out


def word(data, addr):
    if not all(addr + i in data for i in range(4)):
        return None
    return int.from_bytes(bytes(data[addr + i] for i in range(4)), "little")


def main():
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT
    try:
        data = parse_hex(path)
    except (OSError, ValueError) as e:
        print(f"FAIL {path}: {e}")
        return 1
    if not data:
        print(f"FAIL {path}: image is empty")
        return 1

    errors = []
    print(f"{path}: {len(data)} bytes")
    for start, end in segments(sorted(data)):
        inside = APP_START <= start and end <= APP_END
        print(f"  0x{start:08X}-0x{end:08X}  {end - start:7d} B  {'ok' if inside else 'OUTSIDE the application area'}")
        if not inside:
            areas = [name for lo, hi, name in FORBIDDEN if start < hi and lo < end] or ["outside flash"]
            errors.append(f"0x{start:08X}-0x{end:08X} writes {', '.join(areas)}")

    sp, reset = word(data, APP_START), word(data, APP_START + 4)
    if sp is None or reset is None:
        errors.append(f"no vector table at 0x{APP_START:08X}")
    else:
        print(f"  vector table at 0x{APP_START:08X}: SP 0x{sp:08X}, reset 0x{reset:08X}")
        if not 0x20000000 <= sp <= 0x20040000:
            errors.append(f"initial SP 0x{sp:08X} is not in RAM")
        if not (reset & 1 and APP_START <= (reset & ~1) < APP_END):
            errors.append(f"reset vector 0x{reset:08X} is not a Thumb address in the application area")

    if errors:
        for e in errors:
            print(f"FAIL {e}")
        return 1
    print(f"PASS image stays inside 0x{APP_START:08X}-0x{APP_END - 1:08X}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
