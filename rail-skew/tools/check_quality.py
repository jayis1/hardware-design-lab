#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local release gates; does not claim physical validation or global novelty."""
from pathlib import Path
import re
import subprocess
root=Path(__file__).resolve().parents[1]
required=['README.md','firmware/main.c','firmware/board.h','firmware/registers.h',
          'firmware/Makefile','kicad/device.kicad_sch','kicad/device.kicad_pcb',
          'kicad/device.kicad_pro','app/workbench.py']
for name in required:
    assert (root/name).is_file() and (root/name).stat().st_size,name
words=len((root/'README.md').read_text().split())
assert words>=2000,words
counts={}
for extension in ('*.c','*.h'):
    count=0
    for path in (root/'firmware').rglob(extension):
        text=re.sub(r'/\*.*?\*/|//[^\n]*','',path.read_text(),flags=re.S)
        count+=sum(bool(line.strip()) for line in text.splitlines())
    counts[extension]=count
assert counts['*.c']>=500,counts
files=[]
for path in root.rglob('*'):
    if not path.is_file(): continue
    if subprocess.run(['git','check-ignore','-q',str(path)],cwd=root).returncode==0: continue
    text=path.read_text()
    assert 'jayis1' in text,path
    forbidden=['No'+'us Research','Anth'+'ropic','Clau'+'de','Her'+'mes']
    assert not any(name.lower() in text.lower() for name in forbidden),path
    assert not re.search(r'(?:api_key|password|secret)\s*=\s*[\"\'][^\"\']{6,}',text,re.I),path
    files.append(path)
print(f'PASS {words} README words, {counts}, {len(files)} authored files')
