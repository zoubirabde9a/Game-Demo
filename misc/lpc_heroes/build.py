"""Compose LPC layers into per-class animation strips, a manifest, contact sheets and credits,
for the "Heroic" player skins (pack.py turns the strips into web/heroes/*_lpc.png).

Usage: python -I build.py <lpc_root>
  <lpc_root>/src/ulpc  = sparse clone of Universal-LPC-Spritesheet-Character-Generator (data only)
  <lpc_root>/out       = output
"""
import csv, json, os, sys
from PIL import Image, ImageDraw

ROOT = os.path.abspath(sys.argv[1])
REPO = os.path.join(ROOT, 'src', 'ulpc')
SS = os.path.join(REPO, 'spritesheets')
PALDIR = os.path.join(REPO, 'palette_definitions')
OUT = os.path.join(ROOT, 'out')

ROWS = {'up': 0, 'left': 1, 'down': 2, 'right': 3}
FACINGS = ['down', 'up', 'right', 'left']

# ---------------------------------------------------------------- palettes
_pal_cache = {}
def palette(material):
    if material not in _pal_cache:
        _pal_cache[material] = json.load(open(os.path.join(PALDIR, material, f'{material}_ulpc.json'), encoding='utf-8'))
    return _pal_cache[material]

def hex2rgb(h):
    h = h.lstrip('#'); return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))

def recolor(img, maps):
    """maps: list of (src_material, src_name, dst_material, dst_name). Match with tolerance 3 (generator uses 1; 3 catches the cape trim off-palette tone)."""
    pairs = []
    for sm, sn, dm, dn in maps:
        s = palette(sm)[sn]; d = palette(dm)[dn]
        n = min(len(s), len(d))
        pairs += [(hex2rgb(s[i]), hex2rgb(d[i])) for i in range(n)]
    img = img.convert('RGBA'); px = img.load()
    cache = {}
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = px[x, y]
            if a == 0: continue
            k = (r, g, b)
            if k not in cache:
                cache[k] = k
                for s, d in pairs:
                    if abs(r - s[0]) <= 3 and abs(g - s[1]) <= 3 and abs(b - s[2]) <= 3:
                        cache[k] = d; break
            px[x, y] = cache[k] + (a,)
    return img

# ---------------------------------------------------------------- layers
class Layer:
    def __init__(self, name, z, base, variant=None, recolor=None, oversize=None, optional=False,
                 transplant=False, sheetdef=None, files=None, optional_in=()):
        self.name, self.z, self.base, self.variant = name, z, base, variant
        self.recolor = recolor or []          # list of (src_mat, src_name, dst_mat, dst_name)
        self.oversize = oversize or {}        # lpc anim -> (relpath, frame size)
        self.optional = optional              # missing sheet just means nothing drawn (bg halves)
        self.optional_in = set(optional_in)   # LPC sheets where this layer may be absent (reported)
        self.transplant = transplant          # may borrow its walk frame 0 for idle
        self.sheetdef = sheetdef
        self.files = files or {}              # lpc anim -> explicit relpath (64px)
        self._cache = {}
        self.used = set()

    def resolve(self, anim):
        """Return (relpath, framesize) or None."""
        if anim in self.oversize:
            p, fs = self.oversize[anim]
            return (p, fs) if os.path.exists(os.path.join(SS, p)) else None
        if anim in self.files:
            p = self.files[anim]
            return (p, 64) if os.path.exists(os.path.join(SS, p)) else None
        cands = []
        if self.variant: cands.append(f'{self.base}/{anim}/{self.variant}.png')
        else: cands.append(f'{self.base}/{anim}.png')
        for c in cands:
            if os.path.exists(os.path.join(SS, c)): return (c, 64)
        return None

    def sheet(self, anim):
        r = self.resolve(anim)
        if r is None: return None
        if r[0] not in self._cache:
            im = Image.open(os.path.join(SS, r[0])).convert('RGBA')
            if self.recolor: im = recolor(im, self.recolor)
            self._cache[r[0]] = im
        self.used.add(r[0])
        return self._cache[r[0]], r[1]

def frame_of(layer, anim, row, col):
    s = layer.sheet(anim)
    if s is None: return None, None
    im, fs = s
    if (col + 1) * fs > im.width or (row + 1) * fs > im.height: return None, fs
    return im.crop((col * fs, row * fs, (col + 1) * fs, (row + 1) * fs)), fs

