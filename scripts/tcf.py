import os
import struct
import zlib
import sys
from pathlib import Path

# -----------------------------
# Constants
# -----------------------------
MAGIC = b"TCF"
VERSION = 2
ENDIANNESS = b"\x00"
FLAGS = 0

HEADER_SIZE = 64
EOF_MARKER = b"EOF"

BUFFER_SIZE = 64 * 1024
SHIFT_BITS = 2

# -----------------------------
# Lookup tables
# -----------------------------
SHIFT_TABLE = bytes(((i << SHIFT_BITS) & 0xFF) | (i >> (8 - SHIFT_BITS)) for i in range(256))
UNSHIFT_TABLE = bytes(((i >> SHIFT_BITS) & 0xFF) | ((i << (8 - SHIFT_BITS)) & 0xFF) for i in range(256))

def shift_data(data: bytes) -> bytes:
    return data.translate(SHIFT_TABLE)

def unshift_data(data: bytes) -> bytes:
    return data.translate(UNSHIFT_TABLE)

# -----------------------------
# Pack
# -----------------------------
def pack_tcf(folder: str, output_path: str) -> None:
    folder_path = Path(folder)

    if not folder_path.exists() or not folder_path.is_dir():
        raise ValueError("Invalid input folder")

    files = []
    total_size = 0

    for f in folder_path.rglob("*"):
        if f.is_file():
            rel = str(f.relative_to(folder_path)).replace("\\", "/")
            size = f.stat().st_size
            total_size += size
            files.append((rel, f, size))

    if not files:
        raise ValueError("No files to pack")

    files.sort(key=lambda x: x[2])

    with open(output_path, "wb", buffering=BUFFER_SIZE) as out:
        # Reserve header
        out.write(b"\x00" * HEADER_SIZE)

        offsets = []
        sizes = []
        paths = []

        current_offset = 0

        # Write payload
        for path, fpath, size in files:
            paths.append(path)
            offsets.append(current_offset)
            sizes.append(size)

            with open(fpath, "rb") as f:
                while chunk := f.read(BUFFER_SIZE):
                    out.write(shift_data(chunk))

            current_offset += size

        payload_end = out.tell()

        # Write index
        for path, offset, size in zip(paths, offsets, sizes):
            pb = path.encode("utf-8")

            out.write(struct.pack("<H", len(pb)))
            out.write(pb)
            out.write(struct.pack("<Q", offset))
            out.write(struct.pack("<Q", size))

        out.write(EOF_MARKER)

        index_offset = payload_end

        # -----------------------------
        # Build header (64 bytes)
        # -----------------------------
        header = bytearray(HEADER_SIZE)

        header[0:3] = MAGIC
        header[3] = VERSION
        header[4] = ENDIANNESS[0]
        header[5] = FLAGS

        # 6-7 reserved

        struct.pack_into("<Q", header, 8, index_offset)
        struct.pack_into("<Q", header, 16, len(files))
        struct.pack_into("<Q", header, 24, HEADER_SIZE)

        # Remaining space reserved (future use)

        crc = zlib.crc32(header[:56]) & 0xFFFFFFFF
        struct.pack_into("<I", header, 56, crc)

        # Write header
        out.seek(0)
        out.write(header)

    print(f"Packed {len(files)} files")
    print(f"Original size: {total_size} bytes")
    print(f"TCF size: {os.path.getsize(output_path)} bytes")

# -----------------------------
# Unpack
# -----------------------------
def unpack_tcf(tcf_path: str, output_folder: str) -> None:
    with open(tcf_path, "rb") as f:
        data = f.read()

    if data[0:3] != MAGIC:
        raise ValueError("Invalid magic")

    version = data[3]
    if version != 2:
        raise ValueError("Unsupported version")

    index_offset = struct.unpack_from("<Q", data, 8)[0]
    file_count = struct.unpack_from("<Q", data, 16)[0]
    data_offset = struct.unpack_from("<Q", data, 24)[0]

    crc_expected = struct.unpack_from("<I", data, 56)[0]
    if zlib.crc32(data[:56]) & 0xFFFFFFFF != crc_expected:
        raise ValueError("CRC mismatch")

    payload = data[data_offset:index_offset]

    pos = index_offset
    entries = []

    for _ in range(file_count):
        path_len = struct.unpack_from("<H", data, pos)[0]
        pos += 2

        path = data[pos:pos+path_len].decode("utf-8")
        pos += path_len

        offset = struct.unpack_from("<Q", data, pos)[0]
        pos += 8

        size = struct.unpack_from("<Q", data, pos)[0]
        pos += 8

        entries.append((path, offset, size))

    root = Path(output_folder)
    root.mkdir(parents=True, exist_ok=True)

    for path, offset, size in entries:
        out_path = root / path
        out_path.parent.mkdir(parents=True, exist_ok=True)

        raw = payload[offset:offset+size]

        with open(out_path, "wb") as f:
            f.write(unshift_data(raw))

    print(f"Extracted {len(entries)} files")

# -----------------------------
# View raw vs decoded
# -----------------------------
def view_obfuscated(tcf_path: str, file_index: int = 0, num_bytes: int = 100) -> None:
    with open(tcf_path, "rb") as f:
        data = f.read()

    if data[0:3] != MAGIC:
        raise ValueError("Invalid magic")

    index_offset = struct.unpack_from("<Q", data, 8)[0]
    file_count = struct.unpack_from("<Q", data, 16)[0]
    data_offset = struct.unpack_from("<Q", data, 24)[0]

    pos = index_offset
    entries = []

    for _ in range(file_count):
        l = struct.unpack_from("<H", data, pos)[0]
        pos += 2

        path = data[pos:pos+l].decode()
        pos += l

        offset = struct.unpack_from("<Q", data, pos)[0]
        pos += 8

        size = struct.unpack_from("<Q", data, pos)[0]
        pos += 8

        entries.append((path, offset, size))

    if file_index >= len(entries):
        print("Index out of range")
        return

    path, offset, size = entries[file_index]

    chunk = data[data_offset + offset : data_offset + offset + min(num_bytes, size)]

    print(path)
    print("Obfuscated:", chunk.hex())
    print("Unshifted :", unshift_data(chunk).hex())

# -----------------------------
# CLI
# -----------------------------
if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage:")
        print("  pack <folder> <output.tcf>")
        print("  unpack <input.tcf> <output_folder>")
        print("  view <input.tcf> [index] [bytes]")
        sys.exit(1)

    cmd = sys.argv[1]

    if cmd == "pack":
        pack_tcf(sys.argv[2], sys.argv[3])
    elif cmd == "unpack":
        unpack_tcf(sys.argv[2], sys.argv[3])
    elif cmd == "view":
        idx = int(sys.argv[3]) if len(sys.argv) > 3 else 0
        n = int(sys.argv[4]) if len(sys.argv) > 4 else 100
        view_obfuscated(sys.argv[2], idx, n)
    else:
        print("Unknown command")
