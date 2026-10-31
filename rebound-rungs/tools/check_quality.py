#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Deliverable thresholds, attribution, text-only scope and pin contract checks."""
from pathlib import Path
import json
import re
import subprocess
root=Path(__file__).resolve().parents[1]
repo=root.parent
names=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard',
                               '--',root.name],cwd=repo,text=True).splitlines()
paths=[repo/name for name in names]
assert paths, 'no deliverables'
unwanted=('Nous Research','Anthropic','Claude','Hermes')
attribution_pattern=r'(?i)(author|creator|copyright)[^\n]*(?:'+'|'.join(unwanted)+')'
for path in paths:
    assert path.is_file() and not path.is_symlink(),path
    assert path.stat().st_size<1_000_000,path
    text=path.read_text()
    assert 'jayis1' in text, f'author missing: {path}'
    assert '\0' not in text,path
    assert not re.search(r'(?i)(api_key|password|secret)\s*=\s*[\"\'][^\"\']{8,}',text),path
    assert not re.search(attribution_pattern,text),path
words=len((root/'README.md').read_text().split())
assert words>=2000,words
sources=list((root/'firmware').rglob('*.c'))
def substantive(path):
    text=re.sub(r'/\*.*?\*/|//[^\n]*','',path.read_text(),flags=re.S)
    return sum(bool(line.strip()) for line in text.splitlines())
lines=sum(map(substantive,sources))
assert lines>=500,lines
manifest=json.loads((root/'kicad/connectivity.json').read_text())
components={c['ref']:c for c in manifest['components']}
assert components['J1']['nets']==['VCC','GND']
assert components['J2']['nets']==[f'SENSE{i}' for i in range(4)]
for i in range(4):
    assert components[f'J{i+3}']['nets']==[f'RUNG{i}','GND']
    assert components[f'R{2*i+1}']['value']=='10k'
    assert components[f'R{2*i+2}']['value']=='1k'
assert 'rung 0, rung 1, rung 2, rung 3' in (root/'firmware/board.h').read_text()
print(f'PASS: {words} README words; {lines} substantive production C lines; '
      f'{len(paths)} attributed text deliverables; connector contract consistent')
