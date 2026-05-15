#!/usr/bin/env python3
import json
import argparse
import sys
from pathlib import Path
from typing import Dict, List, Optional


def extract_frame_number(frame_name: str) -> int:
    parts = frame_name.split()
    if len(parts) >= 2:
        try:
            return int(parts[-1].replace('.gif', ''))
        except ValueError:
            pass
    import re
    numbers = re.findall(r'\d+', frame_name)
    if numbers:
        return int(numbers[-1])

    return 0


def sort_frames(frames_dict: Dict) -> List[tuple]:
    frame_items = list(frames_dict.items())
    try:
        frame_items.sort(key=lambda x: extract_frame_number(x[0]))
    except:
        pass

    return frame_items


def convert_aseprite_to_engine_format(input_json: Path, source_image: Optional[str] = None) -> Dict:
    with open(input_json, 'r', encoding='utf-8') as f:
        aseprite_data = json.load(f)
    if source_image is None:
        meta = aseprite_data.get('meta', {})
        source_image = meta.get('image')

        if source_image is None:
            json_path = Path(input_json)
            stem = json_path.stem
            for ext in ['.png', '.jpg', '.jpeg', '.webp']:
                for candidate in [
                    json_path.with_suffix(ext),
                    json_path.parent / f"{stem}_sheet{ext}",
                    json_path.parent / f"{stem}-sheet{ext}",
                    json_path.parent / f"{stem}_atlas{ext}",
                    json_path.parent / f"{stem}-atlas{ext}",
                ]:
                    if candidate.exists():
                        source_image = candidate.name
                        break
                if source_image:
                    break

    if not source_image:
        print(f"Warning: Could not determine source image path. Using default 'spritesheet.png'")
        source_image = "spritesheet.png"
    frames_raw = aseprite_data.get('frames', {})
    if isinstance(frames_raw, list):
        frames_dict = {f"frame_{i}": frame for i, frame in enumerate(frames_raw)}
    else:
        frames_dict = frames_raw
    sorted_frames = sort_frames(frames_dict)
    engine_frames = []
    for frame_name, frame_data in sorted_frames:
        frame = frame_data.get('frame', {})

        engine_frame = {
            'Width': frame.get('w', 0),
            'Height': frame.get('h', 0),
            'X': frame.get('x', 0),
            'Y': frame.get('y', 0),
            'Duration': frame_data.get('duration', 100)
        }

        engine_frames.append(engine_frame)
    engine_format = {
        'SourceImagePath': source_image,
        'FrameCount': len(engine_frames),
        'Frames': engine_frames
    }

    return engine_format


def main():
    parser = argparse.ArgumentParser(
        description='Convert Aseprite JSON sprite sheet to CE json animation format'
    )
    parser.add_argument(
        'input',
        type=Path,
        help='Input Aseprite JSON file'
    )
    parser.add_argument(
        '-o', '--output',
        type=Path,
        help='Output JSON file (default: input_name.engine.json)'
    )
    parser.add_argument(
        '--source-image',
        type=str,
        help='Path to the source sprite sheet image'
    )
    parser.add_argument(
        '--pretty',
        action='store_true',
        help='Pretty-print the output JSON'
    )

    args = parser.parse_args()

    if not args.input.exists():
        print(f"Error: Input file '{args.input}' not found.", file=sys.stderr)
        return 1

    if args.output is None:
        args.output = args.input.parent / f"{args.input.stem}.engine.json"
    try:
        engine_data = convert_aseprite_to_engine_format(args.input, args.source_image)

        with open(args.output, 'w', encoding='utf-8') as f:
            if args.pretty:
                json.dump(engine_data, f, indent=2)
            else:
                json.dump(engine_data, f)

        print(f"Successfully converted '{args.input}' to '{args.output}'")
        print(f"  - {engine_data['FrameCount']} frames")
        print(f"  - Source image: {engine_data['SourceImagePath']}")

    except Exception as e:
        print(f"Error converting file: {e}", file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
