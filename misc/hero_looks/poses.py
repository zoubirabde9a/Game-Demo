"""Animation poses for the new hero look, per view. The front ('down') and
side ('right') are written out; the back ('up') is the front mirrored,
with what was in front of the body moved behind it.

Weapon arm = the character's right: on the viewer's left from the front,
the near arm from the side. The weapon category (staff, blade, bow)
changes only how it is held at rest; every class swings what it holds.
"""
from body import Pose

# frames per animation, the game's frame table depends on these
ANIMS = [('idle', 4), ('walk', 4), ('stop', 2), ('swing_a', 6), ('swing_b', 6),
         ('heavy', 6), ('thrust', 6), ('cast_thrust', 4), ('cast_raise', 4), ('cast_ground', 4)]
SINGLES = ['jump', 'fall', 'dash', 'hurt']


def category(look):
    w = look['weapon']
    if w.startswith('staff'):
        return 'staff'
    if w == 'bow':
        return 'bow'
    return 'blade'


def rest(cat, view):
    """arm angle, weapon angle, weapon z at rest"""
    if view == 'down':
        return {'staff': (110, -110, 'back'), 'blade': (105, 100, 'front'),
                'bow': (110, 180, 'front')}[cat]
    return {'staff': (50, -58, 'front'), 'blade': (100, 60, 'front'),
            'bow': (90, 0, 'front')}[cat]


def P(**k):
    return Pose(**k)


