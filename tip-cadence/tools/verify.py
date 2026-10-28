#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce compile, test, attribution and EDA gates; no network or flashing."""
from pathlib import Path
import re
import subprocess
ROOT=Path(__file__).resolve().parents[1]
def run(*args):
    subprocess.run(args,cwd=ROOT,check=True)
run('make','-C','firmware','clean','all','test')
run('xvfb-run','-a','python3','tests/test_app.py')
run('/usr/bin/python3','tools/check_hardware.py')
words=len((ROOT/'README.md').read_text().split())
sources=list((ROOT/'firmware').rglob('*.c'))
text='\n'.join(p.read_text() for p in sources)
text=re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)
lines=sum(bool(line.strip()) for line in text.splitlines())
assert words>=2000 and lines>=500,(words,lines)
required=['firmware/main.c','firmware/board.h','firmware/registers.h',
          'firmware/Makefile','app/cadence.py','kicad/device.kicad_sch',
          'kicad/device.kicad_pcb','kicad/device.kicad_pro']
assert all((ROOT/path).is_file() for path in required)
checked=0
for p in ROOT.rglob('*'):
    if not p.is_file() or any(x in p.parts for x in ('build','__pycache__')):
        continue
    if p.suffix=='.kicad_prl':
        continue
    assert 'jayis1' in p.read_text(),str(p)
    checked+=1
print(f'PASS content: {words} README words; {lines} nonblank/comment-free .c lines; {checked} attributed files')
