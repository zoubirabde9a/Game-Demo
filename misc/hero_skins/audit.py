"""Sprite audit: scores each class sheet in web/heroes against the original
hero, so a sheet is judged by numbers rather than by eye.

Each metric is measured the same way on the original (read out of
asset_1.zas) and on a class sheet, over the frames the game plays (the
original's animation table, sim/animations.cpp), then turned into a 0..100
closeness score:
  height, width     the body on screen, in world units
  px_scale          world units one art pixel covers (chunkiness/crispness)
  soft_edges        share of half-transparent pixels
  speckle           share of lone pixels unlike all four neighbours (noise)
  colours           distinct colours per frame
  outline           share of the silhouette's edge that is dark
  contrast          spread of brightness inside the body
  walk_change       share of the body that changes between walk frames
  feet_wobble       how far the feet travel in the walk, world units
  side_lopsided     how unlike its own mirror the side view is
  front_vs_back     how unlike the front and back views are
Hard gates: height within 10% and px_scale within 20% of the original.
Exits 1 when a sheet fails a gate or scores under MIN_SCORE.

usage: python -I audit.py <asset_1.zas> <web/heroes folder>
"""
import os, sys
import numpy as np
from PIL import Image
from make_skins import original_sheet, P

MIN_SCORE = 85
FACINGS = ['down', 'up', 'right']
# NOTE: first frame and count per animation and facing (sim/animations.cpp)
TABLE = {
    'idle': ({'down': 177, 'up': 241, 'right': 209}, 1),
    'walk': ({'down': 180, 'up': 244, 'right': 212}, 4),
    'attack': ({'down': 32, 'up': 80, 'right': 64}, 6),
    'cast': ({'down': 96, 'up': 160, 'right': 128}, 4),
    'skid': ({'down': 184, 'up': 248, 'right': 216}, 4),
    'jump': ({'down': 103, 'up': 167, 'right': 135}, 2),
}


def sheet_frames(sheet):
    """the played frames of a 16x16 sheet of 32-pixel cells, drawn at 48
    world units; the game counts the sheet's rows from the bottom"""
    def cell(i):
        c, r = i % 16, 15 - i // 16
        return sheet[r * 32:(r + 1) * 32, c * 32:(c + 1) * 32]
    frames = {(a, f): [cell(s + k) for k in range(n)]
              for a, (starts, n) in TABLE.items() for f, s in starts.items()}
    return {'frames': frames, 'scale': 48 / 32}


# ---------------------------------------------------------------- metrics
def alpha(fr):
    return fr[..., 3]


def opaque(fr):
    return alpha(fr) > 127


def bbox(fr):
    m = opaque(fr)
    ys, xs = np.nonzero(m)
    if len(ys) == 0:
        return None
    return xs.min(), ys.min(), xs.max() + 1, ys.max() + 1


def luma(fr):
    c = fr[..., :3].astype(float)
    return 0.299 * c[..., 0] + 0.587 * c[..., 1] + 0.114 * c[..., 2]


def body_size(sk):
    """median opaque height and width in world units, front idle and walk"""
    hs, ws = [], []
    for key in [('idle', 'down'), ('walk', 'down')]:
        for fr in sk['frames'].get(key, []):
            b = bbox(fr)
            if b:
                hs.append((b[3] - b[1]) * sk['scale'])
                ws.append((b[2] - b[0]) * sk['scale'])
    return float(np.median(hs)), float(np.median(ws))


def all_frames(sk):
    return [fr for frs in sk['frames'].values() for fr in frs]


def semi_alpha(sk):
    """share of drawn pixels that are partly transparent (soft edges)"""
    tot = semi = 0
    for fr in all_frames(sk):
        a = alpha(fr)
        tot += (a > 0).sum()
        semi += ((a > 0) & (a < 250)).sum()
    return semi / max(tot, 1)


def speckle(sk):
    """share of opaque pixels whose colour differs from all 4 neighbours
    (lone pixels: dithering noise or lost detail)"""
    tot = lone = 0
    for fr in all_frames(sk):
        m = opaque(fr)
        c = fr[..., :3].astype(int)
        key = c[..., 0] * 65536 + c[..., 1] * 256 + c[..., 2]
        diff = np.ones_like(m)
        for dy, dx in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
            sh = np.roll(np.roll(key, dy, 0), dx, 1)
            diff &= (sh != key)
        inner = m & np.roll(m, 1, 0) & np.roll(m, -1, 0) & np.roll(m, 1, 1) & np.roll(m, -1, 1)
        tot += inner.sum()
        lone += (diff & inner).sum()
    return lone / max(tot, 1)


def colours(sk):
    """median distinct colours per frame"""
    counts = []
    for fr in all_frames(sk):
        m = opaque(fr)
        if m.sum():
            counts.append(len(np.unique(fr[..., :3][m].reshape(-1, 3), axis=0)))
    return float(np.median(counts))


