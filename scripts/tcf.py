import os
import struct
import zlib
import sys
from pathlib import Path

# ----------------------------------------------------------------------
# TCF v3 constants (mirrors tcf.hpp)
# ----------------------------------------------------------------------
MAGIC = b"TCF"
VERSION = 3
ENDIANNESS = 0                    # Endianness::Little

HEADER_SIZE = 128                 # kHeaderSize
HEADER_CRC_LEN = 0x20             # kHeaderCrcLen
HEADER_CRC_OFFSET = 0x20

CHUNK_HEADER_SIZE = 8 + 4 + 1 + 8 + 8 + 8    # 37
DIRECTORY_CONTENT_SIZE = 1 + 8                # 9

FILE_CHUNK_SIZE = 256 * 1024                  # kFileChunkSize
MAX_NAME_SIZE = 255                           # kMaxNameSize
MAX_CHUNKS_PER_FILE = 1_000_000               # kMaxChunksPerFile
MAX_FILE_COUNT = 10_000_000                   # kMaxFileCount

# enum class CompressionType : uint8_t { Zstd = 0, LZ4 = 1, None = 2 };
COMPRESSION_NONE = 2

# enum class DirectoryContentType : uint8_t { Directory = 0, File = 1 };
CONTENT_TYPE_DIRECTORY = 0
CONTENT_TYPE_FILE = 1

BUFFER_SIZE = 64 * 1024


