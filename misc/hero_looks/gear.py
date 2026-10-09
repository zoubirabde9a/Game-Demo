"""What sets the classes apart: heads and headgear, capes, torso trims,
weapons, off-hand items and spell effects, each drawn for one view
('down' front, 'right' side, 'up' back). Colours come from looks.py.
"""
import math
import numpy as np
from canvas import (m_ellipse, m_capsule, m_poly, m_rect, m_line, shift, SIZE)

EYE = (26, 16, 30, 255)


def polar(x, y, ang, length):
    a = math.radians(ang)
    return x + math.cos(a) * length, y + math.sin(a) * length


def dilate(m):
    return m | shift(m, 1, 0) | shift(m, -1, 0) | shift(m, 0, 1) | shift(m, 0, -1)


def px(x, y):
    m = np.zeros((SIZE, SIZE), bool)
    x, y = int(math.floor(x)), int(math.floor(y))
    if 0 <= x < SIZE and 0 <= y < SIZE:
        m[y, x] = True
    return m


# ---------------------------------------------------------------- heads

def face_mask(view, cx, cy):
    if view == 'down':
        return m_ellipse(cx, cy + 2.0, 5.4, 4.6)
    if view == 'right':
        return m_ellipse(cx + 3.4, cy + 2.0, 4.0, 4.6)
    return np.zeros((SIZE, SIZE), bool)


def eyes(fr, view, cx, cy, p, colour=EYE, glint=None):
    ex = [cx - 3, cx + 2] if view == 'down' else ([cx + 4] if view == 'right' else [])
    ey = int(math.floor(cy + 1.0))
    for x in ex:
        if p.eyes == 'shut':
            fr.paint(px(x, ey + 1) | px(x + (1 if view == 'down' and x > cx else -1), ey + 1), colour)
        else:
            fr.paint(px(x, ey) | px(x, ey + 1), colour)
            if glint:
                fr.paint(px(x, ey), glint)


def head(fr, look, view, cx, cy, p):
    kind = look['head']
    skull = m_ellipse(cx, cy, 7.0, 6.4)
    face = face_mask(view, cx, cy) & skull
    fr.part(skull, look['skin'], shade='round', cx=cx - 1, cy=cy - 1, rx=7, ry=6.4)
    HEADS[kind](fr, look, view, cx, cy, p, skull, face)


def _hair(fr, look, skull, face, view, cx, cy):
    hair = skull & ~face
    if view == 'down':   # fringe: the face's top edge is hair
        hair |= skull & m_rect(0, 0, SIZE, int(cy - 0.5))
    fr.part(hair, look['hair'], shade='round', cx=cx - 1, cy=cy - 2, rx=7, ry=6.4)


def _hood_shape(view, cx, cy, peak):
    hood = m_ellipse(cx, cy - 0.2, 7.7, 7.0)
    lean = {'down': 0, 'right': -3, 'up': 0}[view]
    if peak:
        hood |= m_poly([(cx - 3, cy - 5), (cx + 3, cy - 5), (cx + lean + 0.5, cy - peak)])
    return hood


def _opening(view, cx, cy, big=False):
    if view == 'down':
        return m_ellipse(cx, cy + 2.0, 5.0 if big else 4.6, 4.4)
    if view == 'right':
        return m_ellipse(cx + 4.2, cy + 2.0, 3.6, 4.3)
    return np.zeros((SIZE, SIZE), bool)


def _mantle(fr, look, view, cx, cy):
    rmp = look.get('hood', look['body'])
    if view == 'right':
        m = m_poly([(cx - 5, cy + 4), (cx + 3, cy + 4), (cx + 4, cy + 9), (cx - 6, cy + 9)])
    else:
        m = m_poly([(cx - 6, cy + 4), (cx + 6, cy + 4), (cx + 7.6, cy + 9.4), (cx - 7.6, cy + 9.4)])
    fr.part(m, rmp)


