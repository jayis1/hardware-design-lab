#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Mechanical acceptance checks; does not substitute for hardware testing."""
from pathlib import Path
import re
import subprocess
ROOT=Path(__file__).resolve().parents[1]
required=['README.md','firmware/main.c','firmware/board.h','firmware/registers.h',
          'firmware/Makefile','kicad/device.kicad_sch','kicad/device.kicad_pcb',
          'kicad/device.kicad_pro','app/workbench.py','app/protocol.py']
for name in required:
    assert (ROOT/name).is_file(), name
words=len((ROOT/'README.md').read_text().split())
assert words>=2000,words
lines=0
for path in (ROOT/'firmware').rglob('*'):
    if path.suffix in ('.c','.h'):
        text=re.sub(r'/\*.*?\*/|//[^\n]*','',path.read_text(),flags=re.S)
        lines+=sum(bool(line.strip()) for line in text.splitlines())
assert lines>=500,lines
files=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard',str(ROOT)],text=True).splitlines()
for name in files:
    text=Path(name).read_text()
    assert 'jayis1' in text, f'missing attribution: {name}'

    assert not re.search(r'^-----BEGIN .*PRIVATE KEY|ghp_[A-Za-z0-9]{20}',text,re.M),name
print(f'PASS: {words} README words, {lines} substantive C/header lines, {len(files)} attributed source files')
