#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Generate dimensioned 1:1 cam plate templates, not qualified production parts."""
from pathlib import Path
import math
root=Path(__file__).resolve().parents[1]/'mechanical'
root.mkdir(exist_ok=True)
parts=['<svg xmlns="http://www.w3.org/2000/svg" width="260mm" height="290mm" viewBox="0 0 260 290">',
       '<title>Sector Soak cam templates — Author: jayis1</title>',
       '<metadata>Author: jayis1; units mm; 2mm plate, four layers at 12mm pitch; unvalidated prototype</metadata>']
for bit in range(4):
    cx=65+(bit%2)*130; cy=65+(bit//2)*130
    points=[]
    # Clockwise sector indexing viewed from the printed side; raised=closed.
    # At each sector boundary, repeat the angle to make a radial step.
    for n in range(16):
        radius=54 if ((n^(n>>1))>>bit)&1 else 50
        for j in range(17):
            a=math.radians(n*22.5+j*22.5/16)
            points.append((cx+radius*math.sin(a),cy-radius*math.cos(a)))
    path=' '.join(f'{x:.4f},{y:.4f}' for x,y in points)
    parts.append(f'<polygon points="{path}" fill="none" stroke="black" stroke-width="0.1"/>')
    parts.append(f'<circle cx="{cx}" cy="{cy}" r="6" fill="none" stroke="black" stroke-width="0.1"/>')
    for angle in (0,110,235):
        a=math.radians(angle)
        x=cx+18*math.sin(a); y=cy-18*math.cos(a)
        parts.append(f'<circle cx="{x:.4f}" cy="{y:.4f}" r="1.6" fill="none" stroke="black" stroke-width="0.1"/>')
    parts.append(f'<text x="{cx}" y="{cy+30}" text-anchor="middle" font-size="3">GRAY{bit} / jayis1</text>')
    parts.append(f'<path d="M {cx} {cy-35} v -10" stroke="blue" stroke-width="0.3"/>')
parts.append('<text x="10" y="269" font-size="3">Author: jayis1. Black outlines=cut. Blue radius=sector 0 start. Raised radius=54; base=50.</text>')
parts.append('<text x="10" y="275" font-size="3">Print 100%; verify scale. Bore 12; mounting 3.2. Hand-fit chamfers; never use sharp steps on a live head.</text>')
parts.append('<path d="M 10 282 h 100" stroke="black"/><text x="115" y="283" font-size="3">100 mm scale</text>')
parts.append('</svg>')
(root/'cam-plates.svg').write_text('\n'.join(parts)+'\n')
print('Generated four dimensioned cam plate templates')
