#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce software, CAD, attribution and requested content gates."""
from pathlib import Path
import re
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]

def run(*args):
    subprocess.run(args,cwd=ROOT,check=True)

def ignored(path):
    return ('build' in path.parts or '__pycache__' in path.parts
            or path.suffix in ('.pyc','.kicad_prl','.lck','.rpt'))

sources=[p for p in (ROOT/'firmware').rglob('*') if p.suffix in ('.c','.h') and not ignored(p)]
lines=sum(len(p.read_text().splitlines()) for p in sources)
substantive=0
for p in sources:
    source=re.sub(r'/\*.*?\*/|//[^\n]*','',p.read_text(),flags=re.S)
    substantive+=sum(bool(line.strip()) for line in source.splitlines())
words=len((ROOT/'README.md').read_text().split())
assert lines>=500 and substantive>=500,(lines,substantive)
assert words>=2000,words
files=[p for p in ROOT.rglob('*') if p.is_file() and not ignored(p)]
for p in files:
    text=p.read_text()
    assert 'jayis1' in text,p
run('make','-C','firmware','all','test')
run('xvfb-run','-a',sys.executable,'tests/test_app.py')
run('/usr/bin/python3','tools/check_hardware.py')
print(f'PASS: {words} README words; {lines} C/header lines; {substantive} nonblank noncomment C/header lines; {len(files)} attributed source/design files')
