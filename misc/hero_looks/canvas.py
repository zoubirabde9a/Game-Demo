"""Pixel canvas for the new hero look (draw_heroes.py).

A frame is built from parts drawn back to front. Each part is a mask with a
material (a ramp of 4 colours, dark to highlight). Shading is per part: a
pillow shade lit from the top left, or a round shade for heads and orbs.
Then two outline passes: a part's edge against a part behind it gets that
part's darkest tone (the inner line), and the empty pixels touching the
figure get the outline colour.
"""
import math
import numpy as np

SIZE = 32
OUTLINE = (24, 14, 22, 255)


def ramp(*cols):
    """4 tones dark -> highlight from 2..4 given colours"""
    cols = list(cols)
    while len(cols) < 4:
        cols.append(cols[-1])
    return [tuple(c) + (255,) for c in cols]


def grid():
    ys, xs = np.mgrid[0:SIZE, 0:SIZE]
    return xs + 0.5, ys + 0.5


GX, GY = grid()


def m_ellipse(cx, cy, rx, ry):
    return ((GX - cx) / rx) ** 2 + ((GY - cy) / ry) ** 2 <= 1.0


def m_capsule(x0, y0, x1, y1, r):
    dx, dy = x1 - x0, y1 - y0
    ll = dx * dx + dy * dy
    if ll < 1e-6:
        t = np.zeros_like(GX)
    else:
        t = np.clip(((GX - x0) * dx + (GY - y0) * dy) / ll, 0, 1)
    px, py = x0 + t * dx, y0 + t * dy
    return (GX - px) ** 2 + (GY - py) ** 2 <= r * r


def m_poly(pts):
    """pixel centres inside the polygon (even-odd)"""
    inside = np.zeros((SIZE, SIZE), bool)
    n = len(pts)
    for i in range(n):
        x0, y0 = pts[i]
        x1, y1 = pts[(i + 1) % n]
        cond = (y0 > GY) != (y1 > GY)
        with np.errstate(divide='ignore', invalid='ignore'):
            xint = (x1 - x0) * (GY - y0) / (y1 - y0 + 1e-12) + x0
        inside ^= cond & (GX < xint)
    return inside


def m_rect(x0, y0, x1, y1):
    """inclusive pixel rect"""
    m = np.zeros((SIZE, SIZE), bool)
    x0, x1 = max(0, int(x0)), min(SIZE - 1, int(x1))
    y0, y1 = max(0, int(y0)), min(SIZE - 1, int(y1))
    if x1 >= x0 and y1 >= y0:
        m[y0:y1 + 1, x0:x1 + 1] = True
    return m


def m_line(x0, y0, x1, y1):
    """1-pixel line"""
    m = np.zeros((SIZE, SIZE), bool)
    n = int(max(abs(x1 - x0), abs(y1 - y0)) * 2) + 1
    for i in range(n + 1):
        t = i / n
        x, y = int(math.floor(x0 + (x1 - x0) * t)), int(math.floor(y0 + (y1 - y0) * t))
        if 0 <= x < SIZE and 0 <= y < SIZE:
            m[y, x] = True
    return m


def shift(m, dx, dy):
    out = np.zeros_like(m)
    h, w = m.shape
    xs0, xs1 = max(0, -dx), min(w, w - dx)
    ys0, ys1 = max(0, -dy), min(h, h - dy)
    if xs1 > xs0 and ys1 > ys0:
        out[ys0 + dy:ys1 + dy, xs0 + dx:xs1 + dx] = m[ys0:ys1, xs0:xs1]
    return out


def pillow(m):
    """0 dark, 1 mid, 2 light: lit from the top left"""
    s = np.ones(m.shape, np.int8)
    up = shift(m, 0, 1)      # up[y] = m[y-1]
    left = shift(m, 1, 0)
    down = shift(m, 0, -1)
    right = shift(m, -1, 0)
    lit = m & (~up | ~left)
    dark = m & (~down | ~right)
    s[dark] = 0
    s[lit & ~dark] = 2
    return s


def roundshade(m, cx, cy, rx, ry):
    nx = (GX - cx) / max(rx, 0.5)
    ny = (GY - cy) / max(ry, 0.5)
    d = nx + ny
    s = np.ones(m.shape, np.int8)
    s[d > 0.55] = 0
    s[d < -0.45] = 2
    s[(nx + 0.45) ** 2 + (ny + 0.5) ** 2 < 0.05] = 3
    return s


class Frame:
    def __init__(self):
        self.order = np.full((SIZE, SIZE), -1, np.int32)
        self.col = np.zeros((SIZE, SIZE, 4), np.uint8)
        self.parts = []          # (ramp, inner_line)
        self.overlay = []        # (mask, colour) drawn after outlines

    def part(self, m, rmp, shade='pillow', inner=True, cx=None, cy=None, rx=None, ry=None,
             tone=None):
        if not m.any():
            return
        idx = len(self.parts)
        self.parts.append((rmp, inner))
        if tone is not None:
            s = np.full(m.shape, tone, np.int8)
        elif shade == 'round':
            s = roundshade(m, cx, cy, rx, ry)
        elif shade == 'flat':
            s = np.ones(m.shape, np.int8)
        else:
            s = pillow(m)
        cols = np.array(rmp, np.uint8)
        self.col[m] = cols[np.clip(s[m], 0, 3)]
        self.order[m] = idx

    def paint(self, m, colour):
        """recolour pixels of the figure (details: eyes, trims), keeps owner"""
        m = m & (self.order >= 0)
        self.col[m] = colour

    def glow(self, m, colour):
        """drawn last, over outlines, no outline of its own"""
        self.overlay.append((m, colour))

    def render(self):
        img = self.col.copy()
        order = self.order
        filled = order >= 0
        # inner lines: a part's edge pixel next to a part drawn before it
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nb = np.full_like(order, -1)
            ys0, ys1 = max(0, dy), SIZE + min(0, dy)
            xs0, xs1 = max(0, dx), SIZE + min(0, dx)
            nb[ys0 - dy:ys1 - dy, xs0 - dx:xs1 - dx] = order[ys0:ys1, xs0:xs1]
            edge = filled & (nb >= 0) & (nb < order)
            for idx in np.unique(order[edge]):
                rmp, inner = self.parts[idx]
                if inner:
                    sel = edge & (order == idx)
                    img[sel] = rmp[0]
        # outer outline, 4-connected
        grow = shift(filled, 1, 0) | shift(filled, -1, 0) | shift(filled, 0, 1) | shift(filled, 0, -1)
        out = grow & ~filled
        img[out] = OUTLINE
        for m, c in self.overlay:
            img[m] = c
        return img


def mirror(img):
    return img[:, ::-1].copy()
