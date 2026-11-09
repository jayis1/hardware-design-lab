#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce native builds, synthetic tests, CAD checks and source acceptance."""
from pathlib import Path
import re
import subprocess
ROOT=Path(__file__).resolve().parents[1]
def run(*args):
    subprocess.run(args,cwd=ROOT,check=True)
run('make','-C','firmware','clean','all','test','fixture')
run('xvfb-run','-a','/usr/bin/python3','-m','unittest','discover','-s','tests','-p','test_*.py')
run('/usr/bin/python3','-m','compileall','-q','app','tests','tools')
run('/usr/bin/python3','tools/check_hardware.py')
words=len((ROOT/'README.md').read_text().split())
assert words>=2000,words
counts={}
for suffix in ('*.c','*.h'):
    counts[suffix]=sum(sum(bool(line.strip()) for line in re.sub(
        r'/\*.*?\*/|//[^\n]*','',path.read_text(),flags=re.S).splitlines())
        for path in (ROOT/'firmware').rglob(suffix))
assert counts['*.c']>=500,counts
required=['firmware/main.c','firmware/board.h','firmware/registers.h',
          'firmware/Makefile','kicad/device.kicad_sch','kicad/device.kicad_pcb',
          'kicad/device.kicad_pro','app/workbench.py','VALIDATION.md']
assert all((ROOT/p).is_file() for p in required)
paths=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','--','.'],cwd=ROOT,text=True).splitlines()
for name in paths:
    text=(ROOT/name).read_text()
    assert 'jayis1' in text,('missing attribution',name)
    assert not re.search(r'Author:\s*(?:Hermes|Claude|Anthropic|Nous)',text,re.I),name
print('PASS acceptance:',words,'README words;',counts,'substantive production lines;',len(paths),'authored files')
