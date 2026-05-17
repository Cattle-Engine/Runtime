#!/usr/bin/env python3
import argparse
import json
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Tuple


# Matches include/engine/common/fs/tdf.hpp
MAGIC = b"TDF"
VERSION_OBJECT = 0x11
DEFAULT_ROOT_KEY = "Value"


class Type:
    Null = 0x00
    Bool = 0x01
    Int32 = 0x02
    UInt32 = 0x03
    Float = 0x04
    String = 0x05
    Object = 0x06

    ArrBool = 0xE0
    ArrInt32 = 0xE1
    ArrUInt32 = 0xE2
    ArrFloat = 0xE3
    ArrString = 0xE4
    ArrObject = 0xE5


def _u32(v: int) -> bytes:
    return struct.pack("<I", int(v) & 0xFFFFFFFF)

def _i32(v: int) -> bytes:
    return struct.pack("<i", int(v))

def _f32(v: float) -> bytes:
    return struct.pack("<f", float(v))


def _cstring(s: str) -> bytes:
    return s.encode("utf-8") + b"\x00"


@dataclass(frozen=True)
class Entry:
    key: str
    type_id: int
    data: bytes


def _serialize_entries(entries: List[Entry], *, version: int) -> bytes:
    out = bytearray()
    out += MAGIC
    out.append(version & 0xFF)

    index = bytearray()
    data = bytearray()

    for e in entries:
        key_b = e.key.encode("utf-8")
        if not (1 <= len(key_b) <= 255):
            raise ValueError(f"Key '{e.key}' length must be 1..255 bytes")

        offset = len(data)
        index.append(len(key_b))
        index += key_b
        index.append(e.type_id & 0xFF)
        index += _u32(offset)

        data += e.data

    index.append(0)  # end of index
    out += index
    out += data
    return bytes(out)


def tdf_string(key: str, value: str) -> Entry:
    return Entry(key=key, type_id=Type.String, data=_cstring(value))

def tdf_bool(key: str, value: bool) -> Entry:
    return Entry(key=key, type_id=Type.Bool, data=bytes([1 if value else 0]))

def tdf_i32(key: str, value: int) -> Entry:
    return Entry(key=key, type_id=Type.Int32, data=_i32(value))

def tdf_u32(key: str, value: int) -> Entry:
    return Entry(key=key, type_id=Type.UInt32, data=_u32(value))

def tdf_f32(key: str, value: float) -> Entry:
    return Entry(key=key, type_id=Type.Float, data=_f32(value))

def tdf_null(key: str) -> Entry:
    return Entry(key=key, type_id=Type.Null, data=b"")


def tdf_object(key: str, obj_entries: List[Entry], *, version: int = VERSION_OBJECT) -> Entry:
    payload = _serialize_entries(obj_entries, version=version)
    return Entry(key=key, type_id=Type.Object, data=_u32(len(payload)) + payload)

def tdf_bool_array(key: str, values: List[bool]) -> Entry:
    data = bytearray()
    data += _u32(len(values))
    data += bytes([1 if v else 0 for v in values])
    return Entry(key=key, type_id=Type.ArrBool, data=bytes(data))

def tdf_i32_array(key: str, values: List[int]) -> Entry:
    data = bytearray()
    data += _u32(len(values))
    for v in values:
        data += _i32(v)
    return Entry(key=key, type_id=Type.ArrInt32, data=bytes(data))

def tdf_u32_array(key: str, values: List[int]) -> Entry:
    data = bytearray()
    data += _u32(len(values))
    for v in values:
        data += _u32(v)
    return Entry(key=key, type_id=Type.ArrUInt32, data=bytes(data))

def tdf_f32_array(key: str, values: List[float]) -> Entry:
    data = bytearray()
    data += _u32(len(values))
    for v in values:
        data += _f32(v)
    return Entry(key=key, type_id=Type.ArrFloat, data=bytes(data))

def tdf_string_array(key: str, values: List[str]) -> Entry:
    data = bytearray()
    data += _u32(len(values))
    for v in values:
        data += _cstring(v)
    return Entry(key=key, type_id=Type.ArrString, data=bytes(data))


def tdf_object_array(key: str, objects: List[List[Entry]], *, version: int = VERSION_OBJECT) -> Entry:
    data = bytearray()
    data += _u32(len(objects))
    for obj_entries in objects:
        payload = _serialize_entries(obj_entries, version=version)
        data += _u32(len(payload))
        data += payload
    return Entry(key=key, type_id=Type.ArrObject, data=bytes(data))


def _iter_json_files(in_dir: Path) -> Iterable[Path]:
    if not in_dir.exists():
        return []
    return sorted([p for p in in_dir.rglob("*.json") if p.is_file()])

def _maybe_typed_wrapper(obj: Any) -> Optional[Tuple[str, Any]]:
    # Explicit type override:
    #   { "__type": "UInt32", "value": 123 }
    #   { "__type": "String", "value": "hi" }
    if not isinstance(obj, dict):
        return None
    if set(obj.keys()) != {"__type", "value"}:
        return None
    t = obj.get("__type")
    v = obj.get("value")
    if not isinstance(t, str):
        return None
    return t, v

