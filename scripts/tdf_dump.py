#!/usr/bin/env python3
import struct
import sys
from dataclasses import dataclass

MAGIC = b"TDF"

TYPE = {
    0: "Null",
    1: "Bool",
    2: "Int32",
    3: "UInt32",
    4: "Float",
    5: "String",
    6: "Object",
    0xE0: "ArrBool",
    0xE1: "ArrInt32",
    0xE2: "ArrUInt32",
    0xE3: "ArrFloat",
    0xE4: "ArrString",
    0xE5: "ArrObject",
}


# -----------------------------
# helpers
# -----------------------------

class Cursor:
    def __init__(self, data):
        self.data = data
        self.p = 0

    def u8(self):
        v = self.data[self.p]
        self.p += 1
        return v

    def u32(self):
        v = struct.unpack_from("<I", self.data, self.p)[0]
        self.p += 4
        return v

    def i32(self):
        v = struct.unpack_from("<i", self.data, self.p)[0]
        self.p += 4
        return v

    def f32(self):
        v = struct.unpack_from("<f", self.data, self.p)[0]
        self.p += 4
        return v

    def bytes(self, n):
        v = self.data[self.p:self.p+n]
        self.p += n
        return v


@dataclass
class Entry:
    key: str
    type_id: int
    offset: int


# -----------------------------
# index parsing
# -----------------------------

def read_index(data):
    r = Cursor(data)
    r.p = 4  # skip magic + version

    entries = []

    while True:
        if r.p >= len(data):
            raise ValueError("Unexpected EOF in index")

        key_len = r.u8()
        if key_len == 0:
            break

        key = data[r.p:r.p + key_len].decode("utf-8", "replace")
        r.p += key_len

        type_id = r.u8()
        offset = r.u32()

        entries.append(Entry(key, type_id, offset))

    return entries, r.p


# -----------------------------
# decoding
# -----------------------------

def parse_object(blob):
    if len(blob) < 4:
        return {"error": "too small"}

    entries, data_start = read_index(blob)
    out = {}

    for e in entries:
        start = data_start + e.offset
        chunk = blob[start:]

        out[e.key] = {
            "type": TYPE.get(e.type_id, f"Unknown({e.type_id})"),
            "value": decode(chunk, e.type_id)
        }

    return out


def read_cstring(c):
    s = bytearray()
    while True:
        b = c.u8()
        if b == 0:
            break
        s.append(b)
    return s.decode("utf-8", "replace")


def decode(data, type_id):
    c = Cursor(data)

    # ---------------- scalar types ----------------
    if type_id == 0:
        return None

    if type_id == 1:
        return bool(c.u8())

    if type_id == 2:
        return c.i32()

    if type_id == 3:
        return c.u32()

    if type_id == 4:
        return c.f32()

    if type_id == 5:
        return read_cstring(c)

    # ---------------- object ----------------
    if type_id == 6:
        ln = c.u32()
        return parse_object(c.bytes(ln))

    # ---------------- arrays ----------------
    if type_id == 0xE0:  # bool
        count = c.u32()
        return [bool(c.u8()) for _ in range(count)]

    if type_id == 0xE1:  # int32
        count = c.u32()
        return [c.i32() for _ in range(count)]

    if type_id == 0xE2:  # uint32
        count = c.u32()
        return [c.u32() for _ in range(count)]

    if type_id == 0xE3:  # float
        count = c.u32()
        return [c.f32() for _ in range(count)]

    if type_id == 0xE4:  # string array
        count = c.u32()
        out = []
        for _ in range(count):
            out.append(read_cstring(c))
        return out

    if type_id == 0xE5:  # object array
        count = c.u32()
        out = []
        for _ in range(count):
            ln = c.u32()
            out.append(parse_object(c.bytes(ln)))
        return out

    # fallback
    return data.hex()


# -----------------------------
# pretty print
# -----------------------------

def dump(obj, indent=0):
    pad = "  " * indent

    if isinstance(obj, dict):
        for k, v in obj.items():
            if isinstance(v, dict) and "value" in v:
                print(f"{pad}{k} ({v['type']}):")
                dump(v["value"], indent + 1)
            else:
                print(f"{pad}{k}:")
                dump(v, indent + 1)
    else:
        print(pad + repr(obj))


# -----------------------------
# main
# -----------------------------

def main():
    if len(sys.argv) != 2:
        print("Usage: tdf_dump.py file.tdf")
        return

    with open(sys.argv[1], "rb") as f:
        data = f.read()

    if data[:3] != MAGIC:
        raise ValueError("Not a TDF file")

    print(f"TDF version: {data[3]}")
    parsed = parse_object(data)
    dump(parsed)


if __name__ == "__main__":
    main()
