#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Source-count, attribution, required artifact and interface checks."""
from pathlib import Path
import json
import re
root=Path(__file__).resolve().parents[1]
files=[p for p in root.rglob('*') if p.is_file() and not any(
    part in ('build','__pycache__') for part in p.relative_to(root).parts)
    and p.suffix not in ('.pyc','.kicad_prl')]
for p in files:
    text=p.read_text()
    assert 'jayis1' in text, f'missing author: {p}'

required=['firmware/main.c','firmware/board.h','firmware/registers.h','firmware/Makefile',
          'kicad/device.kicad_sch','kicad/device.kicad_pcb','kicad/device.kicad_pro',
          'app/workbench.py','app/model.py','README.md']
for name in required: assert (root/name).is_file(),name
sources=list((root/'firmware').rglob('*.c'))+list((root/'firmware').rglob('*.h'))
text='\n'.join(p.read_text() for p in sources)
without_comments=re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)
lines=sum(bool(line.strip()) for line in without_comments.splitlines())
words=len((root/'README.md').read_text().split())
assert lines>=500,lines
assert words>=2000,words
manifest=json.loads((root/'kicad/connectivity.json').read_text())
lookup={c['ref']:c for c in manifest['components']}
for i,name in enumerate(('A_SRC','A_DST','B_SRC','B_DST')):
    assert lookup[f'J{i+3}']['nets']==[name,'GND']
    assert lookup[f'R{2*i+2}']['nets']==[f'SENSE{i}',name]
    assert lookup[f'R{2*i+1}']['nets']==['VCC',f'SENSE{i}']
assert lookup['J2']['nets']==['SENSE0','SENSE1','SENSE2','SENSE3']
assert 'CT1' in (root/'app/transport.py').read_text()
print(f'PASS: {len(files)} authored text files; {len(text.splitlines())} C/header lines; {lines} nonblank noncomment lines; {words} README words; connector contract')
