#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce local acceptance gates; no hardware programming or network writes."""
from pathlib import Path
import re
import subprocess
import sys
root=Path(__file__).resolve().parents[1]
def run(*args):
    subprocess.run(args,cwd=root,check=True)
run('make','-C','firmware','clean','all','test','fixture')
run(sys.executable,'tests/test_app.py')
run('xvfb-run','-a',sys.executable,'tests/test_gui.py')
run(sys.executable,'app/workbench.py','--check','firmware/build/demo.vw1')
run('/usr/bin/python3','tools/check_hardware.py')
paths=[p for p in root.rglob('*') if p.is_file() and not any(
    part in ('build','__pycache__') for part in p.relative_to(root).parts)
    and p.suffix not in ('.pyc','.kicad_prl')]
for p in paths:
    assert 'jayis1' in p.read_text(),str(p)
words=len((root/'README.md').read_text().split())
assert words>=2000,words
cfiles=list((root/'firmware').rglob('*.c'))
text='\n'.join(p.read_text() for p in cfiles)
text=re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)
lines=sum(bool(line.strip()) for line in text.splitlines())
assert lines>=500,lines
for path in ['firmware/main.c','firmware/board.h','firmware/registers.h','firmware/Makefile',
             'kicad/device.kicad_sch','kicad/device.kicad_pcb','kicad/device.kicad_pro',
             'app/workbench.py']:
    assert (root/path).is_file(),path
run('git','diff','--check')
print(f'PASS: {words} README words, {lines} nonblank noncomment production .c lines, {len(paths)} authored files')
