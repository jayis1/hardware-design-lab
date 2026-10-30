#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Mechanical completeness checks; not physical qualification."""
from pathlib import Path
import re
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REQUIRED = ['README.md', 'VERIFICATION.md', 'firmware/main.c', 'firmware/engine.c',
            'firmware/board.h', 'firmware/registers.h', 'firmware/Makefile',
            'firmware/drivers/board.c', 'firmware/drivers/clock.c',
            'firmware/drivers/serial.c', 'kicad/device.kicad_sch',
            'kicad/device.kicad_pcb', 'kicad/device.kicad_pro',
            'app/workbench.py', 'app/protocol.py']
for name in REQUIRED:
    assert (ROOT/name).is_file(), name
words = len((ROOT/'README.md').read_text().split())
assert words >= 2000, words
lines = 0
for file in (ROOT/'firmware').rglob('*'):
    if file.suffix in ('.c', '.h'):
        text = re.sub(r'/\*.*?\*/|//[^\n]*', '', file.read_text(), flags=re.S)
        lines += sum(bool(line.strip()) for line in text.splitlines())
assert lines >= 500, lines
paths = subprocess.check_output(['git', 'ls-files', '--cached', '--others',
                                 '--exclude-standard', '--', str(ROOT)], text=True).splitlines()
for path in paths:
    file = Path(path)
    assert not file.is_symlink(), path
    text = file.read_text()
    assert 'jayis1' in text, f'Missing author: {path}'
    assert not re.search(r'-----BEGIN (?:RSA |OPENSSH )?PRIVATE KEY', text), path
    assert not re.search(r'(?:api_key|password|token)\s*=\s*[\"\'][^\"\']{6,}', text, re.I), path
print(f'PASS: {words} README words; {lines} nonblank/noncomment C/header lines; '
      f'{len(paths)} attributed source files; required paths present')