def outline(sk):
    """share of the silhouette's edge pixels that are dark"""
    tot = dark = 0
    for fr in all_frames(sk):
        m = opaque(fr)
        edge = m & ~(np.roll(m, 1, 0) & np.roll(m, -1, 0) & np.roll(m, 1, 1) & np.roll(m, -1, 1))
        tot += edge.sum()
        dark += (edge & (luma(fr) < 70)).sum()
    return dark / max(tot, 1)


def contrast(sk):
    vals = []
    for fr in all_frames(sk):
        m = opaque(fr)
        if m.sum() > 20:
            vals.append(luma(fr)[m].std())
    return float(np.median(vals))


def motion(sk):
    """walk: share of the body's pixels that change between frames, and how
    much the feet (lowest opaque row) move, in world units"""
    ch, feet = [], []
    for f in FACINGS:
        frs = sk['frames'].get(('walk', f), [])
        for i in range(len(frs)):
            a, b = frs[i], frs[(i + 1) % len(frs)]
            ma, mb = opaque(a), opaque(b)
            union = (ma | mb).sum()
            if union:
                ch.append((ma ^ mb).sum() / union)
            bb = bbox(a)
            if bb:
                feet.append(bb[3] * sk['scale'])
    return float(np.mean(ch)), float(np.std(feet))


def side_read(sk):
    """how far the side view is from its own mirror, against how far the
    front is from its mirror: a side view that reads as a side view is
    lopsided. Also front vs back: they should not look alike"""
    def first(a, f):
        frs = sk['frames'].get((a, f))
        return frs[0] if frs else None

    def d(x, y):
        mx, my = opaque(x), opaque(y)
        return (mx ^ my).sum() / max((mx | my).sum(), 1)
    side, front, back = first('idle', 'right'), first('idle', 'down'), first('idle', 'up')
    lop = d(side, side[:, ::-1])
    fb = np.abs(luma(front) - luma(back))[opaque(front) & opaque(back)].mean() / 255
    return lop, fb


def measure(sk):
    h, w = body_size(sk)
    mo, feet = motion(sk)
    lop, fb = side_read(sk)
    return {
        'height': h, 'width': w, 'px_scale': sk['scale'],
        'soft_edges': semi_alpha(sk), 'speckle': speckle(sk), 'colours': colours(sk),
        'outline': outline(sk), 'contrast': contrast(sk),
        'walk_change': mo, 'feet_wobble': feet, 'side_lopsided': lop, 'front_vs_back': fb,
        'walk_frames': len(sk['frames'].get(('walk', 'down'), [])),
    }


# ---------------------------------------------------------------- scoring
def closeness(v, ref, tol):
    """100 at the original's value, 0 at tol (relative) away"""
    if ref == 0:
        return max(0.0, 100 * (1 - abs(v) / tol))
    return max(0.0, 100 * (1 - abs(v / ref - 1) / tol))


WEIGHTS = {  # metric: (weight, tolerance as a share of the original's value)
    'height': (3, 0.25), 'width': (1, 0.4), 'px_scale': (3, 0.5),
    'soft_edges': (1, 1.0), 'speckle': (2, 1.0), 'colours': (1, 1.0),
    'outline': (2, 0.6), 'contrast': (1, 0.6),
    'walk_change': (2, 0.8), 'feet_wobble': (1, 2.0),
    'side_lopsided': (2, 0.8), 'front_vs_back': (1, 0.8),
}


def score(m, ref):
    parts = {k: closeness(m[k], ref[k], t) for k, (wgt, t) in WEIGHTS.items()}
    total = sum(parts[k] * WEIGHTS[k][0] for k in parts) / sum(w for w, _ in WEIGHTS.values())
    gates = {
        'size': abs(m['height'] / ref['height'] - 1) <= 0.10,
        'pixel_scale': abs(m['px_scale'] / ref['px_scale'] - 1) <= 0.20,
    }
    return total, parts, gates


if __name__ == '__main__':
    zas, folder = sys.argv[1], sys.argv[2]
    ref = measure(sheet_frames(original_sheet(zas)))
    keys = list(WEIGHTS)
    print(f"{'sheet':12}{'score':>6}  gates  " + ' '.join(f'{k[:9]:>9}' for k in keys))
    print(f"{'original':12}{100:6.0f}  ok ok  " + ' '.join(f'{ref[k]:9.3f}' for k in keys))
    failed = []
    for name in P:
        sheet = np.asarray(Image.open(os.path.join(folder, name + '.png')).convert('RGBA'))
        m = measure(sheet_frames(sheet))
        total, parts, gates = score(m, ref)
        ok = all(gates.values()) and total >= MIN_SCORE
        if not ok:
            failed.append(name)
        g = ' '.join('ok' if v else 'NO' for v in gates.values())
        print(f'{name:12}{total:6.0f}  {g}  ' + ' '.join(f'{m[k]:9.3f}' for k in keys))
    print('all sheets pass' if not failed else 'below the bar: ' + ', '.join(failed))
    sys.exit(1 if failed else 0)