def head_hood(fr, look, view, cx, cy, p, skull, face):
    rmp = look['hood']
    if look.get('mantle'):
        _mantle(fr, look, view, cx, cy)
    hood = _hood_shape(view, cx, cy, look.get('peak', 9))
    fr.part(hood, rmp, shade='round', cx=cx - 1, cy=cy - 2, rx=7.7, ry=7)
    hole = _opening(view, cx, cy) & hood
    if not hole.any():
        return
    if look.get('masked'):
        # shadowed face, glowing eyes, a scarf over the mouth
        fr.part(hole, look['shadow'], tone=1, inner=False)
        eyes(fr, view, cx, cy, p, colour=look['eyeglow'][2], glint=look['eyeglow'][3])
        mask = hole & m_rect(0, int(cy + 3), SIZE, SIZE)
        fr.part(mask, look['scarf'], tone=1, inner=False)
        fr.paint(hole & m_rect(0, int(cy + 3), SIZE, int(cy + 3)), look['scarf'][2])
    else:
        fr.part(hole, look['skin'], tone=1, inner=False)
        fr.paint(hole & m_rect(0, 0, SIZE, int(cy)), look['skin'][0])   # hood shadow on brow
        eyes(fr, view, cx, cy, p)
    if look.get('hood_trim'):
        ring = dilate(hole) & ~hole & hood
        fr.paint(ring, look['trim'][1])
    if look.get('fur'):
        ring = dilate(dilate(hole)) & ~hole & hood
        fr.part(ring, look['fur'], inner=True)
    if look.get('emblem') and view == 'down':
        fr.paint(px(cx - 0.5, cy - 4) | px(cx - 1, cy - 3) | px(cx - 0.5, cy - 3) |
                 px(cx - 0.5, cy - 2), look['emblem'])
    if look.get('emblem') and view == 'right':
        fr.paint(px(cx + 3, cy - 4) | px(cx + 2, cy - 3) | px(cx + 3, cy - 3), look['emblem'])


def head_wizard(fr, look, view, cx, cy, p, skull, face):
    _hair(fr, look, skull, face, view, cx, cy)
    if view != 'up':
        eyes(fr, view, cx, cy, p)
    tipx = {'down': cx + 5, 'right': cx - 4, 'up': cx - 5}[view]
    crown = m_poly([(cx - 5.2, cy - 3), (cx + 5.2, cy - 3), (cx + 1.5, cy - 8.5), (cx - 1.5, cy - 8.5)])
    crown |= m_capsule(cx, cy - 8, tipx, cy - 9.2, 1.1)
    fr.part(crown, look['hat'])
    band = crown & m_rect(0, int(cy - 4), SIZE, int(cy - 4))
    fr.paint(band, look['trim'][2])
    brim = m_ellipse(cx + (1 if view == 'right' else 0), cy - 2.6, 9.0, 2.1)
    fr.part(brim, look['hat'])


def head_helm(fr, look, view, cx, cy, p, skull, face):
    helm = m_ellipse(cx, cy - 0.2, 7.4, 6.8)
    fr.part(helm, look['metal'], shade='round', cx=cx - 1.5, cy=cy - 2, rx=7.4, ry=6.8)
    # plume
    if view == 'right':
        plume = m_capsule(cx - 1, cy - 6.5, cx - 6, cy - 5, 1.5)
    else:
        plume = m_capsule(cx - 0.0, cy - 6.5, cx - 0.0, cy - 8.4, 1.6)
    fr.part(plume, look['plume'])
    if view == 'down':
        slit = m_rect(cx - 5, cy + 1, cx + 4, cy + 1) | m_rect(cx - 1, cy + 1, cx, cy + 4)
        fr.paint(slit & helm, look['metal'][0])
        fr.paint(m_rect(cx - 5, cy, cx + 4, cy) & helm, look['metal'][3])
    elif view == 'right':
        slit = m_rect(cx + 2, cy + 1, cx + 7, cy + 1)
        fr.paint(slit & helm, look['metal'][0])
        fr.paint(m_rect(cx + 2, cy, cx + 7, cy) & helm, look['metal'][3])
    else:
        fr.paint(m_rect(cx - 1, cy - 5, cx, cy + 5) & helm, look['metal'][2])


