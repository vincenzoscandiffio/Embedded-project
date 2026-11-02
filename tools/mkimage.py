#!/usr/bin/env python3
"""Pack program files into an IMG1 image (assignment.md 6.1 and 6.3).

Big-version host tool: `image` only. Programs are ELF files, packed byte
for byte and unchanged -- this tool never parses or validates ELF content,
that is the kernel's job at load time (B1). It only enforces the IMG1
container rules: name/size limits, no duplicate names, 4-byte-aligned file
starts, and the 2 MiB image cap.

    python3 tools/mkimage.py image build/fs.img NAME=PROGRAM.elf [...]
"""

import argparse
import struct
import sys

MAGIC = b"IMG1"
VERSION = 1
HEADER_SIZE = 20
DIRECTORY_OFFSET = 20  # the packer always puts the directory here (6.1)
ENTRY_SIZE = 24
NAME_FIELD_SIZE = 16
MAX_NAME_LEN = 15
NAME_CHARS = frozenset(
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-"
)
MIN_FILE_SIZE = 40
MAX_FILES = 32
MAX_IMAGE_SIZE = 0x200000  # 2 MiB


def build_parser():
    parser = argparse.ArgumentParser(prog="mkimage.py")
    sub = parser.add_subparsers(dest="command", required=True)
    image = sub.add_parser("image", help="pack program files into an IMG1 image")
    image.add_argument("output", metavar="OUTPUT")
    image.add_argument("programs", metavar="NAME=FILE", nargs="+")
    return parser


def parse_entries(parser, raw_args):
    """Validate every NAME=FILE argument and read its file, in the given
    order (6.1: "keeps the files in the order given on the command line")."""
    entries = []
    seen = set()
    for raw in raw_args:
        name, sep, path = raw.partition("=")
        if not sep:
            parser.error(f"expected NAME=FILE, got {raw!r}")
        if not 1 <= len(name) <= MAX_NAME_LEN or not set(name) <= NAME_CHARS:
            parser.error(
                f"invalid program name {name!r}: 1-{MAX_NAME_LEN} characters "
                "from A-Z a-z 0-9 _ -"
            )
        if name in seen:
            parser.error(f"duplicate program name {name!r}")
        seen.add(name)
        try:
            with open(path, "rb") as f:
                data = f.read()
        except OSError as exc:
            parser.error(f"cannot read {path!r}: {exc.strerror}")
        if len(data) < MIN_FILE_SIZE:
            parser.error(
                f"{name!r}: {path!r} is {len(data)} bytes, "
                f"minimum program file size is {MIN_FILE_SIZE}"
            )
        entries.append((name, data))
    if len(entries) > MAX_FILES:
        parser.error(f"too many programs: {len(entries)} (maximum {MAX_FILES})")
    return entries


def pack_image(entries):
    """Lay out header + directory + file contents (6.1). Each file starts
    at a 4-byte-aligned offset, with zero-padded gaps where needed; image
    size is the end of the last file, with no trailing padding."""
    count = len(entries)
    pos = DIRECTORY_OFFSET + count * ENTRY_SIZE  # always a multiple of 4
    layout = []
    for name, data in entries:
        pos += (-pos) % 4  # align this file's start to 4 bytes
        layout.append((name, data, pos))
        pos += len(data)
    image_size = pos  # end of the last file

    header = struct.pack(
        "<4sIIII", MAGIC, VERSION, image_size, count, DIRECTORY_OFFSET
    )
    directory = bytearray()
    for name, data, offset in layout:
        name_field = name.encode("ascii").ljust(NAME_FIELD_SIZE, b"\x00")
        directory += struct.pack("<16sII", name_field, offset, len(data))

    image = bytearray(header)
    image += directory
    for _name, data, offset in layout:
        image += b"\x00" * (offset - len(image))
        image += data
    return bytes(image), image_size


def main(argv=None):
    parser = build_parser()
    args = parser.parse_args(argv)

    entries = parse_entries(parser, args.programs)
    image, image_size = pack_image(entries)
    if image_size > MAX_IMAGE_SIZE:
        parser.error(f"image is {image_size} bytes, maximum is {MAX_IMAGE_SIZE} (2 MiB)")

    with open(args.output, "wb") as f:
        f.write(image)

    print(f"IMG1: {args.output} ({image_size} bytes, {len(entries)} programs)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
