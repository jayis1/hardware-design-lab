#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Check release scope, format-supported attribution and numerical gates."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
required = ['README.md', 'VALIDATION.md', 'LICENSE', 'firmware/main.c',
            'firmware/board.h', 'firmware/registers.h', 'firmware/Makefile',
            'firmware/drivers/board.c', 'firmware/drivers/serial.c',
            'app/workbench.py', 'app/model.py', 'app/transport.py',
            'kicad/device.kicad_sch', 'kicad/device.kicad_pcb', 'kicad/device.kicad_pro']
for name in required:
    assert (ROOT / name).is_file(), name
files = subprocess.check_output(['git', 'ls-files', '--cached', '--others',
                                 '--exclude-standard', '--', ROOT.name],
                                cwd=ROOT.parent, text=True).splitlines()
files = sorted(set(files))
assert files
for name in files:
    path = ROOT.parent / name
    assert not path.is_symlink(), name
    text = path.read_text()
    assert 'jayis1' in text, 'Missing author: ' + name
    assert not re.search(r'(?:author|creator)\s*[:=]\s*(?:Hermes|Nous|Anthropic|Claude)', text, re.I), name
words = len((ROOT / 'README.md').read_text().split())
assert words >= 2000, words
sources = list((ROOT / 'firmware').rglob('*.c'))
headers = list((ROOT / 'firmware').rglob('*.h'))
def lines(paths):
    count = 0
    for path in paths:
        text = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(), flags=re.S)
        count += sum(bool(line.strip()) for line in text.splitlines())
    return count
assert lines(sources) >= 500, lines(sources)
# The theoretical largest frame fits both firmware and host limits.
import sys
sys.path.insert(0, str(ROOT / 'app'))
from model import FIELDS
maximum = len('TT1') + len(FIELDS)*11 + len('*FFFF\n')
assert maximum < 1024
# No accidental protocol marker reuse by another device's companion.
for path in ROOT.parent.glob('*/app/transport.py'):
    if path.parent.parent != ROOT:
        assert "b'TT1,'" not in path.read_text(), str(path)
subprocess.run(['git', 'diff', '--check'], cwd=ROOT.parent, check=True)
print(f'PASS: {len(files)} authored release files; README {words} words; '
      f'C {lines(sources)} substantive lines, C/header {lines(sources+headers)}; '
      f'{len(FIELDS)} fields, maximum frame {maximum} bytes')