def head_horned(fr, look, view, cx, cy, p, skull, face):
    _hair(fr, look, skull, face, view, cx, cy)
    if view != 'up':
        eyes(fr, view, cx, cy, p)
        # war paint across the eyes
        if view == 'down':
            fr.paint(face & m_rect(cx - 5, int(cy + 0.0), cx + 4, int(cy + 0.0)), look['paint'])
        else:
            fr.paint(face & m_rect(cx + 2, int(cy + 0.0), cx + 7, int(cy + 0.0)), look['paint'])
    cap = m_ellipse(cx, cy - 0.5, 7.3, 6.6) & m_rect(0, 0, SIZE, int(cy - 1.5))
    fr.part(cap, look['metal'], shade='round', cx=cx - 1, cy=cy - 3, rx=7, ry=4)
    if view == 'right':
        horns = [(cx - 3, cy - 3, cx - 7, cy - 6.5, cx - 6, cy - 9.5)]
    else:
        horns = [(cx - 6, cy - 2.5, cx - 9.5, cy - 4.5, cx - 9.5, cy - 8.5),
                 (cx + 5, cy - 2.5, cx + 8.5, cy - 4.5, cx + 8.5, cy - 8.5)]
    for x0, y0, x1, y1, x2, y2 in horns:
        h = m_capsule(x0, y0, x1, y1, 1.5) | m_capsule(x1, y1, x2, y2, 0.9)
        fr.part(h, look['horn'])


def head_feathered(fr, look, view, cx, cy, p, skull, face):
    _hair(fr, look, skull, face, view, cx, cy)
    if view != 'up':
        eyes(fr, view, cx, cy, p)
    crown = m_ellipse(cx, cy - 4.6, 5.4, 3.3)
    fr.part(crown, look['hat'])
    fr.paint(crown & m_rect(0, int(cy - 3), SIZE, int(cy - 3)), look['trim'][2])
    fdir = {'down': 1, 'right': -1, 'up': -1}[view]
    feather = m_capsule(cx + 2 * fdir, cy - 6, cx + 7 * fdir, cy - 10, 1.0) | \
        m_capsule(cx + 7 * fdir, cy - 10, cx + 9 * fdir, cy - 9.5, 0.7)
    fr.part(feather, look['feather'])
    brim = m_ellipse(cx + (1 if view == 'right' else 0), cy - 2.4, 9.2, 1.9)
    fr.part(brim, look['hat'])


def head_leafcrown(fr, look, view, cx, cy, p, skull, face):
    _hair(fr, look, skull, face, view, cx, cy)
    if view != 'up':
        eyes(fr, view, cx, cy, p)
    # small antlers
    if view == 'right':
        ants = [(cx - 1, cy - 5, cx - 3, cy - 10, cx - 5, cy - 8)]
    else:
        ants = [(cx - 4, cy - 5, cx - 6, cy - 10, cx - 8, cy - 8),
                (cx + 3, cy - 5, cx + 5, cy - 10, cx + 7, cy - 8)]
    for x0, y0, x1, y1, x2, y2 in ants:
        a = m_capsule(x0, y0, x1, y1, 0.8) | m_capsule((x0 + x1) / 2, (y0 + y1) / 2, x2, y2, 0.7)
        fr.part(a, look['antler'])
    # ring of leaves round the brow
    band = m_ellipse(cx, cy - 2.6, 7.6, 2.0) & ~m_ellipse(cx, cy - 2.2, 6.0, 1.0)
    if view == 'right':
        band &= ~m_rect(cx + 4, 0, SIZE, SIZE)
    fr.part(band, look['leaf'])
    for i, x in enumerate(range(int(cx - 6), int(cx + 7), 3)):
        fr.paint(band & px(x, cy - 3 + (i % 2)), look['leaf'][3])


