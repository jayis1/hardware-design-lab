#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Check deliverable thresholds, author metadata and cross-artifact pin names."""
from pathlib import Path
import json
import re

root = Path(__file__).resolve().parents[1]
required = ['README.md','VALIDATION.md','LICENSE','firmware/main.c','firmware/board.h',
            'firmware/registers.h','firmware/Makefile','app/workbench.py',
            'kicad/device.kicad_sch','kicad/device.kicad_pcb','kicad/device.kicad_pro']
for name in required:
    assert (root/name).is_file(), name
files = []
for p in root.rglob('*'):
    if not p.is_file() or 'build' in p.parts or '__pycache__' in p.parts:
        continue
    if p.suffix in ('.pyc','.kicad_prl') or p.name == 'fp-info-cache':
        continue
    assert 'jayis1' in p.read_text(), str(p)
    files.append(p)
words = len((root/'README.md').read_text().split())
assert words >= 2000, words
lines = 0
for p in files:
    if 'firmware' in p.parts and p.suffix in ('.c','.h'):
        source = re.sub(r'/\*.*?\*/|//[^\n]*','',p.read_text(),flags=re.S)
        lines += sum(bool(line.strip()) for line in source.splitlines())
assert lines >= 500, lines
manifest = json.loads((root/'kicad/connectivity.json').read_text())
components = {c['ref']:c for c in manifest['components']}
for i in range(4):
    assert components[f'J{i+3}']['nets'] == [f'STATION_{i}','GND']
    assert components[f'R{2*i+1}']['nets'] == ['VCC',f'SENSE{i}']
    assert components[f'R{2*i+2}']['nets'] == [f'SENSE{i}',f'STATION_{i}']
    assert f'STATION_{i}' in (root/'firmware/board.h').read_text()
assert components['J2']['nets'] == ['SENSE0','SENSE1','SENSE2','SENSE3']
print(f'PASS {len(files)} authored deliverables; README {words} words; {lines} nonblank C/header lines excluding comments')
print('PASS four station identities, pull-ups, series resistors and GPIO harness manifest')
print(f'Calculated nominal carrier: I/contact={3.3/11000*1000:.2f} mA; '
      f'closed sense={3.3/11000*1000:.2f} V; '
      f'all-closed power={4*3.3*3.3/11000*1000:.2f} mW')
