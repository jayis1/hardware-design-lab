#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Audit authored files and quantitative gates without counting test padding."""
from pathlib import Path
import json
import re
import subprocess
ROOT = Path(__file__).resolve().parents[1]
required = ['README.md', 'LICENSE', 'firmware/main.c', 'firmware/engine.c',
            'firmware/board.h', 'firmware/registers.h', 'firmware/Makefile',
            'kicad/device.kicad_sch', 'kicad/device.kicad_pcb',
            'kicad/device.kicad_pro', 'app/workbench.py', 'app/protocol.py']
for name in required:
    assert (ROOT/name).is_file(), name
files = subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard',
                                 '--',str(ROOT)],cwd=ROOT.parent,text=True).splitlines()
files = sorted(set(files))
for name in files:
    path = ROOT.parent/name
    text = path.read_text()
    assert 'jayis1' in text, name
    assert not re.search(r'(?:Author|Creator)\s*:\s*(?:Hermes|Claude|Nous|Anthropic)',text,re.I), name
    assert not re.search(r'(?:password|api_key|secret)\s*=\s*[\"\'][^\"\']{6,}',text,re.I), name
words = len((ROOT/'README.md').read_text().split())
assert words >= 2000, words
counts = {}
for extension in ('c','h'):
    total = 0
    for file in (ROOT/'firmware').rglob('*.'+extension):
        text = re.sub(r'/\*.*?\*/|//[^\n]*','',file.read_text(),flags=re.S)
        total += sum(bool(line.strip()) for line in text.splitlines())
    counts[extension] = total
assert counts['c'] >= 500, counts
project = json.loads((ROOT/'kicad/device.kicad_pro').read_text())
assert project['text_variables']['AUTHOR']=='jayis1'
components = json.loads((ROOT/'kicad/connectivity.json').read_text())['components']
expected = ['LEFT','RIGHT','EXIT','CLEAR']
for i, label in enumerate(expected):
    part = next(c for c in components if c['ref']==f'J{i+3}')
    assert part['nets']==[label,'GND']
print(f'PASS {len(files)} attributed files; README {words} words; substantive C={counts["c"]}, headers={counts["h"]}')