HEADS = dict(hood=head_hood, wizard=head_wizard, helm=head_helm, horned=head_horned,
             feathered=head_feathered, leafcrown=head_leafcrown)


# ---------------------------------------------------------------- cape, torso

def cape(fr, look, view, cx, y0, sway):
    rmp = look.get('cape')
    if not rmp:
        return
    g = 29
    if view == 'down':
        m = m_poly([(cx - 5.5, y0 + 0.5), (cx + 5.5, y0 + 0.5), (cx + 7.4 + sway, g - 1), (cx - 7.4 + sway, g - 1)])
    elif view == 'up':
        m = m_poly([(cx - 4.6, y0 + 0.3), (cx + 4.6, y0 + 0.3), (cx + 5.6 + sway, g - 1), (cx - 5.6 + sway, g - 1)])
    else:
        m = m_poly([(cx - 2.8, y0 + 0.4), (cx + 1.5, y0 + 0.4), (cx - 1.5 + sway, g - 1.5),
                    (cx - 6.5 + sway * 1.5, g - 2.5)])
    fr.part(m, rmp)
    if view == 'up':
        fr.paint(m & m_rect(0, int(y0 + 1), SIZE, int(y0 + 1)), rmp[2])


def torso_details(fr, look, view, cx, y0):
    torso_zone = m_rect(0, y0, SIZE, y0 + 13)
    if look.get('tabard') and view != 'right':
        fr.part(m_poly([(cx - 2.6, y0 + 1.5), (cx + 2.6, y0 + 1.5), (cx + 2.4, y0 + 10), (cx - 2.4, y0 + 10)]),
                look['tabard'])
        fr.paint(px(cx - 0.5, y0 + 4) | px(cx - 0.5, y0 + 5) | px(cx - 1.5, y0 + 4.5) | px(cx + 0.5, y0 + 4.5),
                 look['trim'][2])
    if look.get('stripe') and view == 'down':
        fr.paint(m_rect(cx - 1, y0 + 1, cx, 28) & torso_zone, look['trim'][1])
        fr.paint(m_rect(cx - 1, y0 + 1, cx - 1, 28) & torso_zone, look['trim'][2])
    if look.get('belt'):
        w = 4 if view != 'right' else 3
        fr.paint(m_rect(cx - w, y0 + 6, cx + w - 1, y0 + 6) & torso_zone, look['belt'][1])
        if view == 'down':
            fr.paint(px(cx - 1, y0 + 6) | px(cx, y0 + 6), look['trim'][2])
    if look.get('strap') and view == 'down':
        for i in range(6):
            fr.paint(px(cx - 3 + i, y0 + 1 + i), look['belt'][0])
    if look.get('fur_collar') and view != 'up':
        fr.part(m_ellipse(cx + (1 if view == 'right' else 0), y0 + 1, 6.0 if view != 'right' else 4.5, 1.8),
                look['fur_collar'])
    if look.get('bare') and view != 'up':
        # chest open to the waist: skin with a fur strap
        w = 2.6 if view == 'down' else 2
        ox = 0 if view == 'down' else 1.2
        chest = m_poly([(cx + ox - w, y0 + 1), (cx + ox + w, y0 + 1), (cx + ox + w - 0.5, y0 + 5.5),
                        (cx + ox - w + 0.5, y0 + 5.5)])
        fr.part(chest, look['skin'], inner=False)


# ---------------------------------------------------------------- weapons

def _metal_line(fr, rmp, x0, y0, x1, y1, light=2):
    m = m_line(x0, y0, x1, y1)
    fr.part(m, rmp, tone=light)
    return m