def front(anim, cat):
    wa, wpn, wz = rest(cat, 'down')
    base = dict(wa=wa, wpn=wpn, wz=wz, oa=75, oz='front')

    def R(**k):
        d = dict(base)
        d.update(k)
        return P(**d)

    S = (12.5, 15)   # swing centre: the weapon shoulder area
    if anim == 'idle':
        return [R(), R(), R(bob=1, sway=0), R(bob=1)]
    if anim == 'walk':
        return [R(feet=(2, 0, 0, 0), wa=wa - 12, oa=oa_(87), sway=1),
                R(bob=1),
                R(feet=(0, 2, 0, 0), wa=wa + 8, oa=oa_(63), sway=-1),
                R(bob=1)]
    if anim == 'stop':
        return [R(crouch=1, wa=wa - 20, oa=60, fx=[('dust', 11, 29.5, 1), ('dust', 21, 29.5, -1)]),
                R(crouch=1, fx=[('dust', 9, 29, 1), ('dust', 23, 29, -1)])]
    if anim == 'swing_a':
        return [R(wa=-150, wpn=-120, wz='back', oa=60),
                R(wa=-165, wpn=-150, wz='back', oa=60, lean=-1),
                R(wa=125, wpn=160, wz='front', oa=70, fx=[('arc', 16, 17, 9, 200, 120)]),
                R(wa=50, wpn=15, wz='front', oa=80, lean=1, fx=[('arc', 16, 17, 9, 165, 30, 2)]),
                R(wa=40, wpn=30, wz='front', oa=80, lean=1, fx=[('arc', 16, 17, 9, 90, 35, 1.2)]),
                R(wa=80, wpn=70, wz='front')]
    if anim == 'swing_b':
        return [R(wa=40, wpn=0, wz='front', oa=100),
                R(wa=25, wpn=-25, wz='front', oa=100, lean=1),
                R(wa=90, wpn=110, wz='front', fx=[('arc', 16, 17, 9, -20, 80)]),
                R(wa=150, wpn=190, wz='front', lean=-1, fx=[('arc', 16, 17, 9, 10, 175, 2)]),
                R(wa=165, wpn=205, wz='front', lean=-1, fx=[('arc', 16, 17, 9, 110, 180, 1.2)]),
                R(wa=130, wpn=150)]
    if anim == 'heavy':
        up = dict(wz='back', twohand=True)
        return [R(crouch=1, wa=-120, wpn=-100, oa=-60, **up),
                R(bob=-1, wa=-100, wpn=-92, oa=-80, ol=5.5, **up),
                R(bob=-1, wa=-98, wpn=-90, oa=-82, ol=5.5, **up),
                R(crouch=2, wa=70, wpn=88, oa=105, wz='front',
                  fx=[('arc', 3, 17, 10, -42, 45, 2.4)]),
                R(crouch=2, wa=72, wpn=90, oa=105, wz='front',
                  fx=[('ring', 13, 29, 9), ('dust', 7, 29, 1), ('dust', 22, 29, -1)]),
                R(crouch=1, wa=85, wpn=95, oa=90)]
    if anim == 'thrust':
        return [R(wa=150, wl=4, wpn=100, wscale=0.7, oa=60, lean=1),
                R(wa=160, wl=3.5, wpn=110, wscale=0.7, oa=60, lean=1, crouch=1),
                R(wa=90, wl=6, wpn=90, wscale=0.55, oa=40, crouch=1, bob=1,
                  fx=[('arc', 12, 24, 4, 60, 120, 1.4)]),
                R(wa=88, wl=6, wpn=90, wscale=0.55, oa=40, crouch=1, bob=1),
                R(wa=95, wl=5.5, wpn=95, wscale=0.7, oa=60, crouch=1),
                R(wa=100, wpn=100)]
    if anim == 'cast_thrust':
        return [R(oa=10, ol=4.5),
                R(oa=40, ol=4, fx=[('orb', 23, 21, 1)]),
                R(oa=90, ol=3.5, fx=[('orb', 20.5, 23.5, 2)]),
                R(oa=90, ol=3.5, fx=[('orb', 20.5, 24, 2.6), ('sparks', 20.5, 24, 5, 3)])]
    if anim == 'cast_raise':
        up = dict(wa=-110, wpn=-95, wz='back', oa=-70)
        return [R(wa=-150, wpn=-110, wz='back', oa=-30),
                R(fx=[('orb', 16, 4.5, 1.5)], **up),
                R(fx=[('orb', 16, 4, 2.3), ('sparks', 16, 4, 5, 1)], **up),
                R(fx=[('orb', 16, 4, 2.7), ('sparks', 16, 4, 5, 2)], bob=-1, **up)]
    if anim == 'cast_ground':
        return [R(bob=-1, wa=-120, wpn=-100, wz='back', oa=-60),
                R(crouch=2, wa=80, wpn=90, wz='front', oa=100),
                R(crouch=2, wa=80, wpn=90, wz='front', oa=100, fx=[('ring', 16, 29, 8), ('sparks', 16, 28, 9, 4)]),
                R(crouch=1, wa=85, wpn=92, oa=95, fx=[('ring', 16, 29, 11.5)])]
    if anim == 'jump':
        return [R(feet=(3, 3, 0, 0), wa=-140, wpn=-100 if cat == 'staff' else 200, oa=-40)]
    if anim == 'fall':
        return [R(feet=(0, 1, 0, 0), wa=170, wpn=200 if cat != 'staff' else -110, oa=10)]
    if anim == 'dash':
        return [R(crouch=1, wa=150, wpn=150, oa=30, fx=[('lines', 8, 22, -1), ('lines', 24, 22, 1)])]
    if anim == 'hurt':
        return [R(crouch=1, wa=150, wpn=200 if cat != 'staff' else -120, oa=30, eyes='shut',
                  hdy=1, fx=[('stars', 16, 9)])]
    raise KeyError(anim)


def oa_(a):
    return a


