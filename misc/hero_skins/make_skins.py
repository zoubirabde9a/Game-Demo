"""Class skins from the original hero's sheet (client/heroes/hero_skins.cpp).

Reads the original hero's texture (AssetType_Zoubir) out of asset_1.zas and
writes one sheet per class to web/heroes/<class>.png: the same 16x16 grid of
32-pixel cells in the same order, so the game plays the original's animation
table on it unchanged. Each cell is the original with
  - the long hair cut short: hair outside the head, or below the chin, is
    removed and the outline redrawn round the cut (the head is found from
    the headphones);
  - every colour mapped through the class palette (the original uses 19):
    hair, clothes, the trim, the headphones (hair on the head, the class's
    metal elsewhere), and the red of the eyes made a dark neutral.

usage: python -I make_skins.py <asset_1.zas> <out folder>
"""
import struct, sys, os
import numpy as np
from PIL import Image


def original_sheet(zas):
    """the original hero's RGBA pixels, rows as stored in the pack"""
    d = open(zas, 'rb').read()
    magic, ver, count, types, infos = struct.unpack_from('<IIIQQ', d, 0)
    first, last = struct.unpack_from('<II', d, types + 8 * 1)
    fam, off, ch, w, h, tags, ox, oy, nx, ny = struct.unpack_from('<IQIIIIffII', d, infos + 44 * first)
    assert ch == 4 and (w, h, nx, ny) == (512, 512, 16, 16), (ch, w, h, nx, ny)
    return np.frombuffer(d, np.uint8, w * h * 4, off).reshape(h, w, 4).copy()


OUTLINE = (29, 9, 16)
HAIR = [(6, 13, 11), (20, 28, 41), (45, 52, 61), (128, 130, 164)]      # dark -> light
CLOTH = [(30, 31, 20), (35, 40, 24), (61, 66, 52)]
METAL = [(128, 131, 122), (207, 208, 204)]
ACCENT = [(110, 14, 18), (204, 28, 41)]
SKIN = [(101, 29, 12), (199, 118, 134), (252, 207, 202)]


def is_col(cell, cols):
    m = np.zeros(cell.shape[:2], bool)
    for c in cols:
        m |= (cell[..., 0] == c[0]) & (cell[..., 1] == c[1]) & (cell[..., 2] == c[2]) & (cell[..., 3] > 0)
    return m


def sprite_top(cell):
    """the first row at least HEAD_ROW_PIXELS wide: the top of the head,
    under a hair bun or a tuft sticking up"""
    rows = np.nonzero((cell[..., 3] > 0).sum(1) >= HEAD_ROW_PIXELS)[0]
    return rows[0] if len(rows) else None


