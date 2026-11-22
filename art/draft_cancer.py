#!/usr/bin/env python3
"""Writes art/cancer.svg: the crab, seen from the front, claws up. Inks: 1 coral, 2 teal, 3 deep blue."""
from draftlib import *

LEGS = [((80, 134), (58, 128), (46, 150)), ((78, 142), (54, 142), (44, 166)),
        ((80, 150), (58, 156), (52, 180)), ((86, 157), (68, 168), (66, 190))]
def legs(m, s):
    for k, (B, K, F) in enumerate(LEGS):
        pts = [(m(B[0]), B[1] - 2.5), (m(K[0]), K[1] - 3.2), (m(F[0]), F[1]), (m(K[0] + 3), K[1] + 2.6), (m(B[0]), B[1] + 2.5)]
        shape(pts, f"leg{s}{k}-seg", width=1.0, fill=INK1, fo=0.42)
both(legs)

body = sym([(120, 122), (100, 120), (84, 126), (74, 138), (80, 152), (98, 162), (120, 164)])
shape(body, "body-line", width=1.6, fill=GOLD, fo=0.06)
def planes(m, s):
    shape([(120, 122), (m(100), 120), (m(96), 138), (120, 142)], f"body-top{s}", width=0.8, so=0.45, fill=INK1, fo=0.62, hide=False)
    shape([(m(100), 120), (m(84), 126), (m(74), 138), (m(96), 138)], f"body-side{s}", width=0.8, so=0.45, fill=INK1, fo=0.40, hide=False)
    shape([(m(74), 138), (m(80), 152), (m(98), 162), (m(96), 138)], f"body-low{s}", width=0.8, so=0.4, fill=INK1, fo=0.24, hide=False)
    shape([(m(96), 138), (m(98), 162), (120, 164), (120, 142)], f"body-belly{s}", width=0.8, so=0.4, fill=INK2, fo=0.30, hide=False)
    path(P([(m(113), 153), (m(116), 158), (120, 156)]), f"body-jaw{s}", stroke=GOLD, width=1.0, so=0.8)
both(planes)
path(P([(114, 159.5), (126, 159.5)]), "body-mouth", stroke=BLACK, width=1.6)

def eye(m, s):
    path(P([(m(110), 121), (m(108), 107)]), f"eye{s}-stalk", stroke=GOLD, width=1.7)
    shape([(m(103.5), 104), (m(108), 99.5), (m(112.5), 104), (m(108), 108.5)], f"eye{s}-ball", width=0.9, fill=GOLD, fo=1.0)
    path(P([(m(108), 103.4), (m(108), 104.6)]), f"eye{s}-pupil", stroke=BLACK, width=3.2)
both(eye)

TH, W, SC = math.radians(20), (66.0, 92.0), 1.05
def claw(pts, m):
    o = []
    for x, y in pts:
        x, y = x * SC, (y - 10) * SC
        o.append((m(W[0] + x * math.cos(TH) - y * math.sin(TH)), W[1] + x * math.sin(TH) + y * math.cos(TH)))
    return o
def arm(m, s):
    shape([(m(88), 123), (m(84), 131), (m(58), 116), (m(61), 108)], f"arm{s}-upper", width=1.0, fill=INK1, fo=0.36)
    shape([(m(56), 112), (m(64), 113), (m(70), 93), (m(62), 91)], f"arm{s}-fore", width=1.0, fill=INK1, fo=0.50)
    shape(claw([(-11, 0), (-10.5, -10), (-7.5, -20), (-2, -29), (-3.5, -18), (-4.5, -9), (-1, -2), (11, 0), (8, 10), (-8, 10)], m),
          f"arm{s}-claw", width=1.3, fill=INK1, fo=0.66)
    path(P(claw([(-11, 0), (-1, -2), (11, 0)], m)), f"arm{s}-knuckle", stroke=GOLD, width=0.8, so=0.6)
    shape(claw([(1, -2), (4.5, -9), (3.5, -18), (2, -29), (7.5, -20), (10.5, -10), (11, 0)], m), f"finger{s}-shape", width=1.3, fill=INK2, fo=0.55)
    if s == "L": print("finger hinge", claw([(8, 0)], m), "wrist", W)
both(arm)
write("cancer")
