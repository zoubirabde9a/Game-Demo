"""The new hero look: draws every class's sheet into web/heroes_v2/<class>.png.

A sheet is 16 by 16 cells of 32 pixels, like the classic one, but laid out
for the new animations (client/heroes/hero_looks.cpp reads it):
  per facing d (0 front, 1 right side, 2 back), animation slot s sits in
  png row d*5 + s//2, columns (s%2)*8 + frame, slots in poses.ANIMS order;
  png row 15 holds the single frames at column d*5 + k (poses.SINGLES).
Facing left is the right side mirrored by the game.

usage: py -I draw_heroes.py <out folder> [--preview <png>]
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import numpy as np
from PIL import Image

import body
import looks
import poses

VIEWS = ['down', 'right', 'up']
CELL = 32


def sheet(look):
    out = np.zeros((16 * CELL, 16 * CELL, 4), np.uint8)
    for d, view in enumerate(VIEWS):
        for s, (anim, count) in enumerate(poses.ANIMS):
            frames = poses.frames(look, anim, view)
            assert len(frames) == count, (anim, view, len(frames))
            row = d * 5 + s // 2
            for f, pose in enumerate(frames):
                col = (s % 2) * 8 + f
                out[row * CELL:(row + 1) * CELL, col * CELL:(col + 1) * CELL] = body.draw(look, pose, view)
        for k, anim in enumerate(poses.SINGLES):
            pose = poses.frames(look, anim, view)[0]
            col = d * 5 + k
            out[15 * CELL:16 * CELL, col * CELL:(col + 1) * CELL] = body.draw(look, pose, view)
    return out


def preview(sheets, path, scale=3):
    """every class side by side on grass, one block per facing"""
    names = list(sheets)
    w = 16 * CELL
    img = Image.new('RGBA', (w * scale, len(names) * 16 * CELL * scale // 1), (86, 112, 74, 255))
    for i, name in enumerate(names):
        s = Image.fromarray(sheets[name], 'RGBA').resize((w * scale, w * scale), Image.NEAREST)
        img.alpha_composite(s, (0, i * w * scale))
    img.save(path)


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    sheets = {}
    for name in looks.ORDER:
        sheets[name] = sheet(looks.LOOKS[name])
        Image.fromarray(sheets[name], 'RGBA').save(os.path.join(out, name + '.png'), optimize=True)
    if '--preview' in sys.argv:
        preview(sheets, sys.argv[sys.argv.index('--preview') + 1])


if __name__ == '__main__':
    main()