def _json_value_to_entry(key: str, value: Any, *, version: int, root_key_for_wrapped_array: str) -> Entry:
    tw = _maybe_typed_wrapper(value)
    if tw is not None:
        t, v = tw
        if t == "Null":
            return tdf_null(key)
        if t == "Bool":
            return tdf_bool(key, bool(v))
        if t == "Int32":
            return tdf_i32(key, int(v))
        if t == "UInt32":
            return tdf_u32(key, int(v))
        if t == "Float":
            return tdf_f32(key, float(v))
        if t == "String":
            return tdf_string(key, str(v))
        raise ValueError(f"Unsupported __type '{t}' for key '{key}'")

    if value is None:
        return tdf_null(key)

    if isinstance(value, bool):
        return tdf_bool(key, value)

    if isinstance(value, int) and not isinstance(value, bool):
        # Prefer UInt32 for non-negative values that fit, else Int32.
        if 0 <= value <= 0xFFFFFFFF:
            return tdf_u32(key, value)
        if -0x80000000 <= value <= 0x7FFFFFFF:
            return tdf_i32(key, value)
        raise ValueError(f"Integer out of 32-bit range for key '{key}': {value}")

    if isinstance(value, float):
        return tdf_f32(key, value)

    if isinstance(value, str):
        return tdf_string(key, value)

    if isinstance(value, dict):
        child_entries: List[Entry] = []
        for k, v in value.items():
            if not isinstance(k, str):
                raise ValueError(f"Non-string key in object for '{key}'")
            child_entries.append(_json_value_to_entry(k, v, version=version, root_key_for_wrapped_array=root_key_for_wrapped_array))
        return tdf_object(key, child_entries, version=version)

    if isinstance(value, list):
        # Choose an array type when homogeneous, else encode as ArrObject by wrapping elements.
        if len(value) == 0:
            return tdf_object_array(key, [], version=version)

        if all(isinstance(x, bool) for x in value):
            return tdf_bool_array(key, [bool(x) for x in value])

        if all(isinstance(x, int) and not isinstance(x, bool) for x in value):
            ints = [int(x) for x in value]
            if all(0 <= x <= 0xFFFFFFFF for x in ints):
                return tdf_u32_array(key, ints)
            if all(-0x80000000 <= x <= 0x7FFFFFFF for x in ints):
                return tdf_i32_array(key, ints)
            raise ValueError(f"Integer array out of 32-bit range for key '{key}'")

        if all(isinstance(x, (int, float)) and not isinstance(x, bool) for x in value):
            return tdf_f32_array(key, [float(x) for x in value])

        if all(isinstance(x, str) for x in value):
            return tdf_string_array(key, [str(x) for x in value])

        if all(isinstance(x, dict) for x in value):
            objs: List[List[Entry]] = []
            for obj in value:
                obj_entries: List[Entry] = []
                for k, v in obj.items():
                    if not isinstance(k, str):
                        raise ValueError(f"Non-string key in object array for '{key}'")
                    obj_entries.append(_json_value_to_entry(k, v, version=version, root_key_for_wrapped_array=root_key_for_wrapped_array))
                objs.append(obj_entries)
            return tdf_object_array(key, objs, version=version)

        # Mixed array: wrap each element as an object {root_key_for_wrapped_array: <value>}.
        wrapped_objs: List[List[Entry]] = []
        for elem in value:
            wrapped_objs.append([
                _json_value_to_entry(root_key_for_wrapped_array, elem, version=version, root_key_for_wrapped_array=root_key_for_wrapped_array)
            ])
        return tdf_object_array(key, wrapped_objs, version=version)

    raise ValueError(f"Unsupported JSON value type for key '{key}': {type(value).__name__}")

def json_to_generic_tdf_entries(src: Any, *, root_key: str, version: int) -> List[Entry]:
    if isinstance(src, dict):
        entries: List[Entry] = []
        for k, v in src.items():
            if not isinstance(k, str):
                raise ValueError("Top-level JSON object keys must be strings")
            entries.append(_json_value_to_entry(k, v, version=version, root_key_for_wrapped_array=root_key))
        return entries

    # If the JSON root is not an object, wrap it.
    return [_json_value_to_entry(root_key, src, version=version, root_key_for_wrapped_array=root_key)]


def _extract_frames(src: dict) -> List[dict]:
    frames = src.get("frames")
    if frames is None:
        raise ValueError("Missing 'frames'")

    if isinstance(frames, dict):
        out = []
        for _, v in frames.items():
            out.append(v)
        return out

    if isinstance(frames, list):
        return frames

    raise ValueError(f"Unsupported 'frames' type: {type(frames).__name__}")


