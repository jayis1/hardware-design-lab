#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Content thresholds, attribution, fixed wiring and protocol contract checks."""
from pathlib import Path
import json
import re
import subprocess
import sys
root=Path(__file__).resolve().parents[1]
words=len((root/'README.md').read_text().split())
assert words>=2000,words
lines=0
for p in (root/'firmware').rglob('*'):
    if p.suffix in ('.c','.h'):
        text=re.sub(r'/\*.*?\*/|//[^\n]*','',p.read_text(),flags=re.S)
        lines+=sum(bool(line.strip()) for line in text.splitlines())
assert lines>=500,lines
# Ask git which deliverables are ignored rather than maintaining a second list.
files=[]
for p in root.rglob('*'):
    if not p.is_file(): continue
    if subprocess.run(['git','check-ignore','-q',str(p)],cwd=root).returncode==0: continue
    text=p.read_text()
    assert 'jayis1' in text,p
    assert not re.search(r'(?im)^[\s#/*<(]*(author|creator|copyright)[^\n]*(?:Nous Research|Anthropic|Claude|Hermes)',text),p
    assert not re.search(r'(?m)^-----BEGIN .*PRIVATE KEY-----|gh[pousr]_[A-Za-z0-9]{30,}',text),p
    files.append(p)
manifest=json.loads((root/'kicad/connectivity.json').read_text())
components={c['ref']:c for c in manifest['components']}
assert components['J1']['nets']==['VCC','GND']
assert components['J2']['nets']==['SENSE0','SENSE1','SENSE2','SENSE3']
for i,name in enumerate(('FACE_A','FACE_B','PRESS','LATCH')):
    assert components[f'J{i+3}']['nets']==[name,'GND']
    assert components[f'R{2*i+1}']['nets']==['VCC',f'SENSE{i}']
    assert components[f'R{2*i+2}']['nets']==[f'SENSE{i}',name]
    assert components[f'R{2*i+1}']['value']=='10k'
    assert components[f'R{2*i+2}']['value']=='1k'
sys.path.insert(0,str(root/'app'))
from model import FIELDS
wire=(root/'firmware/wire.c').read_text().split('const uint32_t values[] = {',1)[1].split('};',1)[0]
assert re.findall(r'e->(\w+)',wire)==FIELDS
print(f'PASS: {words} README words; {lines} nonblank noncomment firmware C/header lines; {len(files)} attributed files; pin manifest and wire fields agree')
