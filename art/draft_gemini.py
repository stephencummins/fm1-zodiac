#!/usr/bin/env python3
"""Writes art/gemini.svg: the twins, two faces in profile turned to each other. Inks: 1 rose, 2 pale yellow (the
left twin), 3 sky blue (the right twin)."""
from draftlib import *

def twin(m, s):
    mine = INK2 if s == "L" else INK3
    shape([(m(68), 182), (m(88), 182), (m(90), 150), (m(66), 150)], f"neck{s}-shape", width=1.0, so=0.7, fill=mine, fo=0.18)
    shape([(m(105), 143), (m(108.5), 148), (m(104), 159), (m(88), 164), (m(77), 150), (m(84), 142)], f"jaw{s}-shape", width=1.2, fill=mine, fo=0.30)
    head = [(m(84), 82), (m(101), 90), (m(105), 104), (m(103), 113), (m(114), 129), (m(106.5), 132.5), (m(109.5), 138), (m(105), 142.5),
            (m(84), 145), (m(66), 151), (m(61), 150), (m(57), 120), (m(65), 94)]
    shape(head, f"head{s}-line", width=1.5, fill=GOLD, fo=0.06)
    shape([(m(84), 82), (m(101), 90), (m(105), 104), (m(86), 108)], f"head{s}-brow", width=0.7, so=0.4, fill=mine, fo=0.55, hide=False)
    shape([(m(84), 82), (m(86), 108), (m(57), 120), (m(65), 94)], f"head{s}-crown", width=0.7, so=0.4, fill=INK1, fo=0.40, hide=False)
    shape([(m(86), 108), (m(103), 113), (m(114), 129), (m(106.5), 132.5), (m(90), 128)], f"head{s}-cheek", width=0.7, so=0.4, fill=mine, fo=0.32, hide=False)
    shape([(m(86), 108), (m(90), 128), (m(84), 145), (m(66), 151), (m(57), 120)], f"head{s}-back", width=0.7, so=0.35, fill=mine, fo=0.16, hide=False)
    path(P([(m(70), 124), (m(66), 130), (m(70), 137), (m(74), 131)], True), f"head{s}-ear", stroke=GOLD, width=0.9, so=0.7)
    shape([(m(90), 112), (m(96.5), 108.5), (m(102), 113), (m(96), 116)], f"eye{s}-shape", width=0.9, fill=GOLD, fo=0.9)
    path(P([(m(97.5), 112.4), (m(98.5), 112.6)]), f"pupil{s}-dot", stroke=BLACK, width=3.4)
    path(P([(m(88), 106), (m(96), 102.5), (m(104), 106.5)]), f"eye{s}-brow", stroke=GOLD, width=1.1, so=0.85)
both(twin)
write("gemini")