def weapon(fr, look, hx, hy, ang, view, scale=1.0):
    kind = look['weapon']
    u = (math.cos(math.radians(ang)), math.sin(math.radians(ang)))
    n = (-u[1], u[0])
    d = (u[0] * scale, u[1] * scale)   # lengths along the weapon shrink when it points at the camera
    if kind.startswith('staff'):
        fx, fy = hx + d[0] * 10.5, hy + d[1] * 10.5
        bx, by = hx - d[0] * 4, hy - d[1] * 4
        fr.part(m_line(bx, by, fx, fy), look['wood'], tone=1)
        top = STAFF_TOPS[kind]
        top(fr, look, fx, fy, d, n)
    elif kind == 'sword':
        tip = (hx + d[0] * 9.5, hy + d[1] * 9.5)
        base = (hx + d[0] * 1.5, hy + d[1] * 1.5)
        fr.part(m_capsule(base[0], base[1], tip[0], tip[1], 0.75), look['steel'])
        g0 = (base[0] + n[0] * 2.2, base[1] + n[1] * 2.2)
        g1 = (base[0] - n[0] * 2.2, base[1] - n[1] * 2.2)
        fr.part(m_line(g0[0], g0[1], g1[0], g1[1]), look['trim'], tone=2)
    elif kind == 'axe':
        fx, fy = hx + d[0] * 9, hy + d[1] * 9
        bx, by = hx - d[0] * 2.5, hy - d[1] * 2.5
        fr.part(m_line(bx, by, fx, fy), look['wood'], tone=1)
        c = (fx - d[0] * 1.5, fy - d[1] * 1.5)
        head = m_poly([(c[0] + n[0] * 0.5 - d[0] * 1.6, c[1] + n[1] * 0.5 - d[1] * 1.6),
                       (c[0] + n[0] * 4.6 - d[0] * 3.4, c[1] + n[1] * 4.6 - d[1] * 3.4),
                       (c[0] + n[0] * 5.4 + d[0] * 0.2, c[1] + n[1] * 5.4 + d[1] * 0.2),
                       (c[0] + n[0] * 4.6 + d[0] * 3.4, c[1] + n[1] * 4.6 + d[1] * 3.4),
                       (c[0] + n[0] * 0.5 + d[0] * 1.6, c[1] + n[1] * 0.5 + d[1] * 1.6)])
        fr.part(head, look['steel'])
        edge = m_line(c[0] + n[0] * 5 - d[0] * 2.8, c[1] + n[1] * 5 - d[1] * 2.8,
                      c[0] + n[0] * 5 + d[0] * 2.8, c[1] + n[1] * 5 + d[1] * 2.8) & head
        fr.paint(edge, look['steel'][3])
    elif kind == 'rapier':
        tip = (hx + d[0] * 12, hy + d[1] * 12)
        _metal_line(fr, look['steel'], hx + d[0] * 1.5, hy + d[1] * 1.5, tip[0], tip[1])
        fr.part(m_ellipse(hx + d[0] * 1.2, hy + d[1] * 1.2, 1.5, 1.5), look['trim'], shade='round',
                cx=hx, cy=hy, rx=1.5, ry=1.5)
    elif kind == 'daggers':
        _dagger(fr, look, hx, hy, d, n)
    elif kind == 'bow':
        pts = []
        for i in range(9):
            t = -1 + i / 4
            pts.append((hx + n[0] * t * 6.5 + d[0] * (1 - t * t) * 2.2,
                        hy + n[1] * t * 6.5 + d[1] * (1 - t * t) * 2.2))
        bow = np.zeros((SIZE, SIZE), bool)
        for a, b in zip(pts, pts[1:]):
            bow |= m_line(a[0], a[1], b[0], b[1])
        string = m_line(pts[0][0], pts[0][1], pts[-1][0], pts[-1][1]) & ~bow
        fr.part(string, look['string'], tone=1, inner=False)
        fr.part(bow, look['wood'], tone=2)


def _dagger(fr, look, hx, hy, d, n):
    tip = (hx + d[0] * 6, hy + d[1] * 6)
    _metal_line(fr, look['steel'], hx + d[0] * 1.5, hy + d[1] * 1.5, tip[0], tip[1], light=3)
    fr.part(m_line(hx + d[0] * 1.2 + n[0] * 1.2, hy + d[1] * 1.2 + n[1] * 1.2,
                   hx + d[0] * 1.2 - n[0] * 1.2, hy + d[1] * 1.2 - n[1] * 1.2), look['trim'], tone=1)


