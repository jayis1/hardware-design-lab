#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Scope, attribution, minimum substance, and source hygiene gate."""
from pathlib import Path
import re
import subprocess
root=Path(__file__).resolve().parents[1]
repo=root.parent
names=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard',str(root)],cwd=repo,text=True).splitlines()
files=sorted({repo/name for name in names})
required=['README.md','LICENSE','VERIFICATION.md','firmware/main.c','firmware/board.h',
          'firmware/registers.h','firmware/Makefile','kicad/device.kicad_sch',
          'kicad/device.kicad_pcb','kicad/device.kicad_pro','app/quartet.py']
for name in required:
    assert (root/name).is_file(),name
for path in files:
    text=path.read_text()
    assert 'jayis1' in text, f'missing author: {path}'
    assert len(text)<1000000, f'excessive size: {path}'
    assert not re.search(r'-----BEGIN (?:RSA |OPENSSH |EC )?PRIVATE KEY-----',text),path
    assert not re.search(r'(?:api_key|password)\s*=\s*[\"\'][^\"\']{6,}',text,re.I),path
words=len((root/'README.md').read_text().split())
assert words>=2000,words
count=0
for path in list((root/'firmware').rglob('*.c'))+list((root/'firmware').rglob('*.h')):
    source=re.sub(r'/\*.*?\*/|//[^\n]*','',path.read_text(),flags=re.S)
    count+=sum(bool(line.strip()) for line in source.splitlines())
assert count>=500,count
print(f'PASS: {len(files)} authored source files; README {words} words; C/header {count} substantive lines')
