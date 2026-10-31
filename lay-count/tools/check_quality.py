#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local acceptance gates; excludes ignored builds, preferences and bytecode."""
from pathlib import Path
import json
import re
import subprocess
root=Path(__file__).resolve().parents[1]
ignored={'build','__pycache__'}
files=[p for p in root.rglob('*') if p.is_file() and not ignored.intersection(p.parts)
       and p.suffix not in ('.pyc','.kicad_prl')]
for p in files:
    text=p.read_text()
    assert 'jayis1' in text, f'missing author: {p}'
    assert not re.search(r'(?i)(api_key|password|secret)\s*=\s*[\"\'][^\"\']{6,}',text),p
words=len((root/'README.md').read_text().split())
assert words>=2000,words
sources=list((root/'firmware').rglob('*.c'))+list((root/'firmware').rglob('*.h'))
text='\n'.join(p.read_text() for p in sources)
code=re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)
lines=sum(bool(line.strip()) for line in code.splitlines())
assert lines>=500,lines
assert not re.search(r'TODO|FIXME|implement me',text)
manifest=json.loads((root/'kicad/connectivity.json').read_text())
parts={c['ref']:c for c in manifest['components']}
assert parts['J2']['nets']==['SENSE0','SENSE1','SENSE2','SENSE3']
for ref,name in zip(('J3','J4','J5','J6'),('LEFT','RIGHT','INDEX','TENSION')):
    assert parts[ref]['nets']==[name,'GND']
json.loads((root/'kicad/device.kicad_pro').read_text())
subprocess.run(['git','diff','--check'],cwd=root,check=True)
print(f'PASS: {len(files)} authored deliverables; README {words} words; firmware {lines} nonblank noncomment C/header lines')