def json_atlas_to_animation_tdf_entries(src: dict, *, source_image_path: str) -> List[Entry]:
    frames_raw = _extract_frames(src)

    frames_obj_entries: List[List[Entry]] = []
    for f in frames_raw:
        frame = f.get("frame", f)
        if not isinstance(frame, dict):
            raise ValueError("Frame entry must be an object")

        x = int(frame.get("x", 0))
        y = int(frame.get("y", 0))
        w = int(frame.get("w", frame.get("width", 0)))
        h = int(frame.get("h", frame.get("height", 0)))

        duration = f.get("duration", f.get("Duration", 100))
        duration = int(duration)

        frames_obj_entries.append([
            tdf_u32("Width", w),
            tdf_u32("Height", h),
            tdf_u32("X", x),
            tdf_u32("Y", y),
            tdf_u32("Duration", duration),
        ])

    return [
        tdf_string("SourceImagePath", source_image_path),
        tdf_u32("FrameCount", len(frames_obj_entries)),
        tdf_object_array("Frames", frames_obj_entries, version=VERSION_OBJECT),
    ]


def _default_source_image_path(json_path: Path, src: dict) -> Optional[str]:
    meta = src.get("meta")
    if isinstance(meta, dict):
        image = meta.get("image")
        if isinstance(image, str) and image:
            return image
    # Heuristic: pick the only image in the same folder that contains the json stem.
    stem = json_path.stem.lower()
    candidates = []
    for ext in (".png", ".jpg", ".jpeg", ".webp"):
        for p in json_path.parent.glob(f"*{ext}"):
            if stem in p.stem.lower():
                candidates.append(p.name)
    if len(candidates) == 1:
        return candidates[0]
    return None


def convert_one(input_json: Path, output_tdf: Path, *, source_image_override: Optional[str]) -> None:
    with input_json.open("r", encoding="utf-8") as f:
        src = json.load(f)

    source_image = source_image_override or _default_source_image_path(input_json, src)
    if not source_image:
        print(
            f"[json_to_tdf] Skipping '{input_json}': could not determine SourceImagePath "
            f"(add meta.image in JSON or pass --source-image)."
        )
        return

    entries = json_atlas_to_animation_tdf_entries(src, source_image_path=source_image)
    blob = _serialize_entries(entries, version=VERSION_OBJECT)

    output_tdf.parent.mkdir(parents=True, exist_ok=True)
    output_tdf.write_bytes(blob)

def convert_one_generic(input_json: Path, output_tdf: Path, *, root_key: str) -> None:
    with input_json.open("r", encoding="utf-8") as f:
        src = json.load(f)

    entries = json_to_generic_tdf_entries(src, root_key=root_key, version=VERSION_OBJECT)
    blob = _serialize_entries(entries, version=VERSION_OBJECT)

    output_tdf.parent.mkdir(parents=True, exist_ok=True)
    output_tdf.write_bytes(blob)


def parse_map_args(items: List[str]) -> Dict[str, str]:
    mapping: Dict[str, str] = {}
    for it in items:
        if "=" not in it:
            raise ValueError(f"Invalid --map '{it}', expected 'in.json=out.tdf'")
        k, v = it.split("=", 1)
        k = k.strip()
        v = v.strip()
        if not k or not v:
            raise ValueError(f"Invalid --map '{it}', expected 'in.json=out.tdf'")
        mapping[k] = v
    return mapping


def main() -> int:
    ap = argparse.ArgumentParser(description="Convert JSON to CE TDF format (atlas or generic).")
    ap.add_argument(
        "--mode",
        choices=["atlas", "generic"],
        default="atlas",
        help="Conversion mode. 'atlas' expects Aseprite/TexturePacker-style frames. 'generic' converts any JSON to TDF.",
    )
    ap.add_argument("--in-dir", type=Path, action="append", default=[], help="Directory to scan for *.json (repeatable).")
    ap.add_argument("--out-dir", type=Path, required=True, help="Output directory for generated .tdf files.")
    ap.add_argument("--source-image", type=str, default=None, help="Override SourceImagePath for all converted files.")
    ap.add_argument("--root-key", type=str, default=DEFAULT_ROOT_KEY, help="Root key used when generic mode wraps a non-object JSON root.")
    ap.add_argument(
        "--map",
        action="append",
        default=[],
        help="Optional rename mapping: 'input.json=output.tdf' (repeatable).",
    )
    ap.add_argument("input", nargs="*", type=Path, help="Optional explicit input json files.")
    args = ap.parse_args()

    mapping = parse_map_args(args.map)

    inputs: List[Path] = []
    inputs.extend(args.input)
    for d in args.in_dir:
        inputs.extend(_iter_json_files(d))

    seen = set()
    unique_inputs: List[Path] = []
    for p in inputs:
        p = p.resolve()
        if p in seen:
            continue
        seen.add(p)
        unique_inputs.append(p)

    if not unique_inputs:
        return 0

    out_dir = args.out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    for inp in unique_inputs:
        out_name = mapping.get(inp.name, inp.with_suffix(".tdf").name)
        out_path = out_dir / out_name
        if args.mode == "atlas":
            convert_one(inp, out_path, source_image_override=args.source_image)
        else:
            convert_one_generic(inp, out_path, root_key=args.root_key)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