def crc32(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF


def validate_name(name: str) -> None:
    if not name:
        raise ValueError("Empty name")
    if name == "." or name == "..":
        raise ValueError(f"Forbidden name: {name!r}")
    if len(name.encode("ascii", errors="ignore")) > MAX_NAME_SIZE or len(name) > MAX_NAME_SIZE:
        raise ValueError(f"Name too long: {name!r}")
    for c in name:
        if ord(c) > 0x7F:
            raise ValueError(f"Non-ASCII character in name {name!r}")
        if c in ("/", "\\", "\0"):
            raise ValueError(f"Invalid character in name {name!r}")


# ----------------------------------------------------------------------
# Pack
# ----------------------------------------------------------------------
def pack_tcf(folder: str, output_path: str) -> None:
    folder_path = Path(folder)

    if not folder_path.exists() or not folder_path.is_dir():
        raise ValueError("Invalid input folder")

    files = []                 # (rel_path, abs_path, size, mtime_ms)
    dir_set = {""}             # relative directory paths, root == ""

    for item in folder_path.rglob("*"):
        rel = item.relative_to(folder_path).as_posix()
        if item.is_dir():
            dir_set.add(rel)
        elif item.is_file():
            st = item.stat()
            files.append((rel, item, st.st_size, int(st.st_mtime * 1000)))

    if not files:
        raise ValueError("No files to pack")

    files.sort(key=lambda x: x[0])
    dir_list = [""] + sorted(
        (d for d in dir_set if d != ""),
        key=lambda p: (p.count("/"), p),
    )

    dir_id_map = {d: i for i, d in enumerate(dir_list)}
    file_id_map = {fp: i for i, (fp, _, _, _) in enumerate(files)}

    # Build directory contents -----------------------------------------
    dir_contents = {d: [] for d in dir_list}

    for fp, _, _, _ in files:
        parent = os.path.dirname(fp)
        dir_contents[parent].append((CONTENT_TYPE_FILE, file_id_map[fp]))

    for d in dir_list:
        if d == "":
            continue
        parent = os.path.dirname(d)
        dir_contents[parent].append((CONTENT_TYPE_DIRECTORY, dir_id_map[d]))

    # Validate names + uniqueness --------------------------------------
    for fp, _, _, _ in files:
        validate_name(os.path.basename(fp))
    for d in dir_list:
        if d == "":
            continue
        validate_name(os.path.basename(d))

    for d, contents in dir_contents.items():
        seen = set()
        for ctype, cid in contents:
            if ctype == CONTENT_TYPE_FILE:
                name = os.path.basename(files[cid][0])
            else:
                name = os.path.basename(dir_list[cid])
            if name in seen:
                raise ValueError(f"Duplicate name '{name}' in directory '{d or '/'}'")
            seen.add(name)

    # ----------------------------------------------------------------
    # Write archive
    # ----------------------------------------------------------------
    total_uncompressed = 0

    with open(output_path, "wb", buffering=BUFFER_SIZE) as out:
        # Reserve header
        out.write(b"\x00" * HEADER_SIZE)
        data_offset = HEADER_SIZE

        # ------------------------------------------------------------
        # Chunk data region
        # ------------------------------------------------------------
        file_chunk_blocks = []  # (chunk_block_offset, chunks_list)

        for fp, fpath, _size, _mtime in files:
            chunk_block_offset = out.tell()
            chunks = []

            with open(fpath, "rb") as f:
                chunk_id = 0
                while True:
                    chunk_data = f.read(FILE_CHUNK_SIZE)
                    if not chunk_data:
                        break

                    crc = crc32(chunk_data)
                    comp_size = len(chunk_data)
                    uncomp_size = len(chunk_data)

                    # Compute offsets BEFORE writing the header.
                    # Layout: [37-byte header][data][0x00]
                    data_start = out.tell() + CHUNK_HEADER_SIZE
                    data_end_offset = data_start + comp_size

                    out.write(struct.pack("<Q", chunk_id))
                    out.write(struct.pack("<I", crc))
                    out.write(struct.pack("<B", COMPRESSION_NONE))
                    out.write(struct.pack("<Q", comp_size))
                    out.write(struct.pack("<Q", uncomp_size))
                    out.write(struct.pack("<Q", data_end_offset))

                    assert out.tell() == data_start
                    out.write(chunk_data)
                    assert out.tell() == data_end_offset
                    out.write(b"\x00")

                    chunks.append({
                        "id": chunk_id,
                        "crc": crc,
                        "comp_size": comp_size,
                        "uncomp_size": uncomp_size,
                        "data_end_offset": data_end_offset,
                    })

                    chunk_id += 1
                    total_uncompressed += len(chunk_data)

                    if len(chunk_data) < FILE_CHUNK_SIZE:
                        break

            file_chunk_blocks.append((chunk_block_offset, chunks))

        # ------------------------------------------------------------
        # File info region
        # ------------------------------------------------------------
        file_info_offset = out.tell()

        for i, (fp, _, _, mtime) in enumerate(files):
            chunk_block_offset, chunks = file_chunk_blocks[i]
            name_bytes = os.path.basename(fp).encode("ascii")
            parent_dir = dir_id_map[os.path.dirname(fp)]

            out.write(struct.pack("<Q", i))
            out.write(struct.pack("<q", mtime))
            out.write(struct.pack("<I", len(name_bytes)))
            out.write(name_bytes)
            out.write(struct.pack("<Q", parent_dir))
            out.write(struct.pack("<Q", len(chunks)))
            # Empty files get chunk_block_offset = 0 (unused by the reader).
            out.write(struct.pack("<Q", chunk_block_offset if chunks else 0))

        # ------------------------------------------------------------
        # Directory table
        # ------------------------------------------------------------
        directory_table_offset = out.tell()

        for d in dir_list:
            dir_id = dir_id_map[d]

            if d == "":
                name_bytes = b""
                parent = 0
                mtime = int(folder_path.stat().st_mtime * 1000)
            else:
                name_bytes = os.path.basename(d).encode("ascii")
                parent = dir_id_map[os.path.dirname(d)]
                mtime = int((folder_path / d).stat().st_mtime * 1000)

            contents = dir_contents[d]
            content_bytes = len(contents) * DIRECTORY_CONTENT_SIZE

            # content_end is the absolute archive offset immediately
            # after the last DirectoryContent record.
            content_end = (
                out.tell()
                + 8                  # id
                + 8                  # date_modified
                + 4                  # name_size
                + len(name_bytes)    # name
                + 8                  # parent
                + 8                  # content_end field
                + content_bytes      # contents
            )

            out.write(struct.pack("<Q", dir_id))
            out.write(struct.pack("<q", mtime))
            out.write(struct.pack("<I", len(name_bytes)))
            out.write(name_bytes)
            out.write(struct.pack("<Q", parent))
            out.write(struct.pack("<Q", content_end))

            for ctype, cid in contents:
                out.write(struct.pack("<B", ctype))
                out.write(struct.pack("<Q", cid))

            assert out.tell() == content_end, (out.tell(), content_end)

        # ------------------------------------------------------------
        # Header
        # ------------------------------------------------------------
        header = bytearray(HEADER_SIZE)
        header[0:3] = MAGIC
        header[3] = VERSION
        header[4] = ENDIANNESS
        # bytes 5..7 reserved = 0

        struct.pack_into("<Q", header, 0x08, file_info_offset)
        struct.pack_into("<Q", header, 0x10, len(files))
        struct.pack_into("<Q", header, 0x18, data_offset)
        struct.pack_into("<Q", header, 0x24, directory_table_offset)
        struct.pack_into("<Q", header, 0x2C, len(dir_list))

        header_crc = crc32(bytes(header[:HEADER_CRC_LEN]))
        struct.pack_into("<I", header, HEADER_CRC_OFFSET, header_crc)

        out.seek(0)
        out.write(header)

    print(f"Packed {len(files)} files, {len(dir_list)} directories")
    print(f"Original size: {total_uncompressed} bytes")
    print(f"TCF size: {os.path.getsize(output_path)} bytes")


# ----------------------------------------------------------------------
# Parse
# ----------------------------------------------------------------------
def _parse_archive(data: bytes):
    archive_size = len(data)

    if archive_size < HEADER_SIZE:
        raise ValueError("File smaller than header")
    if data[0:3] != MAGIC:
        raise ValueError("Invalid magic")
    if data[3] != VERSION:
        raise ValueError(f"Unsupported version: {data[3]}")
    if data[4] != ENDIANNESS:
        raise ValueError(f"Unsupported endianness: {data[4]}")
    for i in (5, 6, 7):
        if data[i] != 0:
            raise ValueError("Reserved header bytes not zero")

    file_info_offset = struct.unpack_from("<Q", data, 0x08)[0]
    file_count       = struct.unpack_from("<Q", data, 0x10)[0]
    data_offset      = struct.unpack_from("<Q", data, 0x18)[0]
    stored_crc       = struct.unpack_from("<I", data, 0x20)[0]
    directory_table_offset = struct.unpack_from("<Q", data, 0x24)[0]
    directory_count  = struct.unpack_from("<Q", data, 0x2C)[0]

    for i in range(0x34, HEADER_SIZE):
        if data[i] != 0:
            raise ValueError("Reserved header bytes not zero")

    computed_crc = crc32(data[:HEADER_CRC_LEN])
    if computed_crc != stored_crc:
        raise ValueError(f"Header CRC mismatch: {computed_crc:#x} != {stored_crc:#x}")

    # ---- File info -------------------------------------------------
    files = []
    pos = file_info_offset
    for expected_id in range(file_count):
        file_id = struct.unpack_from("<Q", data, pos)[0]; pos += 8
        if file_id != expected_id:
            raise ValueError(f"Bad file id {file_id}, expected {expected_id}")
        mtime = struct.unpack_from("<q", data, pos)[0]; pos += 8
        name_size = struct.unpack_from("<I", data, pos)[0]; pos += 4
        name = data[pos:pos + name_size].decode("ascii"); pos += name_size
        parent = struct.unpack_from("<Q", data, pos)[0]; pos += 8
        chunk_count = struct.unpack_from("<Q", data, pos)[0]; pos += 8
        chunk_block_offset = struct.unpack_from("<Q", data, pos)[0]; pos += 8

        files.append({
            "id": file_id,
            "mtime": mtime,
            "name": name,
            "parent": parent,
            "chunk_count": chunk_count,
            "chunk_block_offset": chunk_block_offset,
        })

    # ---- Directory table -------------------------------------------
    directories = []
    pos = directory_table_offset
    for expected_id in range(directory_count):
        dir_id = struct.unpack_from("<Q", data, pos)[0]; pos += 8
        if dir_id != expected_id:
            raise ValueError(f"Bad directory id {dir_id}, expected {expected_id}")
        mtime = struct.unpack_from("<q", data, pos)[0]; pos += 8
        name_size = struct.unpack_from("<I", data, pos)[0]; pos += 4
        name = data[pos:pos + name_size].decode("ascii"); pos += name_size
        parent = struct.unpack_from("<Q", data, pos)[0]; pos += 8
        content_end = struct.unpack_from("<Q", data, pos)[0]; pos += 8

        content_size = content_end - pos
        if content_size % DIRECTORY_CONTENT_SIZE != 0:
            raise ValueError("Malformed directory content size")
        content_count = content_size // DIRECTORY_CONTENT_SIZE

        contents = []
        for _ in range(content_count):
            ctype = struct.unpack_from("<B", data, pos)[0]; pos += 1
            cid = struct.unpack_from("<Q", data, pos)[0]; pos += 8
            contents.append((ctype, cid))

        directories.append({
            "id": dir_id,
            "mtime": mtime,
            "name": name,
            "parent": parent,
            "contents": contents,
        })

    return {
        "file_count": file_count,
        "directory_count": directory_count,
        "data_offset": data_offset,
        "file_info_offset": file_info_offset,
        "directory_table_offset": directory_table_offset,
        "files": files,
        "directories": directories,
    }


def _read_file_bytes(data: bytes, file_info) -> bytes:
    chunk_count = file_info["chunk_count"]
    if chunk_count == 0:
        return b""

    out = bytearray()
    pos = file_info["chunk_block_offset"]

    for expected_chunk_id in range(chunk_count):
        header_start = pos
        chunk_id = struct.unpack_from("<Q", data, pos)[0]; pos += 8
        if chunk_id != expected_chunk_id:
            raise ValueError(f"Bad chunk id {chunk_id}, expected {expected_chunk_id}")
        crc = struct.unpack_from("<I", data, pos)[0]; pos += 4
        compression = struct.unpack_from("<B", data, pos)[0]; pos += 1
        comp_size = struct.unpack_from("<Q", data, pos)[0]; pos += 8
        uncomp_size = struct.unpack_from("<Q", data, pos)[0]; pos += 8
        data_end_offset = struct.unpack_from("<Q", data, pos)[0]; pos += 8

        data_start = header_start + CHUNK_HEADER_SIZE
        if data_end_offset != data_start + comp_size:
            raise ValueError("Chunk physical size mismatch")

        terminator = data[data_end_offset]
        if terminator != 0:
            raise ValueError("Chunk missing 0x00 terminator")

        chunk_bytes = data[data_start:data_end_offset]

        if compression != COMPRESSION_NONE:
            raise ValueError(f"Unsupported compression {compression}")
        if crc32(chunk_bytes) != crc:
            raise ValueError("Chunk CRC mismatch")
        if len(chunk_bytes) != uncomp_size:
            raise ValueError("Uncompressed size mismatch")

        out.extend(chunk_bytes)
        pos = data_end_offset + 1

    return bytes(out)


def unpack_tcf(tcf_path: str, output_folder: str) -> None:
    with open(tcf_path, "rb") as f:
        data = f.read()

    archive = _parse_archive(data)
    files = archive["files"]
    directories = archive["directories"]

    root = Path(output_folder)
    root.mkdir(parents=True, exist_ok=True)

    def walk(dir_id: int, current: Path) -> None:
        for ctype, cid in directories[dir_id]["contents"]:
            if ctype == CONTENT_TYPE_FILE:
                fi = files[cid]
                target = current / fi["name"]
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(_read_file_bytes(data, fi))
            elif ctype == CONTENT_TYPE_DIRECTORY:
                sub = current / directories[cid]["name"]
                sub.mkdir(parents=True, exist_ok=True)
                walk(cid, sub)
            else:
                raise ValueError(f"Unknown content type {ctype}")

    walk(0, root)
    print(f"Extracted {len(files)} files, {len(directories)} directories")


# ----------------------------------------------------------------------
# View
# ----------------------------------------------------------------------
def view_tcf(tcf_path: str, file_index: int = 0, num_bytes: int = 64) -> None:
    with open(tcf_path, "rb") as f:
        data = f.read()

    archive = _parse_archive(data)
    files = archive["files"]
    directories = archive["directories"]

    print(f"Version          : {data[3]}")
    print(f"File count       : {archive['file_count']}")
    print(f"Directory count  : {archive['directory_count']}")
    print(f"Data offset      : {archive['data_offset']}")
    print(f"File info offset : {archive['file_info_offset']}")
    print(f"Dir table offset : {archive['directory_table_offset']}")

    print("\nDirectories:")
    for d in directories:
        parent = "-" if d["id"] == 0 else (directories[d["parent"]]["name"] or "/")
        print(f"  [{d['id']}] {d['name'] or '/'} (parent={parent}, entries={len(d['contents'])})")

    print("\nFiles:")
    for fi in files:
        print(f"  [{fi['id']}] {fi['name']} (parent_dir={fi['parent']}, "
              f"chunks={fi['chunk_count']}, block@{fi['chunk_block_offset']})")

    if not files:
        return
    if file_index >= len(files):
        print("\nIndex out of range")
        return

    fi = files[file_index]
    if fi["chunk_count"] == 0:
        print(f"\n'{fi['name']}' is empty")
        return

    pos = fi["chunk_block_offset"]
    chunk_id = struct.unpack_from("<Q", data, pos)[0]
    crc = struct.unpack_from("<I", data, pos + 8)[0]
    compression = struct.unpack_from("<B", data, pos + 12)[0]
    comp_size = struct.unpack_from("<Q", data, pos + 13)[0]
    uncomp_size = struct.unpack_from("<Q", data, pos + 21)[0]
    data_end = struct.unpack_from("<Q", data, pos + 29)[0]

    data_start = pos + CHUNK_HEADER_SIZE
    preview = data[data_start:data_start + min(num_bytes, comp_size)]

    print(f"\nFirst chunk of '{fi['name']}':")
    print(f"  chunk_id        : {chunk_id}")
    print(f"  crc32           : {crc:#010x}")
    print(f"  compression     : {compression}")
    print(f"  compressed_size : {comp_size}")
    print(f"  uncompressed    : {uncomp_size}")
    print(f"  data_end_offset : {data_end}")
    print(f"  preview (hex)   : {preview.hex()}")


# ----------------------------------------------------------------------
# CLI
# ----------------------------------------------------------------------
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage:")
        print("  pack   <folder>    <output.tcf>")
        print("  unpack <input.tcf> <output_folder>")
        print("  view   <input.tcf> [index] [bytes]")
        sys.exit(1)

    cmd = sys.argv[1]

    if cmd == "pack":
        if len(sys.argv) != 4:
            print("Usage: pack <folder> <output.tcf>")
            sys.exit(1)
        pack_tcf(sys.argv[2], sys.argv[3])
    elif cmd == "unpack":
        if len(sys.argv) != 4:
            print("Usage: unpack <input.tcf> <output_folder>")
            sys.exit(1)
        unpack_tcf(sys.argv[2], sys.argv[3])
    elif cmd == "view":
        if len(sys.argv) < 3:
            print("Usage: view <input.tcf> [index] [bytes]")
            sys.exit(1)
        idx = int(sys.argv[3]) if len(sys.argv) > 3 else 0
        n = int(sys.argv[4]) if len(sys.argv) > 4 else 64
        view_tcf(sys.argv[2], idx, n)
    else:
        print("Unknown command")
        sys.exit(1)