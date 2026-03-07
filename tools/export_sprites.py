"""
Export sprites from sprites_data.json (produced by extract_sprites.js) to PNG files.
Each sprite is written to assets/sprites/<name>.png with a transparent background.

Usage:
    node tools/extract_sprites.js   # builds tools/sprites_data.json
    python tools/export_sprites.py  # converts to PNGs
"""
import json
import os
from PIL import Image

JSON_FILE = os.path.join(os.path.dirname(__file__), 'sprites_data.json')
OUT_DIR   = os.path.join(os.path.dirname(__file__), '..', 'assets', 'sprites')

with open(JSON_FILE, 'r', encoding='utf-8') as f:
    sprites = json.load(f)

os.makedirs(OUT_DIR, exist_ok=True)

def hex_to_rgba(h: str) -> tuple[int, int, int, int]:
    h = h.lstrip('#')
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), 255)

for spr in sprites:
    name = spr['name']
    w, h = spr['w'], spr['h']

    img   = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    pxbuf = img.load()

    for (x, y, color) in spr['pixels']:
        if 0 <= x < w and 0 <= y < h:
            pxbuf[x, y] = hex_to_rgba(color)

    out_path = os.path.join(OUT_DIR, f'{name}.png')
    img.save(out_path)
    print(f'  {name}: {w}x{h}  ({len(spr["pixels"])} px)')

print(f'\nDone — {len(sprites)} PNGs saved to {os.path.abspath(OUT_DIR)}')
