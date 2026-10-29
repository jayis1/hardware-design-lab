#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Source-package acceptance gates; does not replace builds or CAD checks."""
import json
from pathlib import Path
import re

root=Path(__file__).resolve().parents[1]
required=['README.md','LICENSE','firmware/main.c','firmware/board.h',
          'firmware/registers.h','firmware/Makefile','app/scoop.py',
          'kicad/device.kicad_sch','kicad/device.kicad_pcb','kicad/device.kicad_pro']
for name in required:
    assert (root/name).is_file(),name
files=[p for p in root.rglob('*') if p.is_file()
       and not any(part in ('build','__pycache__') or part.endswith('-backups') for part in p.parts)
       and p.suffix not in ('.pyc','.kicad_prl')]
for path in files:
    text=path.read_text()
    assert 'jayis1' in text,f'missing author: {path}'
    if path.resolve()==Path(__file__).resolve():
        continue  # This checker contains the forbidden search vocabulary.
    assert not re.search(r'Nous Research|Anthropic|OpenAI|ChatGPT|Claude|Your Name',text),path
    assert not re.search(r'\bTODO\b|\bFIXME\b',text),path
words=len((root/'README.md').read_text().split())
assert words>=2000,words
sources=sorted(root.glob('firmware/**/*.c'))+sorted(root.glob('firmware/**/*.h'))
raw_lines=sum(len(p.read_text().splitlines()) for p in sources)
substantive=0
for path in sources:
    text=re.sub(r'/\*.*?\*/|//[^\n]*','',path.read_text(),flags=re.S)
    substantive+=sum(bool(line.strip()) for line in text.splitlines())
assert substantive>=500,substantive
manifest=json.loads((root/'kicad/connectivity.json').read_text())
project=json.loads((root/'kicad/device.kicad_pro').read_text())
assert project['text_variables']['AUTHOR']=='jayis1'
board=(root/'firmware/board.h').read_text()
parts={c['ref']:c for c in manifest['components']}
for i,name in enumerate(('DOCK_A','LID_A','DOCK_B','LID_B')):
    assert parts[f'J{i+3}']['nets']==[name,'GND']
    assert f'J{i+3}={name}' in board
    assert parts[f'R{2*i+1}']['nets']==['VCC',f'SENSE{i}']
    assert parts[f'R{2*i+2}']['nets']==[f'SENSE{i}',name]
assert parts['J2']['nets']==[f'SENSE{i}' for i in range(4)]
print(f'PASS {len(files)} authored files; README {words} words; production C/header {raw_lines} total, {substantive} nonblank/noncomment lines; pin contract checked')