# ---------------------------------------------------------------- classes
def body(kind):
    return [Layer('body', 10, f'body/bodies/{kind}', sheetdef='body/body.json'),
            Layer('head', 100, f'head/heads/human/{kind}',
                  sheetdef=f'head/heads/human/heads_human_{kind}.json')]

def staff_simple():
    return [Layer('staff_fg', 140, None, files={a: f'weapon/magic/simple/foreground/{a}/simple.png'
                                                for a in ('walk', 'thrust', 'spellcast', 'hurt')},
                  transplant=True, sheetdef='weapons/magic/weapon_magic_simple.json'),
            Layer('staff_bg', 9, None, files={a: f'weapon/magic/simple/background/{a}/simple.png'
                                              for a in ('walk', 'thrust', 'spellcast', 'hurt')},
                  optional=True, transplant=True, sheetdef='weapons/magic/weapon_magic_simple.json')]

CLOTH = 'cloth'
def cloth(dst, src='white', dmat='cloth'): return [(CLOTH, src, dmat, dst)]

CLASSES = {}

CLASSES['firemage'] = dict(
    colour=(240, 120, 60),
    layers=body('male') + [
        Layer('hair', 120, 'hair/plain/adult', recolor=[('hair', 'orange', 'hair', 'redhead')],
              sheetdef='hair/short/hair_plain.json'),
        Layer('boots', 15, 'feet/boots/basic/male', recolor=cloth('brown'), sheetdef='feet/boots/feet_boots_basic.json'),
        Layer('robe_skirt', 20, 'legs/skirts/plain/male', recolor=cloth('red'),
              sheetdef='legs/skirts/legs_skirts_plain.json'),
        Layer('robe_top', 35, 'torso/clothes/longsleeve/longsleeve2/male', recolor=cloth('red'),
              sheetdef='torso/shirts/longsleeve/torso_clothes_longsleeve2.json'),
        Layer('wizard_hat', 130, 'hat/magic/wizard/base/adult', variant='red',
              sheetdef='headwear/hats/magic/hat_magic_wizard.json'),
        Layer('wizard_hat_band', 131, 'hat/magic/wizard/belt/adult', variant='orange',
              sheetdef='headwear/hats/magic/hat_magic_wizard_belt.json'),
    ] + staff_simple(),
    attack=('thrust', list(range(8)), 12),
)

CLASSES['bulwark'] = dict(
    colour=(96, 160, 245),
    layers=body('male') + [
        Layer('legs_plate', 20, 'legs/armour/plate/male', sheetdef='legs/legs_armour.json'),
        Layer('feet_plate', 15, 'feet/armour/plate/male', sheetdef='feet/feet_armour.json'),
        Layer('torso_plate', 60, 'torso/armour/plate/male', sheetdef='torso/armour/torso_armour_plate.json'),
        Layer('arms_plate', 60, 'arms/armour/plate/male', sheetdef='arms/arms_armour.json'),
        Layer('pauldrons', 61, 'shoulders/pauldrons/male', recolor=cloth('blue'),
              sheetdef='arms/shoulders/shoulders_pauldrons.json'),
        Layer('helmet', 130, 'hat/helmet/armet/adult', sheetdef='headwear/helmets/helmets/hat_helmet_armet.json'),
        Layer('shield_wood_bg', 2, 'shield/heater/revised/wood/bg', optional=True,
              sheetdef='weapons/shields/heater/shield_heater_revised_wood.json'),
        Layer('shield_paint_bg', 4, 'shield/heater/revised/paint/bg', optional=True, recolor=cloth('blue'),
              sheetdef='weapons/shields/heater/shield_heater_revised_paint.json'),
        Layer('shield_trim_bg', 6, 'shield/heater/revised/trim/bg', optional=True,
              sheetdef='weapons/shields/heater/shield_heater_revised_trim.json'),
        Layer('shield_wood', 110, 'shield/heater/revised/wood/fg',
              sheetdef='weapons/shields/heater/shield_heater_revised_wood.json'),
        Layer('shield_paint', 112, 'shield/heater/revised/paint/fg', recolor=cloth('blue'),
              sheetdef='weapons/shields/heater/shield_heater_revised_paint.json'),
        Layer('shield_trim', 115, 'shield/heater/revised/trim/fg',
              sheetdef='weapons/shields/heater/shield_heater_revised_trim.json'),
        Layer('sword_fg', 140, 'weapon/sword/arming/universal/fg', variant='steel', transplant=True,
              optional_in=('spellcast', 'jump'),
              oversize={'slash': ('weapon/sword/arming/attack_slash/fg/steel.png', 128)},
              sheetdef='weapons/sword/weapon_sword_arming.json'),
        Layer('sword_bg', 8, 'weapon/sword/arming/universal/bg', variant='steel', optional=True, transplant=True,
              oversize={'slash': ('weapon/sword/arming/attack_slash/bg/steel.png', 128)},
              sheetdef='weapons/sword/weapon_sword_arming.json'),
    ],
    attack=('slash', list(range(6)), 12),
)

