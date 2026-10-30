#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Run measurable source gates without counting builds, caches or tests as firmware."""
from pathlib import Path
import re
import subprocess
ROOT = Path(__file__).resolve().parents[1]
words = len((ROOT/'README.md').read_text().split())
assert words >= 2000, words
lines = 0
for path in (ROOT/'firmware').rglob('*'):
    if path.suffix in ('.c', '.h'):
        source = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(), flags=re.S)
        lines += sum(bool(line.strip()) for line in source.splitlines())
assert lines >= 500, lines
files = subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard',str(ROOT)], cwd=ROOT).decode().splitlines()
assert files
for name in files:
    path = Path(name) if Path(name).is_absolute() else ROOT / name
    content = path.read_text()
    assert 'jayis1' in content, str(path)

print(f'PASS: {words} README words; {lines} nonblank/noncomment firmware C/header lines; {len(files)} attributed files')
