#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Check publication scope, authorship, source volume and artifact presence."""
from pathlib import Path
import re
import subprocess
root=Path(__file__).resolve().parents[1]
paths=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','--',root.name],cwd=root.parent,text=True).splitlines()
paths=sorted(set(paths))
assert paths
for name in paths:
    p=root.parent/name
    text=p.read_text()
    assert 'jayis1' in text, 'Missing author: '+name
    forbidden=['Nous'+' Research','Anth'+'ropic','Author: '+'Hermes']
    assert not any(value in text for value in forbidden), name
    assert not re.search(r'(api_key|password|secret)\s*=\s*[\'\"][^\'\"]{6,}',text,re.I),name
    assert not any(part in ('build','__pycache__','node_modules') for part in p.parts),name
required=['README.md','firmware/main.c','firmware/board.h','firmware/registers.h','firmware/Makefile',
          'kicad/device.kicad_sch','kicad/device.kicad_pcb','kicad/device.kicad_pro','app/workbench.py']
assert all((root/p).is_file() for p in required)
words=len((root/'README.md').read_text().split())
source='\n'.join(p.read_text() for p in sorted((root/'firmware').rglob('*')) if p.suffix in ('.c','.h'))
clean=re.sub(r'/\*.*?\*/|//[^\n]*','',source,flags=re.S)
lines=sum(bool(line.strip()) for line in clean.splitlines())
assert words>=2000,words
assert lines>=500,lines
print(f'PASS: {len(paths)} authored files, {words} README words, {lines} nonblank noncomment C/header lines')