CLASSES['mender'] = dict(
    colour=(120, 220, 140),
    layers=body('male') + [
        Layer('hair', 120, 'hair/long/adult', recolor=[('hair', 'orange', 'hair', 'blonde')],
              sheetdef='hair/long/hair_long.json'),
        Layer('slippers', 15, 'feet/slippers/male', recolor=cloth('tan'), sheetdef='feet/feet_slippers.json'),
        Layer('robe_skirt', 20, 'legs/skirts/plain/male', recolor=cloth('white'),
              sheetdef='legs/skirts/legs_skirts_plain.json'),
        Layer('robe_top', 35, 'torso/clothes/longsleeve/longsleeve2/male', recolor=cloth('white'),
              sheetdef='torso/shirts/longsleeve/torso_clothes_longsleeve2.json'),
        Layer('cape_bg', 5, 'cape/solid/bg', optional=True, recolor=cloth('green'),
              sheetdef='torso/cape/cape_solid.json'),
        Layer('cape', 85, 'cape/solid/fg', recolor=cloth('green'), sheetdef='torso/cape/cape_solid.json'),
        Layer('cape_trim', 90, 'cape/trim', recolor=[('cloth', 'brown', 'metal', 'gold')],
              sheetdef='torso/cape/cape_trim.json'),
        Layer('amulet', 95, 'neck/amulet/cross/male', variant='gold_yellow',
              sheetdef='headwear/neck/charms/neck_amulet_cross.json'),
    ] + staff_simple(),
    attack=('thrust', list(range(8)), 12),
)

# output animation -> (lpc sheet, frames, fps, loop, facings)
def anim_plan(cls):
    a = cls['attack']
    return {
        'idle':   ('idle', [0, 1], 4, True, FACINGS),
        'walk':   ('walk', list(range(1, 9)), 10, True, FACINGS),
        'run':    ('run', list(range(8)), 12, True, FACINGS),
        'attack': (a[0], a[1], a[2], False, FACINGS),
        'cast':   ('spellcast', list(range(7)), 10, False, FACINGS),
        'skid':   ('walk', [4, 3, 0], 8, False, FACINGS),
        'jump':   ('jump', list(range(5)), 10, False, FACINGS),
        'hurt':   ('hurt', [0, 1, 2], 8, False, ['down']),
        'death':  ('hurt', list(range(6)), 8, False, ['down']),
        'shoot':  ('shoot', list(range(13)), 12, False, FACINGS),
    }

# fallbacks when an LPC sheet is not drawn for every layer: list of (sheet, frame) pairs
FALLBACK = {
    'jump': [('spellcast', 1), ('walk', 0)],   # take-off (arms rising) and fall/land (standing)
}

def body_layer(layers): return next(l for l in layers if l.name == 'body')

def best_offset(a, b, r=3):
    """translation (dx,dy) that best maps image a onto b (alpha + rgb diff)."""
    import itertools
    pa, pb = a.load(), b.load(); W, H = a.size
    best = None
    for dx, dy in itertools.product(range(-r, r + 1), repeat=2):
        cost = 0
        for y in range(0, H):
            for x in range(0, W):
                xs, ys = x - dx, y - dy
                ca = pa[xs, ys] if 0 <= xs < W and 0 <= ys < H else (0, 0, 0, 0)
                cb = pb[x, y]
                if ca[3] == 0 and cb[3] == 0: continue
                if (ca[3] > 0) != (cb[3] > 0): cost += 3
                elif ca[:3] != cb[:3]: cost += 1
        if best is None or cost < best[0]: best = (cost, dx, dy)
    return best[1], best[2]