def side(anim, cat):
    wa, wpn, wz = rest(cat, 'right')
    base = dict(wa=wa, wpn=wpn, wz=wz, oa=85, oz='back')

    def R(**k):
        d = dict(base)
        d.update(k)
        return P(**d)

    S = (16.5, 17.5)
    if anim == 'idle':
        return [R(), R(), R(bob=1), R(bob=1)]
    if anim == 'walk':
        return [R(feet=(3, 0, -3, 0), bob=1, wa=wa + 25, wpn=wpn + 20 if cat == 'blade' else wpn, oa=60, sway=-1),
                R(feet=(0, 2, 0, 0), wa=wa, oa=85),
                R(feet=(-3, 0, 3, 0), bob=1, wa=wa - 25, wpn=wpn - 15 if cat == 'blade' else wpn, oa=110, sway=1),
                R(feet=(0, 0, 0, 2), wa=wa, oa=85)]
    if anim == 'stop':
        return [R(lean=-1, feet=(4, 0, 1, 0), wa=wa - 30, oa=40, fx=[('dust', 22, 29.5, -1)]),
                R(lean=-1, crouch=1, feet=(3, 0, 1, 0), fx=[('dust', 24, 29, -1)])]
    if anim == 'swing_a':
        return [R(lean=-1, wa=-130, wpn=-160, wz='back'),
                R(lean=-1, wa=-110, wpn=-120, wz='back'),
                R(lean=1, wa=-25, wpn=-15, wz='front', feet=(2, 0, -2, 0),
                  fx=[('arc', 16.5, 17.5, 10, -120, -15, 1.6)]),
                R(lean=1, wa=35, wpn=45, wz='front', feet=(2, 0, -2, 0),
                  fx=[('arc', 16.5, 17.5, 10, -75, 50, 2.1)]),
                R(wa=60, wpn=95, wz='front', feet=(2, 0, -2, 0), fx=[('arc', 16.5, 17.5, 10, 10, 95, 1.2)]),
                R(wa=85, wpn=75, wz='front')]
    if anim == 'swing_b':
        return [R(wa=70, wpn=110, wz='front', crouch=1),
                R(wa=60, wpn=130, wz='front', crouch=1, lean=-1),
                R(wa=15, wpn=10, wz='front', lean=1, feet=(2, 0, -2, 0),
                  fx=[('arc', 16.5, 17.5, 10, 130, 15, 1.6)]),
                R(wa=-45, wpn=-70, wz='front', lean=1, feet=(2, 0, -2, 0),
                  fx=[('arc', 16.5, 17.5, 10, 70, -70, 2.1)]),
                R(wa=-60, wpn=-95, wz='back', fx=[('arc', 16.5, 17.5, 10, -10, -90, 1.2)]),
                R(wa=60, wpn=80)]
    if anim == 'heavy':
        return [R(crouch=1, lean=-1, wa=-120, wpn=-160, wz='back', oa=-110),
                R(bob=-1, lean=-1, wa=-100, wpn=-120, wz='back', oa=-100),
                R(bob=-1, lean=-1, wa=-95, wpn=-105, wz='back', oa=-95),
                R(crouch=2, lean=2, wa=35, wpn=55, wz='front', oa=40, feet=(3, 0, -3, 0),
                  fx=[('arc', 17, 17, 11, -110, 55, 2.4)]),
                R(crouch=2, lean=2, wa=40, wpn=70, wz='front', oa=45, feet=(3, 0, -3, 0),
                  fx=[('ring', 26, 29, 6), ('dust', 22, 29, 1), ('dust', 29, 29, -1)]),
                R(crouch=1, lean=1, wa=70, wpn=80, wz='front', oa=70)]
    if anim == 'thrust':
        return [R(lean=-1, wa=165, wl=4.5, wpn=0, wz='front', feet=(1, 0, -1, 0)),
                R(lean=-2, crouch=1, wa=175, wl=4.5, wpn=0, wz='front', feet=(1, 0, -2, 0)),
                R(lean=3, crouch=1, wa=0, wl=6, wpn=0, wz='front', oa=150, feet=(5, 0, -4, 0),
                  fx=[('lines', 11, 21, -1)]),
                R(lean=3, crouch=1, wa=0, wl=6.5, wpn=0, wz='front', oa=150, feet=(5, 0, -4, 0),
                  fx=[('arc', 27, 22, 2, 0, 360, 1.0)]),
                R(lean=2, wa=10, wl=5.5, wpn=5, wz='front', oa=120, feet=(4, 0, -3, 0)),
                R(lean=0, wa=60, wpn=40, wz='front')]
    if anim == 'cast_thrust':
        return [R(oa=160, ol=4.5),
                R(oa=60, ol=5, fx=[('orb', 22, 23, 1)]),
                R(oa=0, ol=6, lean=1, fx=[('orb', 25.5, 18, 2)]),
                R(oa=0, ol=6, lean=1, fx=[('orb', 26, 18, 2.6), ('sparks', 26, 18, 5, 3)])]
    if anim == 'cast_raise':
        up = dict(wa=-80, wpn=-88, wz='front', oa=-60)
        return [R(wa=-130, wpn=-100, wz='back', oa=-150),
                R(fx=[('orb', 19, 4.5, 1.5)], **up),
                R(fx=[('orb', 19, 4, 2.3), ('sparks', 19, 4, 5, 1)], **up),
                R(fx=[('orb', 19, 4, 2.7), ('sparks', 19, 4, 5, 2)], bob=-1, **up)]
    if anim == 'cast_ground':
        return [R(bob=-1, wa=-110, wpn=-120, wz='back', oa=-100),
                R(crouch=2, lean=1, wa=40, wpn=80, wz='front', oa=70),
                R(crouch=2, lean=1, wa=40, wpn=80, wz='front', oa=70,
                  fx=[('ring', 18, 29, 8), ('sparks', 18, 28, 9, 4)]),
                R(crouch=1, wa=60, wpn=85, wz='front', oa=80, fx=[('ring', 18, 29, 11.5)])]
    if anim == 'jump':
        return [R(feet=(1, 3, -1, 2), wa=-120, wpn=-80 if cat == 'staff' else -40, oa=-130)]
    if anim == 'fall':
        return [R(feet=(2, 0, -2, 1), wa=-20, wpn=-60 if cat == 'staff' else 10, oa=-160)]
    if anim == 'dash':
        return [R(lean=3, crouch=1, feet=(-2, 1, -5, 2), wa=165, wpn=170, wz='front', oa=170,
                  fx=[('lines', 9, 21, -1)])]
    if anim == 'hurt':
        return [R(lean=-2, crouch=1, wa=-150, wpn=-160 if cat != 'staff' else -100, oa=-140,
                  eyes='shut', hdx=-1, fx=[('stars', 15, 9)])]
    raise KeyError(anim)