def _orb(fr, look, x, y, d, n, r=1.9):
    fr.part(m_ellipse(x + d[0] * 1.2, y + d[1] * 1.2, r, r), look['magic'], shade='round',
            cx=x + d[0] * 1.2 - 0.6, cy=y + d[1] * 1.2 - 0.6, rx=r, ry=r)


def _top_fire(fr, look, x, y, d, n):
    fr.part(m_capsule(x - n[0] * 1.5, y - n[1] * 1.5, x + n[0] * 1.5, y + n[1] * 1.5, 0.6), look['trim'], tone=1)
    _orb(fr, look, x, y, d, n)


def _top_frost(fr, look, x, y, d, n):
    c = (x + d[0] * 1.5, y + d[1] * 1.5)
    crystal = m_poly([(c[0] + d[0] * 3.2, c[1] + d[1] * 3.2), (c[0] + n[0] * 1.6, c[1] + n[1] * 1.6),
                      (c[0] - d[0] * 2.2, c[1] - d[1] * 2.2), (c[0] - n[0] * 1.6, c[1] - n[1] * 1.6)])
    fr.part(crystal, look['magic'])


def _top_storm(fr, look, x, y, d, n):
    fork = m_line(x, y, x + d[0] * 2 + n[0] * 2.2, y + d[1] * 2 + n[1] * 2.2) | \
        m_line(x, y, x + d[0] * 2 - n[0] * 2.2, y + d[1] * 2 - n[1] * 2.2)
    fr.part(fork, look['trim'], tone=2)
    _orb(fr, look, x, y, d, n, r=1.6)


def _top_mend(fr, look, x, y, d, n):
    ring = m_ellipse(x + d[0] * 2, y + d[1] * 2, 2.6, 2.6) & ~m_ellipse(x + d[0] * 2, y + d[1] * 2, 1.3, 1.3)
    fr.part(ring, look['trim'])
    fr.part(m_ellipse(x + d[0] * 2, y + d[1] * 2, 1.0, 1.0), look['magic'], tone=3)


def _top_druid(fr, look, x, y, d, n):
    crook = m_capsule(x, y, x + d[0] * 2 + n[0] * 2.5, y + d[1] * 2 + n[1] * 2.5, 0.7)
    crook |= m_capsule(x + d[0] * 2 + n[0] * 2.5, y + d[1] * 2 + n[1] * 2.5,
                       x + d[0] * 0.5 + n[0] * 4, y + d[1] * 0.5 + n[1] * 4, 0.6)
    fr.part(crook, look['wood'], tone=2)
    for k in (-1.2, 1.4):
        fr.part(m_ellipse(x - d[0] * 1.5 + n[0] * k, y - d[1] * 1.5 + n[1] * k, 1.1, 1.1), look['leaf'])
    fr.part(m_ellipse(x + d[0] * 2.4 + n[0] * 2.4, y + d[1] * 2.4 + n[1] * 2.4, 0.9, 0.9), look['magic'], tone=3)


STAFF_TOPS = dict(staff_fire=_top_fire, staff_frost=_top_frost, staff_storm=_top_storm,
                  staff_mend=_top_mend, staff_druid=_top_druid)