AUDIT = None
def compose(layers, picks, facing, notes):
    """picks: list of (sheet, col). Returns list of frames (RGBA) all of the same size."""
    row_for = lambda sheet: 0 if sheet == 'hurt' else ROWS[facing]
    # frame size = max over layers present
    fs = 64
    for sheet, col in picks:
        for l in layers:
            r = l.resolve(sheet)
            if r: fs = max(fs, r[1])
    frames = []
    for sheet, col in picks:
        row = row_for(sheet)
        canvas = Image.new('RGBA', (fs, fs), (0, 0, 0, 0))
        for l in sorted(layers, key=lambda l: l.z):
            im, lfs = frame_of(l, sheet, row, col)
            if im is None and l.transplant and sheet in ('idle',):
                w, _ = frame_of(l, 'walk', ROWS[facing], 0)
                if w is not None:
                    b = body_layer(layers)
                    bw, _ = frame_of(b, 'walk', ROWS[facing], 0)
                    bt, _ = frame_of(b, sheet, row, col)
                    dx, dy = best_offset(bw, bt)
                    im = Image.new('RGBA', (64, 64)); im.alpha_composite(w, (max(dx, 0), max(dy, 0)), (max(-dx, 0), max(-dy, 0)))
                    lfs = 64
                    notes.add(f'{l.name}: borrowed walk frame for {sheet}')
            if AUDIT is not None:
                AUDIT.setdefault(l.name, []).append(im is not None and im.getchannel('A').getbbox() is not None)
            if im is None:
                if sheet in l.optional_in and l.resolve(sheet) is None:
                    notes.add(f'{l.name}: not drawn in LPC {sheet} (no art for it)')
                continue
            off = (fs - lfs) // 2
            canvas.alpha_composite(im, (off, off))
        frames.append(canvas)
    return frames, fs

def covered(layers, sheet):
    missing = []
    for l in layers:
        if l.resolve(sheet) is None and not l.optional and sheet not in l.optional_in:
            if l.transplant and sheet == 'idle' and l.resolve('walk'): continue
            missing.append(l.name)
    return missing

def strip(frames):
    fs = frames[0].width
    s = Image.new('RGBA', (fs * len(frames), fs), (0, 0, 0, 0))
    for i, f in enumerate(frames): s.alpha_composite(f, (i * fs, 0))
    return s

def bbox_rows(img):
    bb = img.getchannel('A').getbbox(); return bb

def main():
    os.makedirs(OUT, exist_ok=True)
    manifest = {'frameSize': 64, 'feetY': None, 'bodyHeight': None, 'classes': {}}
    report = {}
    feet_rows, heights = [], []
    for cname, cls in CLASSES.items():
        layers = cls['layers']
        cdir = os.path.join(OUT, cname); os.makedirs(cdir, exist_ok=True)
        for f in os.listdir(cdir): os.remove(os.path.join(cdir, f))
        entries, rows_for_sheet, notes = {}, [], set()
        rep = report.setdefault(cname, {'skipped': {}, 'fallback': {}, 'notes': []})
        for anim, (sheet, cols, fps, loop, facings) in anim_plan(cls).items():
            miss = covered(layers, sheet)
            picks = [(sheet, c) for c in cols]
            if miss:
                if anim in FALLBACK and not any(covered(layers, s) for s, _ in FALLBACK[anim]):
                    picks = FALLBACK[anim]; fps = 4
                    rep['fallback'][anim] = f'LPC {sheet} missing for {", ".join(miss)}; used ' + \
                        ', '.join(f'{s} frame {c}' for s, c in picks)
                else:
                    rep['skipped'][anim] = f'LPC {sheet} sheet not available for: {", ".join(miss)}'
                    continue
            for facing in facings:
                global AUDIT
                AUDIT = {}
                frames, fs = compose(layers, picks, facing, notes)
                for ln, flags in AUDIT.items():
                    if not all(flags):
                        rep.setdefault('gaps', {}).setdefault(f'{anim}_{facing}', {})[ln] =                             'none' if not any(flags) else 'empty in frames ' + ','.join(str(i) for i, f in enumerate(flags) if not f)
                AUDIT = None
                st = strip(frames)
                st.save(os.path.join(cdir, f'{anim}_{facing}.png'))
                e = {'frames': len(frames), 'fps': fps, 'loop': loop}
                if fs != 64:
                    e['frameSize'] = fs
                    e['feetY'] = None  # filled below
                    e['offset'] = (fs - 64) // 2
                entries[f'{anim}_{facing}'] = e
                rows_for_sheet.append((f'{anim}_{facing}', st, fs))
        rep['notes'] = sorted(notes)
        manifest['classes'][cname] = entries
        # feet / height from walk_down standing frame (walk sheet col 0)
        f0, _ = compose([l for l in layers if l.name in ('body', 'head')], [('walk', 0)], 'down', set())
        bb = bbox_rows(f0[0]); feet_rows.append(bb[3] - 1); heights.append(bb[3] - bb[1])
        cls['_rows'] = rows_for_sheet
    feetY = max(set(feet_rows), key=feet_rows.count)
    manifest['feetY'] = feetY
    manifest['bodyHeight'] = max(heights)
    manifest['bodyHeightByClass'] = dict(zip(CLASSES.keys(), heights))
    for c in manifest['classes'].values():
        for e in c.values():
            if 'frameSize' in e: e['feetY'] = feetY + e['offset']
    json.dump(manifest, open(os.path.join(OUT, 'manifest.json'), 'w'), indent=2)
    # contact sheets
    for cname, cls in CLASSES.items():
        contact(cname, cls)
    credits()
    json.dump(report, open(os.path.join(ROOT, 'tmp', 'report.json'), 'w'), indent=2)
    print(json.dumps(report, indent=1))
    print('feetY', feetY, 'feet rows', feet_rows, 'heights', heights)

