import struct
import subprocess
import sys
import os

# =========================
# TGC v3 CONSTANTS
# =========================
TGC_MAGIC   = b"TGC"
TGC_VERSION = 3

PIXEL_H264  = 1  # new pixel format for H.264

HEADER_FMT = "<3sBHHHBBII"
INDEX_FMT  = "<IIIb"

HEADER_SIZE = struct.calcsize(HEADER_FMT)
INDEX_SIZE  = struct.calcsize(INDEX_FMT)

# =========================
# H.264 ENCODER
# =========================

def extract_h264(input_path, tmp_path, fps_override=None):
    """Use ffmpeg + OpenH264 to extract raw H.264 AnnexB stream."""
    args = [
        "ffmpeg",
        "-y",
        "-i", input_path,
        "-c:v", "libopenh264",
        "-preset", "medium",
        "-f", "h264",
        tmp_path
    ]
    if fps_override:
        args.insert(2, "-r")
        args.insert(3, str(fps_override))
    subprocess.check_call(args)

def split_h264_frames(h264_bytes):
    """Split raw AnnexB H.264 stream into NAL frames."""
    start_code = b"\x00\x00\x00\x01"
    parts = h264_bytes.split(start_code)
    frames = []
    for p in parts:
        if not p:
            continue
        frames.append(start_code + p)
    return frames

def encode_tgc_h264(input_video, output_tgc, fps_override=None):
    tmp_h264 = output_tgc + ".tmp.h264"

    extract_h264(input_video, tmp_h264, fps_override)

    with open(tmp_h264, "rb") as f:
        raw = f.read()
    os.remove(tmp_h264)

    frames = split_h264_frames(raw)
    print(f"Found {len(frames)} H.264 frames")

    with open(output_tgc, "wb") as f:
        # ---- HEADER ----
        f.write(struct.pack(
            HEADER_FMT,
            TGC_MAGIC,
            TGC_VERSION,
            0, 0, 0,                # width, height, fps unknown
            PIXEL_H264,             # pixel format
            0,                      # flags
            len(frames),            # frame count
            0                       # key interval (unused)
        ))

        # ---- INDEX PLACEHOLDER ----
        index_offset = f.tell()
        f.write(b"\x00" * (len(frames) * INDEX_SIZE))

        # ---- PAYLOAD ----
        offsets = []
        index = []

        for frame in frames:
            offsets.append(f.tell())
            comp_size = len(frame)
            raw_size = comp_size
            f.write(frame)
            index.append((offsets[-1], comp_size, raw_size, 0))

        # ---- WRITE INDEX ----
        f.seek(index_offset)
        for off, comp_size, raw_size, ftype in index:
            f.write(struct.pack(INDEX_FMT, off, comp_size, raw_size, ftype))

    print(f"Saved H.264 TGC: {output_tgc}")

# =========================
# CLI
# =========================

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python encode_h264_tgc.py input.mp4 output.tgc [fps]")
        sys.exit(1)

    fps_override = int(sys.argv[3]) if len(sys.argv) > 3 else None
    encode_tgc_h264(sys.argv[1], sys.argv[2], fps_override)
