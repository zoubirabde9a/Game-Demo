"""Packs the LPC hero strips (build.py's out/ folder) into the sheets the
game loads from heroes/ (client/heroes/lpc_sheets.cpp reads this layout).

Usage: python -I pack.py <lpc_root> <web/heroes folder>

Each class gets <class>_lpc.png: 64-pixel cells, 8 columns, one row per
animation and facing in HERO_LPC_ANIMS order, facings down, up, right,
left, except hurt and death which are drawn facing down only. An attack
drawn in 128-pixel frames (a sword reaching past the cell) goes into
<class>_lpc_wide.png instead, four rows of 128-pixel cells, and its rows
in the main sheet stay empty. Frame counts are checked against the
table the game uses, so a rebuild that changes them fails here first.
"""
import json, os, sys
from PIL import Image

ROOT, OUT = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
ANIMS = ['idle', 'walk', 'attack', 'cast', 'skid', 'jump', 'hurt', 'death']
FACINGS = ['down', 'up', 'right', 'left']
FRONT_ONLY = {'hurt', 'death'}
# NOTE: must match HeroLpcFrames in code/client/heroes/lpc_sheets.cpp
FRAMES = {
    'firemage':    [2, 8, 8, 7, 3, 2, 3, 6],
    'bulwark':     [2, 8, 6, 7, 3, 5, 3, 6],
    'mender':      [2, 8, 8, 7, 3, 2, 3, 6],
    'ranger':      [2, 8, 6, 7, 3, 5, 3, 6],
    'berserker':   [2, 8, 6, 7, 3, 5, 3, 6],
    'shadowblade': [2, 8, 6, 7, 3, 5, 3, 6],
    'stormcaller': [2, 8, 8, 7, 3, 2, 3, 6],
    'duelist':     [2, 8, 6, 7, 3, 5, 3, 6],
    'frostmage':   [2, 8, 8, 7, 3, 2, 3, 6],
    'druid':       [2, 8, 8, 7, 3, 2, 3, 6],
}
CELL, COLUMNS = 64, 8

man = json.load(open(os.path.join(ROOT, 'out', 'manifest.json')))
os.makedirs(OUT, exist_ok=True)
for cls, counts in FRAMES.items():
    anims = man['classes'][cls]
    rows = sum(1 if a in FRONT_ONLY else len(FACINGS) for a in ANIMS)
    sheet = Image.new('RGBA', (CELL * COLUMNS, CELL * rows))
    wide = None
    row = 0
    for a, count in zip(ANIMS, counts):
        for f in (['down'] if a in FRONT_ONLY else FACINGS):
            info = anims[f'{a}_{f}']
            assert info['frames'] == count, (cls, a, f, info['frames'], count)
            strip = Image.open(os.path.join(ROOT, 'out', cls, f'{a}_{f}.png')).convert('RGBA')
            size = info.get('frameSize', CELL)
            if size == CELL:
                for k in range(count):
                    sheet.paste(strip.crop((k * CELL, 0, k * CELL + CELL, CELL)), (k * CELL, row * CELL))
            else:
                assert a == 'attack' and size == 2 * CELL, (cls, a, size)
                if wide is None:
                    wide = Image.new('RGBA', (size * count, size * len(FACINGS)))
                wide.paste(strip, (0, FACINGS.index(f) * size))
            row += 1
    sheet.save(os.path.join(OUT, f'{cls}_lpc.png'), optimize=True)
    if wide is not None:
        wide.save(os.path.join(OUT, f'{cls}_lpc_wide.png'), optimize=True)
    print(cls, sheet.size, wide.size if wide else '')
# NOTE: CREDITS.txt in OUT is build.py's credits with a header saying
# what the files are for; refresh its body from <lpc_root>/CREDITS.txt by hand
