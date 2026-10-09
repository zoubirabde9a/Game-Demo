"""One hero frame: a class look (looks.py) in a pose (poses.py), seen from
the front ('down'), the right side ('right') or the back ('up').

The figure stands on row 30 (the outline takes row 31), centred on x 16,
like the original hero, so the game places it with the same origin.
Angles are screen angles in degrees: 0 points right, 90 down, -90 up.
"""
import math
import numpy as np
from canvas import (Frame, m_ellipse, m_capsule, m_poly, m_rect, m_line, shift)
import gear

GROUND = 30


def polar(x, y, ang, length):
    a = math.radians(ang)
    return x + math.cos(a) * length, y + math.sin(a) * length


class Pose(dict):
    def __getattr__(self, k):
        return self.get(k, DEFAULTS.get(k))


DEFAULTS = dict(bob=0, lean=0, crouch=0, hdx=0, hdy=0,
                wa=100, wl=5.0, oa=80, ol=5.0, wpn=95, wz='front', oz='front',
                feet=(0, 0, 0, 0), eyes='open', sway=0, fx=(), item=True, twohand=False, wscale=1.0)


def draw(look, pose, view):
    fr = Frame()
    p = pose
    drop = p.bob + p.crouch
    hip_y = 23 + drop
    if view == 'right':
        _draw_side(fr, look, p, drop, hip_y)
    else:
        _draw_front_back(fr, look, p, drop, hip_y, view)
    for fx in p.fx:
        gear.effect(fr, look, fx)
    return fr.render()


# ---------------------------------------------------------------- front/back

def _legs_front(fr, look, p, hip_y):
    lifts = (p.feet[0], p.feet[1])
    for i, x0 in enumerate((13, 16)):
        lift = lifts[i]
        bottom = GROUND - lift
        top = hip_y
        fr.part(m_rect(x0, top, x0 + 2, bottom - 2), look['legs'])
        boot = m_rect(x0 - (1 if i == 0 else 0), bottom - 2, x0 + 2 + (1 if i == 1 else 0), bottom)
        fr.part(boot, look['boots'])


def _torso_front(fr, look, p, drop, view):
    x = p.lean
    y0 = 16 + drop
    torso = m_poly([(11.6 + x, y0 + 0.4), (20.4 + x, y0 + 0.4), (20 + x, y0 + 8.2), (12 + x, y0 + 8.2)])
    fr.part(torso, look['body'])
    if look.get('robe'):
        sway = p.sway
        skirt = m_poly([(12 + x, y0 + 5), (20 + x, y0 + 5),
                        (21.6 + sway, GROUND - 1.5), (10.4 + sway, GROUND - 1.5)])
        fr.part(skirt, look['robe_ramp'] if 'robe_ramp' in look else look['body'])
    gear.torso_details(fr, look, view, 16 + x, y0)
    return 16 + x, y0


def _arm(fr, look, sx, sy, ang, length, ramp_key='sleeve', behind=False):
    hx, hy = polar(sx, sy, ang, length)
    rmp = look.get(ramp_key, look['body'])
    if behind:
        rmp = [rmp[0], rmp[0], rmp[1], rmp[1]]
    fr.part(m_capsule(sx, sy, hx, hy, 1.25), rmp)
    return hx, hy


def _hand(fr, look, hx, hy):
    fr.part(m_ellipse(hx, hy, 1.35, 1.35), look['skin'], shade='flat')