def contact(cname, cls, scale=3):
    rows = cls['_rows']
    label_w = 120
    W = label_w + max(st.width for _, st, _ in rows)
    H = sum(fs + 6 for _, _, fs in rows) + 30
    img = Image.new('RGBA', (W, H), (70, 150, 70, 255))
    d = ImageDraw.Draw(img)
    d.text((4, 4), f'{cname}  (scaled {scale}x)', fill=(255, 255, 255, 255))
    y = 26
    for name, st, fs in rows:
        d.text((4, y + fs // 2 - 6), name, fill=(255, 255, 255, 255))
        # frame borders
        for i in range(st.width // fs):
            d.rectangle((label_w + i * fs, y, label_w + (i + 1) * fs - 1, y + fs - 1), outline=(60, 130, 60, 255))
        img.alpha_composite(st, (label_w, y))
        y += fs + 6
    img = img.resize((W * scale, H * scale), Image.NEAREST)
    img.save(os.path.join(OUT, f'{cname}_sheet.png'))

def credits():
    rows = {}
    with open(os.path.join(REPO, 'CREDITS.csv'), encoding='utf-8') as fh:
        for r in csv.DictReader(fh, skipinitialspace=True):
            rows[r['filename'].strip()] = r
    lines = ['Credits for the LPC art used in lpc/out/ (preview build).',
             'Source: https://github.com/LiberatedPixelCup/Universal-LPC-Spritesheet-Character-Generator',
             'Paths are relative to spritesheets/ in that repository. Authors and licences are copied from its CREDITS.csv.',
             'Modifications: layers were composited, some recoloured with the repo palettes, and cut into per-animation strips.',
             '']
    lic_all, auth_all = set(), set()
    for cname, cls in CLASSES.items():
        lines.append(f'== {cname}')
        for l in cls['layers']:
            for p in sorted(l.used):
                r = rows.get(p)
                src = 'CREDITS.csv'
                if r is None and l.sheetdef:
                    # variant files are credited in the sheet definition by folder prefix
                    d = json.load(open(os.path.join(REPO, 'sheet_definitions', l.sheetdef), encoding='utf-8'))
                    best = None
                    for c in d.get('credits', []):
                        f = c['file'].rstrip('/')
                        if (p == f or p.startswith(f + '/')) and (best is None or len(f) > len(best['file'])): best = c
                    if best:
                        r = {'authors': ','.join(best['authors']), 'licenses': ','.join(best['licenses']),
                             'urls': ','.join(best.get('urls', []))}
                        src = f'sheet_definitions/{l.sheetdef} (entry {best["file"]})'
                if r is None:
                    lines.append(f'{p} | layer {l.name} | NO CREDIT FOUND'); continue
                a = r['authors'].strip(); li = r['licenses'].strip()
                auth_all.update(x.strip() for x in a.split(',')); lic_all.update(x.strip() for x in li.split(','))
                lines.append(f'{p} | layer {l.name} | authors: {a} | licences: {li} | urls: {r["urls"].strip()} | credit source: {src}')
        lines.append('')
    lines += ['== Summary', 'Licences involved: ' + ', '.join(sorted(lic_all)),
              'All authors to credit: ' + ', '.join(sorted(auth_all, key=str.lower))]
    open(os.path.join(ROOT, 'CREDITS.txt'), 'w', encoding='utf-8').write('\n'.join(lines) + '\n')

if __name__ == '__main__':
    main()
