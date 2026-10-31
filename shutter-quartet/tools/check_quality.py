#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Mechanical delivery gates; not a substitute for builds or physical testing."""
from pathlib import Path
import json
import re
import subprocess
root=Path(__file__).resolve().parents[1]
repo=root.parent
listed=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard',str(root)],cwd=repo,text=True)
files=sorted({repo/p for p in listed.splitlines() if (repo/p).is_file()})
assert files,'no deliverables'
for path in files:
    text=path.read_text()
    assert 'jayis1' in text, f'author missing: {path}'
    assert not re.search(r'(?im)^[#/*\s]*(?:author|creator|copyright)[^\n]*(?:Hermes|Nous Research|Anthropic|Claude|OpenAI)',text),path
    assert not re.search(r'(?:ghp_|github_pat_|sk-proj-)[A-Za-z0-9_]{15,}',text),path
    assert path.suffix not in ('.elf','.bin','.o','.pyc'),path
words=len((root/'README.md').read_text().split())
assert words>=2000,words
lines=0
for p in (root/'firmware').rglob('*'):
    if p.suffix in ('.c','.h'):
        text=re.sub(r'/\*.*?\*/|//[^\n]*','',p.read_text(),flags=re.S)
        lines+=sum(bool(line.strip()) for line in text.splitlines())
assert lines>=500,lines
required=['firmware/main.c','firmware/Makefile','firmware/board.h','firmware/registers.h',
          'kicad/device.kicad_sch','kicad/device.kicad_pcb','kicad/device.kicad_pro',
          'app/workbench.py','app/model.py','app/transport.py']
assert all((root/p).is_file() for p in required)
manifest=json.loads((root/'kicad/connectivity.json').read_text())
parts={c['ref']:c for c in manifest['components']}
for i in range(4):
    assert parts[f'J{i+3}']['nets']==['LIGHT_'+'ABCD'[i],'GND']
    assert parts[f'R{2*i+1}']['value']=='10k'
    assert parts[f'R{2*i+2}']['value']=='1k'
assert parts['J2']['nets']==['SENSE0','SENSE1','SENSE2','SENSE3']
print(f'PASS author/scope/artifact gates: {len(files)} files, {words} README words, {lines} substantive C/header lines')