ZSWAP = {'front': 'back', 'back': 'front', 'mid': 'front', 'grip': 'grip'}


def back(anim, cat):
    out = []
    for p in front(anim, cat):
        q = Pose(p)
        q['wa'] = 180 - p.wa
        q['oa'] = 180 - p.oa
        q['wpn'] = 180 - p.wpn
        q['wz'] = ZSWAP[p.wz]
        q['oz'] = ZSWAP[p.oz]
        q['lean'] = -p.lean
        q['sway'] = -p.sway
        q['feet'] = (p.feet[1], p.feet[0], 0, 0)
        fx = []
        for f in p.fx:
            if f[0] == 'arc':
                fx.append(('arc', 32 - f[1], f[2], f[3], 180 - f[4], 180 - f[5]) + tuple(f[6:]))
            elif f[0] in ('orb', 'ring', 'sparks', 'dust', 'stars'):
                g = list(f)
                g[1] = 32 - f[1]
                if f[0] == 'dust':
                    g[3] = -f[3]
                fx.append(tuple(g))
            elif f[0] == 'lines':
                fx.append(('lines', 32 - f[1], f[2], -f[3]))
        q['fx'] = fx
        out.append(q)
    return out


def frames(look, anim, view):
    cat = category(look)
    if view == 'down':
        return front(anim, cat)
    if view == 'right':
        return side(anim, cat)
    return back(anim, cat)
