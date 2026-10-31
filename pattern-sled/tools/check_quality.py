#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Content thresholds and attribution for deliverable (not build/cache) files."""
from pathlib import Path
import re
import subprocess
ROOT=Path(__file__).resolve().parents[1]
files=[]
for p in ROOT.rglob('*'):
    if not p.is_file(): continue
    if subprocess.run(['git','check-ignore','-q',str(p)],cwd=ROOT).returncode==0: continue
    files.append(p)
for p in files:
    text=p.read_text()
    assert 'jayis1' in text, f'missing author: {p}'
    forbidden=['Nous'+' Research','Anthro'+'pic','Clau'+'de','Author: '+'Her'+'mes']
    assert not any(name.lower() in text.lower() for name in forbidden), p
    assert not re.search(r'(?:api_key|password|secret)\s*=\s*[\'"][^\'"]{6,}',text,re.I), p
words=len((ROOT/'README.md').read_text().split())
sources=list((ROOT/'firmware').rglob('*.c'))+list((ROOT/'firmware').rglob('*.h'))
lines=0; substantive=0
for p in sources:
    text=p.read_text(); lines+=len(text.splitlines())
    uncommented=re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)
    substantive+=sum(bool(line.strip()) for line in uncommented.splitlines())
assert words>=2000, words
assert substantive>=500, substantive
print(f'PASS {len(files)} authored deliverables; README {words} words; C/header {lines} lines, {substantive} nonblank uncommented')
