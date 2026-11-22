#!/usr/bin/env python3
"""Writes art/taurus.svg: the bull, broad and heavy, calm under half-closed lids. Inks: 1 umber, 2 ochre, 3 green."""
from draftlib import *

def ear(m, s):
    pts = [(m(87), 112), (m(68), 105), (m(51), 114), (m(69), 123)]
    shape(pts, f"ear{s}-leaf", fill=INK1, fo=0.5)
    path(P([(m(84), 113), (m(58), 114)]), f"ear{s}-vein", stroke=GOLD, width=0.9, so=0.6)
both(ear)

# the lower jaw: it chews
shape([(107, 190), (133, 190), (129, 206), (120, 209), (111, 206)], "jaw-shape", fill=INK2, fo=0.22)

head = sym([(120, 90), (99, 93), (84, 104), (82, 126), (90, 150), (99, 172), (101, 188), (109, 198), (120, 200)])
shape(head, "head-line", width=1.6, fill=GOLD, fo=0.07)
shape([(120, 90), (104, 113), (120, 136), (136, 113)], "head-brow", width=0.8, so=0.5, fill=INK2, fo=0.40, hide=False)
def planes(m, s):
    shape([(m(99), 93), (m(84), 104), (m(82), 126), (m(104), 113)], f"head-temple{s}", width=0.8, so=0.45, fill=INK1, fo=0.55, hide=False)
    shape([(m(82), 126), (m(90), 150), (m(110), 154), (m(120), 136), (m(104), 113)], f"head-cheek{s}", width=0.8, so=0.4, fill=INK1, fo=0.30, hide=False)
    shape([(m(90), 150), (m(99), 172), (m(120), 167), (m(120), 136), (m(110), 154)], f"head-side{s}", width=0.7, so=0.35, fill=INK2, fo=0.20, hide=False)
    path(P([(m(110), 184), (m(113.5), 187)]), f"head-nostril{s}", stroke=BLACK, width=3.0)
    # a calm eye, and a lid that sits half over it
    shape([(m(91), 128), (m(100), 122.5), (m(109), 128), (m(100), 132.5)], f"eye{s}-shape", width=1.0, fill=INK2, fo=1.0)
    path(P([(m(99), 128), (m(101), 128)]), f"eye{s}-pupil", stroke=BLACK, width=4.2)
    lid = [(m(90), 121.5), (m(110), 121.5), (m(110), 127), (m(90), 127)]
    path(P(lid, True), f"lid{s}-shape", fill=BLACK)
    path(P([(m(90), 127), (m(110), 127)]), f"lid{s}-edge", stroke=GOLD, width=1.3)
both(planes)
muzzle = sym([(120, 167), (99, 172), (101, 188), (109, 198), (120, 200)])
shape(muzzle, "head-muzzle", width=1.0, so=0.7, fill=INK2, fo=0.48)
both(lambda m, s: path(P([(m(110), 184), (m(113.5), 187)]), f"head-nose{s}", stroke=BLACK, width=3.0))
for k in range(3):                                     # a curl of hair on the brow
    r = 3.0 + 2.6 * k
    path(P([(120 + r * math.cos(math.radians(a)), 99 + r * 0.8 * math.sin(math.radians(a))) for a in range(200, 521, 20)][:9 + 3 * k]),
         f"head-curl{k}", stroke=GOLD, width=0.8, so=0.6)

# the ring in its nose
path(P([(120 + 7.5 * math.cos(math.radians(a)), 193 + 8.5 * math.sin(math.radians(a))) for a in range(-25, 206, 11)]), "ring-r", stroke=GOLD, width=2.0)

def horn(m, s):
    cx, cy, total = 86.0, 66.0, 165.0
    def at(deg, side):
        u = deg / total
        w = 15 * (1 - u) ** 0.9 + 1.5
        rr = 36 + 3 * u + side * w / 2
        phi = math.radians(90 + deg)
        return (m(cx + rr * math.cos(phi)), cy + rr * math.sin(phi))
    band(f"horn{s}", at, total, 16.5, [(INK2, 0.55), (GOLD, 0.30), (INK2, 0.35), (INK1, 0.45)], n=40)
both(horn)
write("taurus")
