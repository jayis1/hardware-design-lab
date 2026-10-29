#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Source floors, attribution, declared signal mapping and artifact hygiene."""
from pathlib import Path
import json
import re
import subprocess
root=Path(__file__).resolve().parents[1]
words=len((root/'README.md').read_text().split())
assert words>=2000,words
sources=sorted((root/'firmware').rglob('*.c'))+sorted((root/'firmware').rglob('*.h'))
substantive=0
c_only=0
for source in sources:
    code=re.sub(r'/\*.*?\*/|//[^\n]*','',source.read_text(),flags=re.S)
    lines=sum(bool(line.strip()) for line in code.splitlines())
    substantive+=lines
    if source.suffix=='.c': c_only+=lines
assert c_only>=500,c_only
paths=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard',str(root)],text=True).splitlines()
assert paths,'no files'
for name in paths:
    path=Path(name)
    assert path.is_file() and not path.is_symlink(),path
    text=path.read_text()
    assert 'jayis1' in text,path
    assert path.stat().st_size<1_000_000,path
    assert not re.search(r'^-----BEGIN .*PRIVATE KEY-----',text,re.M),path
manifest=json.loads((root/'kicad/connectivity.json').read_text())
components={c['ref']:c for c in manifest['components']}
for i,name in enumerate(('HOME','FIRST','SECOND','SOURCE')):
    assert components[f'J{i+3}']['nets']==[name,'GND']
    assert components[f'R{2*i+1}']['nets']==['VCC',f'SENSE{i}']
    assert components[f'R{2*i+2}']['nets']==[f'SENSE{i}',name]
    assert name in (root/'firmware/board.h').read_text()
assert components['J2']['nets']==['SENSE0','SENSE1','SENSE2','SENSE3']
for file in ('firmware/main.c','firmware/Makefile','firmware/board.h','firmware/registers.h','kicad/device.kicad_pcb','kicad/device.kicad_sch','kicad/device.kicad_pro','app/pipette.py'):
    assert (root/file).is_file(),file
print(f'PASS README={words} words; C-only={c_only}; C+headers={substantive} nonblank noncomment lines; {len(paths)} attributed files; pin contract')
