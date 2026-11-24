#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Release structure, attribution and quantitative checks; not a hardware test."""
from pathlib import Path
import json
import re
import subprocess
ROOT = Path(__file__).resolve().parents[1]
required = ['README.md', 'firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
            'firmware/Makefile', 'kicad/device.kicad_sch', 'kicad/device.kicad_pcb',
            'kicad/device.kicad_pro', 'app/workbench.py']
for name in required:
    assert (ROOT / name).is_file(), name
words = len((ROOT / 'README.md').read_text().split())
assert words >= 2000, words
lines = 0
for path in (ROOT / 'firmware').rglob('*.c'):
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(), flags=re.S)
    lines += sum(bool(line.strip()) for line in text.splitlines())
assert lines >= 500, lines
# Git supplies the same exclusions used during scoped staging.
paths = subprocess.check_output(['git', 'ls-files', '--cached', '--others',
                                 '--exclude-standard', '--', ROOT.name],
                                cwd=ROOT.parent, text=True).splitlines()
for name in sorted(set(paths)):
    path = ROOT.parent / name
    assert not path.is_symlink(), name
    text = path.read_text()
    assert 'jayis1' in text, name
    assert not re.search(r'-----BEGIN (?:RSA |OPENSSH |EC )?PRIVATE KEY-----', text), name
    if path.suffix == '.json' or path.suffix == '.kicad_pro':
        json.loads(text)
manifest = json.loads((ROOT / 'kicad/connectivity.json').read_text())
assert manifest['author'] == 'jayis1'
assert len(manifest['components']) == 14
print(f'PASS {words} README words; {lines} nonblank noncomment firmware C lines; '
      f'{len(set(paths))} attributed deliverables')