def _draw_front_back(fr, look, p, drop, hip_y, view):
    front = view == 'down'
    # weapon arm is the character's right: viewer's left from the front
    wsx = 11.6 if front else 20.4
    osx = 20.4 if front else 11.6
    sy = 17.6 + drop
    cx = 16 + p.lean
    wsx += p.lean
    osx += p.lean

    hx, hy = polar(wsx, sy, p.wa, p.wl)
    ox, oy = polar(osx, sy, p.oa, p.ol)

    # behind everything: cape (front view: peeks out at the sides), back items
    if view == 'up':
        pass
    else:
        gear.cape(fr, look, view, cx, 16 + drop, p.sway)
    if p.item and p.wz == 'back':
        gear.weapon(fr, look, hx, hy, p.wpn, view, p.wscale)
    if p.item and p.oz == 'back':
        gear.offhand(fr, look, ox, oy, p.oa, view)
    _legs_front(fr, look, p, hip_y)
    tx, ty = _torso_front(fr, look, p, drop, view)
    if view == 'up':
        # from the back the arms are under the cape and the head
        _arm(fr, look, wsx, sy, p.wa, p.wl)
        _arm(fr, look, osx, sy, p.oa, p.ol)
        if p.wz != 'front':
            _hand(fr, look, hx, hy)
        if p.oz != 'front':
            _hand(fr, look, ox, oy)
        gear.cape(fr, look, view, cx, 16 + drop, p.sway)
    if p.item and p.wz == 'mid':
        gear.weapon(fr, look, hx, hy, p.wpn, view, p.wscale)
    # an arm raised above the shoulder is drawn over the head
    back = view == 'up'
    w_up = back or math.sin(math.radians(p.wa)) < -0.35
    o_up = back or math.sin(math.radians(p.oa)) < -0.35
    if not w_up:
        _arm(fr, look, wsx, sy, p.wa, p.wl)
    if not o_up:
        _arm(fr, look, osx, sy, p.oa, p.ol)
    gear.head(fr, look, view, 16 + p.lean + p.hdx, 10 + drop + p.hdy, p)
    if w_up and not back:
        _arm(fr, look, wsx, sy, p.wa, p.wl)
    if o_up and not back:
        _arm(fr, look, osx, sy, p.oa, p.ol)
    if p.item and p.wz == 'front':
        gear.weapon(fr, look, hx, hy, p.wpn, view, p.wscale)
    if p.item and p.oz == 'front':
        gear.offhand(fr, look, ox, oy, p.oa, view)
    if not back or p.wz == 'front':
        _hand(fr, look, hx, hy)
    if not back or p.oz == 'front':
        _hand(fr, look, ox, oy)
    if p.item and p.wz == 'grip':
        gear.weapon(fr, look, hx, hy, p.wpn, view, p.wscale)


# ---------------------------------------------------------------- side

def _draw_side(fr, look, p, drop, hip_y):
    x = p.lean
    sy = 17.6 + drop
    near_s = (16.2 + x, sy)
    far_s = (17.2 + x, sy)
    hx, hy = polar(near_s[0], near_s[1], p.wa, p.wl)
    ox, oy = polar(far_s[0], far_s[1], p.oa, p.ol)

    if p.item and p.wz == 'back':
        gear.weapon(fr, look, hx, hy, p.wpn, 'right', p.wscale)
    # far arm and its item, behind the body
    if p.item and p.oz == 'back':
        gear.offhand(fr, look, ox, oy, p.oa, 'right')
    _arm(fr, look, far_s[0], far_s[1], p.oa, p.ol, behind=True)
    _hand(fr, look, ox, oy)
    gear.cape(fr, look, 'right', 16 + x, 16 + drop, p.sway)
    # legs: far then near; feet = (near dx, near lift, far dx, far lift)
    ndx, nlift, fdx, flift = p.feet
    for dx, lift, behind in ((fdx, flift, True), (ndx, nlift, False)):
        fx = 15.5 + dx
        bottom = GROUND - lift
        leg = m_capsule(15.5 + x * 0.5, hip_y, fx, bottom - 2, 1.4)
        rl, rb = look['legs'], look['boots']
        if behind:
            rl = [rl[0], rl[0], rl[1], rl[1]]
            rb = [rb[0], rb[0], rb[1], rb[1]]
        fr.part(leg, rl)
        fr.part(m_rect(fx - 1.5, bottom - 2, fx + 2.5, bottom), rb)
    y0 = 16 + drop
    torso = m_poly([(13 + x, y0 + 0.4), (19.6 + x, y0 + 0.4), (19.2 + x, y0 + 8.2), (13.2 + x, y0 + 8.2)])
    fr.part(torso, look['body'])
    if look.get('robe'):
        s = p.sway
        skirt = m_poly([(13 + x, y0 + 5), (19.6 + x, y0 + 5), (21 + x * 0.5 + s, GROUND - 1.5),
                        (11.5 + x * 0.5 + s, GROUND - 1.5)])
        fr.part(skirt, look['robe_ramp'] if 'robe_ramp' in look else look['body'])
    gear.torso_details(fr, look, 'right', 16.3 + x, y0)
    if p.item and p.wz == 'mid':
        gear.weapon(fr, look, hx, hy, p.wpn, 'right', p.wscale)
    gear.head(fr, look, 'right', 16 + x + p.hdx, 10 + drop + p.hdy, p)
    if p.item and p.oz == 'front':
        gear.offhand(fr, look, ox, oy, p.oa, 'right')
    _arm(fr, look, near_s[0], near_s[1], p.wa, p.wl)
    if p.item and p.wz == 'front':
        gear.weapon(fr, look, hx, hy, p.wpn, 'right', p.wscale)
    _hand(fr, look, hx, hy)
    if p.item and p.wz == 'grip':
        gear.weapon(fr, look, hx, hy, p.wpn, 'right', p.wscale)