def is_streak(patch, top, head):
    """a skin-coloured patch that is the sword's slash streak: big and wide,
    mostly off the head and not reaching its crown (a face, a bald head,
    a hand are none of these)"""
    xs = [x for y, x in patch]
    ys = [y for y, x in patch]
    in_head = sum(1 for y, x in patch if head[y, x])
    return (len(patch) > SLASH_PIXELS and max(xs) - min(xs) + 1 >= SLASH_WIDTH and
            in_head < len(patch) // 2 and min(ys) > top + 2)


def head_of(cell):
    """(centre x, centre y) of the head, from the headphones and the top of
    the sprite; None for an empty cell"""
    top = sprite_top(cell)
    if top is None:
        return None
    op = cell[..., 3] > 0
    phones = is_col(cell, METAL)
    phones[top + 15:] = False
    if phones.sum() >= 3:
        py, px = np.nonzero(phones)
        cx = px.mean()
    else:
        hx = np.nonzero(op[top:top + 12].any(0))[0]
        cx = hx.mean()
    return cx, top + 7.5


def neighbours8(m):
    out = np.zeros_like(m)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            if dy or dx:
                out |= np.roll(np.roll(m, dy, 0), dx, 1)
    return out


def haircut(cell, rx=8.2, ry=8.2):
    cell = cell.copy()
    h = head_of(cell)
    if h is None:
        return cell
    cx, cy = h
    yy, xx = np.mgrid[0:32, 0:32]
    inside = ((xx + 0.5 - cx) / rx) ** 2 + ((yy + 0.5 - cy) / ry) ** 2 <= 1.0
    hair = is_col(cell, HAIR)
    # NOTE: short hair: nothing outside the head, nor below the chin
    cut = hair & (~inside | (yy > cy + 4.5))
    cell[cut] = 0
    # NOTE: outline left hanging where the hair was: drop outline pixels
    # that no longer touch the body, then outline the new edge
    body = (cell[..., 3] > 0) & ~is_col(cell, [OUTLINE])
    line = is_col(cell, [OUTLINE])
    cell[line & ~neighbours8(body)] = 0
    op = cell[..., 3] > 0
    edge = ~op & (np.roll(op, 1, 0) | np.roll(op, -1, 0) | np.roll(op, 1, 1) | np.roll(op, -1, 1))
    edge &= neighbours8(cut)
    cell[edge] = OUTLINE + (255,)
    return cell


def remap(cell, mapping):
    out = cell.copy()
    for src, dst in mapping.items():
        m = is_col(cell, [src])
        out[m, :3] = dst
    return out


# NOTE: per class, dark to light: hair (4 shades), clothes (3), metal (2),
# trim (2). The darkest shade of each sits on the sprite's edge, so it
# stays dark even for white or pale outfits, keeping the outline
P = {
 'firemage':   dict(hair=[(30,14,10),(66,32,20),(104,54,32),(150,90,56)], cloth=[(60,10,14),(96,18,22),(150,34,30)], metal=[(176,120,40),(240,200,90)], accent=[(170,70,10),(250,140,40)]),
 'bulwark':    dict(hair=[(28,18,12),(60,40,26),(96,68,44),(140,104,70)], cloth=[(48,54,66),(70,78,94),(128,138,156)], metal=[(170,178,192),(232,236,244)], accent=[(26,60,140),(70,130,230)]),
 'mender':     dict(hair=[(64,40,12),(132,90,30),(176,132,56),(222,190,110)], cloth=[(54,52,48),(150,148,140),(206,204,196)], metal=[(176,130,40),(244,210,110)], accent=[(30,120,60),(90,210,120)]),
 'ranger':     dict(hair=[(40,20,12),(84,44,24),(128,72,40),(176,112,70)], cloth=[(22,48,30),(34,74,44),(62,118,72)], metal=[(118,84,50),(176,136,90)], accent=[(20,110,90),(70,200,160)]),
 'berserker':  dict(hair=[(70,16,8),(130,36,16),(190,70,30),(236,130,70)], cloth=[(48,30,20),(72,46,30),(118,80,52)], metal=[(110,110,116),(180,182,188)], accent=[(120,16,16),(220,40,40)]),
 'shadowblade':dict(hair=[(8,6,12),(24,18,34),(44,34,62),(96,80,130)], cloth=[(22,16,30),(36,26,50),(66,50,90)], metal=[(90,90,110),(160,160,180)], accent=[(90,40,170),(170,110,255)]),
 'stormcaller':dict(hair=[(52,52,72),(120,120,142),(170,170,190),(222,222,236)], cloth=[(14,20,48),(24,36,84),(44,64,136)], metal=[(176,130,40),(244,210,110)], accent=[(190,150,20),(250,220,80)]),
 'duelist':    dict(hair=[(10,8,10),(30,22,26),(56,42,48),(110,90,96)], cloth=[(70,14,44),(120,30,76),(196,70,130)], metal=[(190,186,180),(250,248,244)], accent=[(170,50,110),(240,120,180)]),
 'frostmage':  dict(hair=[(44,62,92),(110,138,170),(160,186,214),(214,232,246)], cloth=[(34,56,92),(76,118,166),(136,182,220)], metal=[(180,196,210),(240,248,255)], accent=[(60,150,200),(150,220,255)]),
 'druid':      dict(hair=[(50,44,36),(90,80,66),(140,128,108),(196,186,166)], cloth=[(34,48,20),(56,76,30),(96,124,52)], metal=[(100,70,40),(156,116,72)], accent=[(80,130,30),(160,210,60)]),
}
EYE = [(40, 24, 30), (70, 44, 50)]
# NOTE: a man's skin: warm tan in place of the original's pinks, which
# also takes the blush and the lip colour away
MALE_SKIN = [(104, 58, 36), (190, 130, 92), (234, 186, 146)]
WHITE = (255, 255, 255)
# NOTE: a patch of skin colour bigger than this is the sword's slash
# streak, not a hand or a face
SLASH_PIXELS = 20
SLASH_WIDTH = 10
HEAD_ROW_PIXELS = 5
# NOTE: marker colours for the slash streak between bald_head and the
# palette; nothing in the original uses them
STREAK = [(1, 2, 3), (4, 5, 6), (7, 8, 9)]
# NOTE: marker for the bald head's shine, turned into SHINE_RGB
SHINE = (10, 11, 12)
SHINE_RGB = (252, 222, 196)


def components(mask):
    """lists of (y, x) for each 4-connected patch of mask"""
    seen = np.zeros_like(mask)
    out = []
    for y0, x0 in zip(*np.nonzero(mask)):
        if seen[y0, x0]:
            continue
        stack, patch = [(y0, x0)], []
        seen[y0, x0] = True
        while stack:
            y, x = stack.pop()
            patch.append((y, x))
            for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                ny, nx = y + dy, x + dx
                if 0 <= ny < 32 and 0 <= nx < 32 and mask[ny, nx] and not seen[ny, nx]:
                    seen[ny, nx] = True
                    stack.append((ny, nx))
        out.append(patch)
    return out


# NOTE: rows from the head's centre down to where the legs start, and how
# many columns the body gains down its middle from the neck on
WAIST_BELOW_HEAD = 8.5
NECK_BELOW_HEAD = 6.5
WIDEN = 2


def widen(cell):
    """a broader man: from the neck down, every row is split at the head's
    centre and each half pushed out a pixel, the middle columns repeated,
    so the shoulders, chest and stance widen while the head keeps its size"""
    h = head_of(cell)
    if h is None:
        return cell
    cx, cy = h
    c = int(round(cx))
    out = cell.copy()
    half = WIDEN // 2
    for y in range(int(cy + NECK_BELOW_HEAD), 32):
        row = cell[y]
        new = row.copy()
        for x in range(32):
            if x < c - half:
                src = x + half
            elif x >= c + half:
                src = x - half
            else:
                src = c - half if x < c else c
            new[x] = row[min(31, max(0, src))]
        out[y] = new
    return out


# NOTE: a bald head's skin, dark to light, for the hair's four shades:
# the darkest stays on the head's rim, the lightest is the shine on top
BALD = [(96, 52, 32), (176, 116, 80), (204, 144, 104), (244, 204, 168)]
EYE_WHITE = (244, 240, 232)


def reoutline(cell, removed):
    """after pixels were taken away: drop outline left touching nothing,
    then outline the new edge next to what was removed"""
    body = (cell[..., 3] > 0) & ~is_col(cell, [OUTLINE])
    line = is_col(cell, [OUTLINE])
    cell[line & ~neighbours8(body)] = 0
    op = cell[..., 3] > 0
    edge = ~op & (np.roll(op, 1, 0) | np.roll(op, -1, 0) | np.roll(op, 1, 1) | np.roll(op, -1, 1))
    edge &= neighbours8(removed)
    cell[edge] = OUTLINE + (255,)
    return cell


def trim_hips(cell):
    """no flared hips: in the band between the waist and the legs, cloth
    wider than the legs below it is taken off"""
    h = head_of(cell)
    if h is None:
        return cell
    cx, cy = h
    cell = cell.copy()
    op = cell[..., 3] > 0
    lo_y, hi_y = int(cy + WAIST_BELOW_HEAD), int(cy + WAIST_BELOW_HEAD + 4)
    legs = op[hi_y:min(32, hi_y + 3)]
    xs = np.nonzero(legs.any(0))[0]
    if len(xs) == 0:
        return cell
    left, right = xs.min() - 1, xs.max() + 1
    cloth = is_col(cell, CLOTH + ACCENT)
    removed = np.zeros((32, 32), bool)
    for y in range(max(0, lo_y), min(32, hi_y)):
        for x in range(32):
            if cloth[y, x] and (x < left or x > right):
                removed[y, x] = True
    cell[removed] = 0
    return reoutline(cell, removed)


def thicken_legs(cell):
    """sturdier legs: below the waist, every run of the body in a row grows
    a pixel on each side, its outline moving out with it"""
    h = head_of(cell)
    if h is None:
        return cell
    cx, cy = h
    out = cell.copy()
    for y in range(int(cy + WAIST_BELOW_HEAD + 2), 32):
        row = cell[y]
        op = row[:, 3] > 0
        x = 0
        while x < 32:
            if not op[x]:
                x += 1
                continue
            a = x
            while x < 32 and op[x]:
                x += 1
            b = x - 1
            if b - a >= 2:
                if a > 0 and not op[a - 1]:
                    out[y, a - 1] = row[a]
                    out[y, a] = row[a + 1]
                if b < 31 and not op[b + 1]:
                    out[y, b + 1] = row[b]
                    out[y, b] = row[b - 1]
    return out


# NOTE: the bald head, drawn in place of the hair: radii and how far
# below the hair's centre it sits
BALD_RX = 7.0
BALD_RY = 7.2
BALD_DROP = 1.0
BALD_FACE_LIFT = 2.5


def bald_head(cell):
    """takes the hair (and the headphones) away and draws a round bald head
    a size smaller in their place, shaded from the upper left, then puts
    the original face (skin, eyes, mouth) back on it"""
    h = head_of(cell)
    if h is None:
        return cell
    cx, cy = h
    cell = cell.copy()
    yy, xx = np.mgrid[0:32, 0:32]
    skin = is_col(cell, SKIN)
    top = sprite_top(cell)
    # NOTE: the sword's streak can cross the head; it stays
    streak = np.zeros((32, 32), bool)
    wide = ((xx + 0.5 - cx) / 10.5) ** 2 + ((yy + 0.5 - cy) / 9.5) ** 2 <= 1.0
    for patch in components(skin):
        if is_streak(patch, -99, wide):
            for y, x in patch:
                streak[y, x] = True
    # NOTE: the face is the topmost patch of skin; the head is found from
    # it, since a hair bun sticking up throws the sprite's top off
    face_patch = None
    for patch in components(skin & ~streak):
        if len(patch) >= 4 and min(y for y, x in patch) <= top + 18:
            if face_patch is None or min(y for y, x in patch) < min(y for y, x in face_patch):
                face_patch = patch
    if face_patch:
        fy = np.array([y for y, x in face_patch])
        fx = np.array([x for y, x in face_patch])
        cy = fy.mean() + 0.5 - BALD_FACE_LIFT - BALD_DROP
        neck = fy.max() + 1.5
        box = (yy >= fy.min() - 2) & (yy <= fy.max()) & (xx >= fx.min() - 2) & (xx <= fx.max() + 2)
        face = box & (skin | is_col(cell, ACCENT + [WHITE])) & ~streak
    else:
        neck = cy + NECK_BELOW_HEAD + 1
        face = np.zeros((32, 32), bool)
    old = ((xx + 0.5 - cx) / 10.5) ** 2 + ((yy + 0.5 - cy) / 9.5) ** 2 <= 1.0
    old &= yy + 0.5 < neck
    face &= yy + 0.5 < neck
    keep = cell.copy()
    gone = old & ~streak & (cell[..., 3] > 0)
    cell[gone] = 0
    # NOTE: the head sits where the face says, the face on its lower half;
    # seen from behind (no face) a little under the hair's centre
    by = cy + BALD_DROP
    bx = cx
    nx = (xx + 0.5 - bx) / BALD_RX
    ny = (yy + 0.5 - by) / BALD_RY
    dome = (nx ** 2 + ny ** 2 <= 1.0) & ~streak
    # NOTE: shaded so it reads the same from every side: the face's own
    # light tone all over, a thin shadow crescent on the lower right rim
    # and a small shine up on the left, never a dark half across the face
    r2 = nx ** 2 + ny ** 2
    cell[dome] = SKIN[2] + (255,)
    cell[dome & (r2 > 0.55) & (nx + ny > 0.45)] = SKIN[1] + (255,)
    shine = ((nx + 0.38) ** 2 + (ny + 0.5) ** 2) <= 0.06
    cell[dome & shine] = SHINE + (255,)
    # NOTE: the face goes back where it was, over the dome; no hair is left
    # anywhere (the long hair down the back, the ponytail)
    cell[face] = keep[face]
    hair = is_col(cell, HAIR) | (is_col(cell, METAL) & old)
    cell[hair] = 0
    cell = reoutline(cell, hair | gone)
    op = cell[..., 3] > 0
    edge = ~op & (np.roll(op, 1, 0) | np.roll(op, -1, 0) | np.roll(op, 1, 1) | np.roll(op, -1, 1))
    edge &= neighbours8(dome)
    cell[edge] = OUTLINE + (255,)
    # NOTE: the dome's own rim inside the old outline: darken its edge
    rim = dome & ~(np.roll(dome, 1, 0) & np.roll(dome, -1, 0) & np.roll(dome, 1, 1) & np.roll(dome, -1, 1))
    rim &= ~face
    cell[rim & (cell[..., 3] > 0)] = OUTLINE + (255,)
    # NOTE: the streak keeps marker colours from here on, so the new head
    # it may touch is never taken for it
    for src, mark in zip(SKIN, STREAK):
        cell[streak & is_col(keep, [src])] = mark + (255,)
    return cell


# NOTE: a detached bit this small is a left-over speck (a headphone's
# edge, a strand), not part of the hero
SPECK_PIXELS = 3


def despeck(cell):
    """removes tiny bits that touch nothing else, with their outline"""
    op = cell[..., 3] > 0
    parts = components(op)
    if len(parts) < 2:
        return cell
    biggest = max(len(p) for p in parts)
    out = cell.copy()
    for patch in parts:
        body = [(y, x) for y, x in patch if tuple(cell[y, x, :3]) != OUTLINE]
        if len(patch) < biggest and len(body) <= SPECK_PIXELS:
            for y, x in patch:
                out[y, x] = 0
    return out


def class_cell(cell, pal):
    cell = thicken_legs(trim_hips(widen(bald_head(cell))))
    h = head_of(cell)
    out = cell.copy()
    yy, xx = np.mgrid[0:32, 0:32]
    head = np.zeros((32, 32), bool)
    if h:
        cx, cy = h
        head = ((xx + 0.5 - cx) / 9.0) ** 2 + ((yy + 0.5 - cy) / 8.6) ** 2 <= 1.0
    def put(src, dst, mask=None):
        m = is_col(cell, [src])
        if mask is not None:
            m &= mask
        out[m, :3] = dst
    # NOTE: bald: the hair is skin, shaded like it
    for s, d in zip(HAIR, BALD):
        put(s, d)
    for s, d in zip(CLOTH, pal['cloth']):
        put(s, d)
    # NOTE: the headphones' cups become ears; elsewhere grey is the class
    # metal
    put(METAL[0], BALD[1], head)
    put(METAL[1], BALD[2], head)
    put(METAL[0], pal['metal'][0], ~head)
    put(METAL[1], pal['metal'][1], ~head)
    # NOTE: red in the face is the eyes
    put(ACCENT[0], EYE[0], head)
    put(ACCENT[1], EYE[1], head)
    put(ACCENT[0], pal['accent'][0], ~head)
    put(ACCENT[1], pal['accent'][1], ~head)
    # NOTE: a man's face: big plain eyes (the sparkle is an eye white), tan
    # skin, and the girl's ribbon tie under the chin becomes collar
    put(WHITE, EYE_WHITE, head)
    # NOTE: bigger eyes: each eye grows a row down, dark under its white
    # and under the pupil beside it (toward the face's middle)
    if h:
        whites = np.argwhere(is_col(out, [EYE_WHITE]) & head)
        skin_now = is_col(out, MALE_SKIN)
        for y, x in whites:
            inward = 1 if x < cx else -1
            for dx in (0, inward):
                yb, xb = y + 1, x + dx
                if 0 <= yb < 32 and 0 <= xb < 32 and skin_now[yb, xb]:
                    out[yb, xb, :3] = EYE[0]
    if h:
        neck = ~head & (np.abs(xx + 0.5 - cx) <= 2.5) & (yy + 0.5 >= cy + 7.5) & (yy + 0.5 <= cy + 14.)
        put(ACCENT[0], pal['cloth'][1], neck)
        put(ACCENT[1], pal['cloth'][2], neck)
    slash = is_col(cell, STREAK)
    put(SHINE, SHINE_RGB)
    for s, d in zip(SKIN, MALE_SKIN):
        put(s, d, ~slash)
    put(STREAK[0], pal['accent'][0])
    put(STREAK[1], pal['accent'][0])
    put(STREAK[2], pal['accent'][1])
    # NOTE: trousers, not a skirt over red stockings: below the waist the
    # red stripes, the hem and the bare thigh all become plain trousers
    if h:
        legs = (yy + 0.5 >= cy + WAIST_BELOW_HEAD) & ~slash
        pants = pal.get('pants', [pal['cloth'][0], pal['cloth'][1]])
        put(ACCENT[0], pants[0], legs)
        put(ACCENT[1], pants[1], legs)
        for s in SKIN:
            put(s, pants[1], legs)
    return despeck(out)


def class_sheet(sheet, pal):
    """every 32-pixel cell of the sheet, in place of the original's"""
    out = sheet.copy()
    for r in range(16):
        for c in range(16):
            out[r * 32:(r + 1) * 32, c * 32:(c + 1) * 32] = class_cell(sheet[r * 32:(r + 1) * 32, c * 32:(c + 1) * 32], pal)
    return out




if __name__ == '__main__':
    zas, out = sys.argv[1], sys.argv[2]
    sheet = original_sheet(zas)
    os.makedirs(out, exist_ok=True)
    for name, pal in P.items():
        Image.fromarray(class_sheet(sheet, pal)).save(os.path.join(out, f'{name}.png'), optimize=True)
        print(name)
