"""What a class wears over the bald head and the body (make_skins.py): a
beard, a cloak, or a keffiyeh held by an agal with the face left open.

Each is drawn on one 32-pixel cell from where the head and face are
(make_skins.bald_head works them out), in final colours that the palette
step leaves alone, and gets the original's dark outline.
"""
import numpy as np

OUTLINE = (29, 9, 16)


def outline_new(cell, new):
    """the dark outline round newly drawn pixels, where they meet nothing"""
    op = cell[..., 3] > 0
    near = np.zeros_like(new)
    for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        near |= np.roll(np.roll(new, dy, 0), dx, 1)
    cell[~op & near] = OUTLINE + (255,)
    return cell


def paint(cell, mask, colour):
    cell[mask] = tuple(colour) + (255,)


def facing_of(face, bx):
    """+1 facing right, -1 facing left, 0 toward the camera, None from
    behind (no face)"""
    if face.sum() == 0:
        return None
    fx = np.nonzero(face)[1].mean()
    if fx > bx + 1.2:
        return 1
    if fx < bx - 1.2:
        return -1
    return 0


def beard(cell, info, colours):
    """a full beard from under the eyes to the chest, the whole width of
    the face; from the side on the face's side only; none from behind"""
    face, bx = info['face'], info['bx']
    side = facing_of(face, bx)
    if side is None:
        return cell
    ys, xs = np.nonzero(face)
    eyes = info['eye_row']
    chin = ys.max()
    left, right = xs.min() - 1, xs.max() + 1
    if side > 0:
        left = int(round(bx))
    if side < 0:
        right = int(round(bx))
    area = np.zeros((32, 32), bool)
    for y in range(eyes + 2, chin + 4):
        if not 0 <= y < 32:
            continue
        taper = max(0, y - chin)
        lo, hi = left + taper, right - taper
        if side > 0:
            lo = left
        if side < 0:
            hi = right
        if lo <= hi:
            area[y, max(0, lo):min(32, hi + 1)] = True
    # NOTE: the eyes stay
    area &= ~np.isin(cell[..., 0], [244, 255])
    paint(cell, area, colours[1])
    # NOTE: lit along its top, dark along its bottom edge, a strand or two
    xx = np.mgrid[0:32, 0:32][1]
    top = area & ~np.roll(area, 1, 0)
    bottom = area & ~np.roll(area, -1, 0)
    paint(cell, area & (xx % 3 == 1) & ~top, colours[0])
    paint(cell, top & ~bottom, colours[2])
    paint(cell, bottom, colours[0])
    return outline_new(cell, area)


def cloak(cell, info, colours):
    """a cloak from the shoulders to the knees: over the back from behind,
    trailing behind the body from the side, showing past the arms from
    the front"""
    op = cell[..., 3] > 0
    rows = np.nonzero(op.any(1))[0]
    if len(rows) == 0:
        return cell
    feet = rows[-1]
    shoulder = int(info['by'] + info['ry'] + 0.5)
    bottom = feet - 4
    if bottom <= shoulder + 2:
        return cell
    span = np.nonzero(op[min(31, shoulder + 2)])[0]
    if len(span) == 0:
        return cell
    left, right = span.min(), span.max()
    side = facing_of(info['face'], info['bx'])
    new = np.zeros((32, 32), bool)
    for y in range(shoulder + 1, bottom + 1):
        flare = (y - shoulder) // 3
        if side is None:
            lo, hi = left - flare // 2, right + flare // 2
            new[y, max(0, lo):min(32, hi + 1)] = True
        elif side == 0:
            lo, hi = left - 1 - flare // 2, right + 1 + flare // 2
            row = np.zeros(32, bool)
            row[max(0, lo):min(32, hi + 1)] = True
            new[y] = row & ~op[y]
        else:
            body = np.nonzero(op[y])[0]
            if len(body) == 0:
                continue
            back = body.min() if side > 0 else body.max()
            reach = 1 + flare
            xs = range(back - reach, back) if side > 0 else range(back + 1, back + 1 + reach)
            for x in xs:
                if 0 <= x < 32 and not op[y, x]:
                    new[y, x] = True
    if not new.any():
        return cell
    paint(cell, new, colours[1])
    # NOTE: folds: every third column a shade darker, the hem darkest
    xx = np.mgrid[0:32, 0:32][1]
    paint(cell, new & (xx % 3 == 0), colours[0])
    hem = new & ~np.roll(new, -1, 0)
    paint(cell, hem, colours[0])
    lit = new & ~np.roll(new, 1, 0)
    paint(cell, lit, colours[2])
    return outline_new(cell, new)


def keffiyeh(cell, info, look):
    """a cloth over the head falling to the shoulders, the face open, held
    by a dark agal; a checked one has a sparse red grid"""
    dome, face, bx, by = info['dome'], info['face'], info['bx'], info['by']
    rx, ry = info['rx'], info['ry']
    side = facing_of(face, bx)
    yy, xx = np.mgrid[0:32, 0:32]
    shoulder = int(by + ry + 0.5)
    if face.any():
        fys, fxs = np.nonzero(face)
        opening = ((yy >= info['eye_row'] - 2) & (yy <= fys.max() + 1) &
                   (xx >= fxs.min() - 1) & (xx <= fxs.max() + 1))
    else:
        opening = np.zeros((32, 32), bool)
    drape = (yy >= by) & (yy <= shoulder) & (np.abs(xx + 0.5 - bx) <= rx + 0.5)
    if side:
        # NOTE: from the side the cloth falls behind the head, not over the face
        drape &= (xx + 0.5 - bx) * side <= 1
    cloth = (dome | drape) & ~opening
    paint(cell, cloth, look['cloth'][1])
    # NOTE: lit along the crown, a shade darker in the folds that fall
    paint(cell, cloth & ~np.roll(cloth, 1, 0), look['cloth'][2])
    paint(cell, cloth & (yy > by) & (xx % 3 == 0), look['cloth'][0])
    if look.get('check'):
        paint(cell, cloth & (xx % 3 == 1) & (yy % 3 == 1), look['check'])
    # NOTE: the agal: a dark cord round the crown, two rows
    band = int(round(by - ry * 0.45))
    for k, colour in enumerate(look['agal']):
        paint(cell, dome & (yy == band + k) & ~opening, colour)
    return outline_new(cell, cloth)


def draw_look(cell, info, look):
    """everything one class wears, in order: cloak behind, then the head"""
    if not look:
        return cell
    if 'cloak' in look:
        cell = cloak(cell, info, look['cloak'])
    if 'beard' in look:
        cell = beard(cell, info, look['beard'])
    if 'keffiyeh' in look:
        cell = keffiyeh(cell, info, look['keffiyeh'])
    return cell