def offhand(fr, look, x, y, ang, view):
    kind = look.get('offhand')
    if kind == 'shield':
        if view == 'right':
            m = m_ellipse(x + 1.0, y - 0.5, 2.0, 4.6)
        elif view == 'down':
            m = m_ellipse(x + 0.4, y - 0.5, 3.6, 4.4)
        else:
            m = m_ellipse(x, y - 0.5, 3.6, 4.4)
        fr.part(m, look['shield'], shade='round', cx=x - 1, cy=y - 2, rx=3.6, ry=4.4)
        ring = m & ~m_ellipse(x + (0.4 if view == 'down' else 0), y - 0.5, 2.6, 3.4)
        if view != 'right':
            fr.paint(ring, look['trim'][1])
            fr.paint(m_ellipse(x + (0.4 if view == 'down' else 0), y - 0.5, 0.9, 0.9), look['trim'][3])
    elif kind == 'dagger':
        d = (math.cos(math.radians(ang + 10)), math.sin(math.radians(ang + 10)))
        _dagger(fr, look, x, y, d, (-d[1], d[0]))


# ---------------------------------------------------------------- effects

def effect(fr, look, fx):
    kind = fx[0]
    if kind == 'arc':          # ('arc', cx, cy, r, a0, a1, width)
        _, cx, cy, r, a0, a1 = fx[:6]
        width = fx[6] if len(fx) > 6 else 1.6
        steps = max(4, int(abs(a1 - a0) / 6))
        core = np.zeros((SIZE, SIZE), bool)
        halo = np.zeros((SIZE, SIZE), bool)
        for i in range(steps + 1):
            t = i / steps
            a = a0 + (a1 - a0) * t
            x, y = polar(cx, cy, a, r)
            w = width * (0.35 + 0.65 * t)
            halo |= m_ellipse(x, y, w + 0.4, w + 0.4)
            core |= m_ellipse(x, y, max(0.5, w - 0.5), max(0.5, w - 0.5))
        fr.glow(halo & ~core, look['slash'][0])
        fr.glow(core, look['slash'][1])
    elif kind == 'orb':        # ('orb', x, y, r)
        _, x, y, r = fx
        fr.glow(m_ellipse(x, y, r + 1, r + 1), look['magic'][1])
        fr.glow(m_ellipse(x, y, r, r), look['magic'][2])
        fr.glow(m_ellipse(x - 0.3, y - 0.3, max(0.6, r - 1.2), max(0.6, r - 1.2)), look['magic'][3])
    elif kind == 'ring':       # ('ring', x, y, rx)
        _, x, y, rx = fx
        outer = m_ellipse(x, y, rx, rx * 0.42)
        inner = m_ellipse(x, y, rx - 1.3, rx * 0.42 - 1.0)
        fr.glow(outer & ~inner, look['magic'][2])
        fr.glow(m_ellipse(x, y, rx - 1.3, rx * 0.42 - 1.0) & ~m_ellipse(x, y, rx - 2.2, rx * 0.42 - 1.6),
                look['magic'][1])
    elif kind == 'sparks':     # ('sparks', x, y, r, seed)
        _, x, y, r, seed = fx
        rng = np.random.default_rng(seed)
        m = np.zeros((SIZE, SIZE), bool)
        for _ in range(5):
            a = rng.uniform(0, 360)
            dd = rng.uniform(r * 0.5, r)
            m |= px(*polar(x, y, a, dd))
        fr.glow(m, look['magic'][3])
    elif kind == 'dust':       # ('dust', x, y, side)
        _, x, y, side = fx
        m = m_ellipse(x, y, 1.6, 1.0) | m_ellipse(x - 2.5 * side, y + 0.3, 1.0, 0.8)
        fr.glow(m, (196, 186, 170, 200))
    elif kind == 'lines':      # ('lines', x, y, dx)  speed lines trailing the body
        _, x, y, dx = fx
        m = np.zeros((SIZE, SIZE), bool)
        for k, (ox, oy, ln) in enumerate(((0, -6, 5), (-1, -1, 6), (0, 4, 4))):
            m |= m_line(x + ox, y + oy, x + ox + dx * ln, y + oy)
        fr.glow(m, (236, 240, 244, 200))
    elif kind == 'stars':      # hurt: a few pale pixels round the head
        _, x, y = fx
        m = px(x - 6, y - 4) | px(x + 6, y - 5) | px(x - 7, y + 1) | px(x + 7, y)
        fr.glow(m, (255, 236, 200, 255))
