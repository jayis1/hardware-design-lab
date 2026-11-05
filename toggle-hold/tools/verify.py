#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Deliverable/attribution and shared hardware-interface gates; no physical claims."""
from pathlib import Path
import json
import re

root = Path(__file__).resolve().parents[1]
required = ['README.md', 'VALIDATION.md', 'LICENSE', 'firmware/main.c', 'firmware/board.h',
            'firmware/registers.h', 'firmware/Makefile', 'app/workbench.py',
            'kicad/device.kicad_sch', 'kicad/device.kicad_pcb', 'kicad/device.kicad_pro']
for name in required:
    assert (root/name).is_file(), name
files = []
for p in root.rglob('*'):
    if not p.is_file() or 'build' in p.parts or '__pycache__' in p.parts:
        continue
    if p.suffix in ('.pyc', '.kicad_prl') or p.name == 'fp-info-cache':
        continue
    assert 'jayis1' in p.read_text(), str(p)
    files.append(p)
words = len((root/'README.md').read_text().split())
assert words >= 2000, words
counts = {'.c': 0, '.h': 0}
for p in files:
    if 'firmware' in p.parts and p.suffix in counts:
        source = re.sub(r'/\*.*?\*/|//[^\n]*', '', p.read_text(), flags=re.S)
        counts[p.suffix] += sum(bool(line.strip()) for line in source.splitlines())
assert counts['.c'] >= 500, counts
manifest = json.loads((root/'kicad/connectivity.json').read_text())
components = {c['ref']: c for c in manifest['components']}
for i, name in enumerate(('END_A', 'END_B', 'PUSH_B', 'PUSH_A')):
    assert components[f'J{i+3}']['nets'] == [name, 'GND']
    assert components[f'R{2*i+1}']['nets'] == ['VCC', f'SENSE{i}']
    assert components[f'R{2*i+2}']['nets'] == [f'SENSE{i}', name]
    assert name in (root/'firmware/board.h').read_text()
assert components['J2']['nets'] == ['SENSE0', 'SENSE1', 'SENSE2', 'SENSE3']
print(f'PASS {len(files)} authored deliverables; README {words} words; nonblank comment-free firmware lines {counts}')
print('PASS shared endpoint/plunger pin contract and passive network')
print(f'Nominal closed contact: {3.3/11000*1000:.2f} mA; sense {3.3/11:.2f} V; '
      f'four-channel resistive power {4*3.3*3.3/11000*1000:.2f} mW')
