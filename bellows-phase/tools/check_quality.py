#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Measure source requirements and audit authored, non-generated products."""
from pathlib import Path
import json, re, subprocess
root=Path(__file__).resolve().parents[1]
files=[p for p in root.rglob('*') if p.is_file() and not any(
    part in ('build','__pycache__') for part in p.relative_to(root).parts)
    and p.suffix not in ('.pyc','.kicad_prl')]
for p in files:
    text=p.read_text()
    assert 'jayis1' in text, f'missing attribution: {p}'
    assert not re.search(r'gh[pousr]_[A-Za-z0-9]{20,}',text),p
    assert ('-----BEGIN '+'PRIVATE KEY') not in text,p
    if p.suffix in ('.json','.kicad_pro'): json.loads(text)
readme_words=len((root/'README.md').read_text().split())
cfiles=sorted((root/'firmware').rglob('*.c'))+sorted((root/'firmware').rglob('*.h'))
source='\n'.join(p.read_text() for p in cfiles)
lines=len(source.splitlines())
substantive=sum(bool(line.strip()) for line in re.sub(r'/\*.*?\*/','',source,flags=re.S).splitlines())
assert readme_words>=2000 and substantive>=500
manifest=json.loads((root/'kicad/connectivity.json').read_text())
components={c['ref']:c for c in manifest['components']}
assert components['J2']['nets']==['SENSE0','SENSE1','SENSE2','SENSE3']
for index,name in enumerate(('COMPRESSED','EXPANDED','INLET','OUTLET')):
    assert components[f'J{index+3}']['nets']==[name,'GND']
print(f'PASS: {len(files)} attributed source/design files; README {readme_words} words; C/header {lines} lines, {substantive} substantive')
subprocess.run(['git','diff','--check'],cwd=root,check=True)
