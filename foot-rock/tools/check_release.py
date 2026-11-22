#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Mechanical release gates, in addition to behavioral and KiCad checks."""
from pathlib import Path
import re
import subprocess
ROOT=Path(__file__).resolve().parents[1]
required=['README.md','firmware/main.c','firmware/board.h','firmware/registers.h',
          'firmware/Makefile','kicad/device.kicad_sch','kicad/device.kicad_pcb',
          'kicad/device.kicad_pro','app/workbench.py','VALIDATION.md']
for name in required:
    assert (ROOT/name).is_file(),name
words=len((ROOT/'README.md').read_text().split())
assert words>=2000,words
lines=0
for path in (ROOT/'firmware').rglob('*.c'):
    text=re.sub(r'/\*.*?\*/|//[^\n]*','',path.read_text(),flags=re.S)
    lines+=sum(bool(line.strip()) for line in text.splitlines())
assert lines>=500,lines
files=[]
for path in ROOT.rglob('*'):
    if not path.is_file() or 'build' in path.parts or '__pycache__' in path.parts:
        continue
    if path.suffix=='.kicad_prl': continue
    text=path.read_text()
    assert 'jayis1' in text,path
    files.append(path)
# Assertions here compare the interface with the authored manifest, not merely
# with duplicate strings in two documents. The CAD checker compares real pads.
import json
components=json.loads((ROOT/'kicad/connectivity.json').read_text())['components']
by_ref={c['ref']:c for c in components}
assert by_ref['J1']['nets']==['VCC','GND']
assert by_ref['J2']['nets']==['SENSE0','SENSE1','SENSE2','SENSE3']
for i,name in enumerate(('FOOT_NW','FOOT_NE','FOOT_SW','FOOT_SE')):
    assert by_ref[f'J{i+3}']['nets']==[name,'GND']
    assert by_ref[f'R{2*i+1}']['nets']==['VCC',f'SENSE{i}']
    assert by_ref[f'R{2*i+2}']['nets']==[f'SENSE{i}',name]
subprocess.run(['git','diff','--check'],cwd=ROOT,check=True)
print(f'PASS: {words} README words, {lines} nonblank noncomment C lines, {len(files)} attributed files')
