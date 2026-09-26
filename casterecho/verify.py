#!/usr/bin/env python3
"""CasterEcho repository quality gate. Author: jayis1."""
# Copyright (c) 2026 jayis1; SPDX-License-Identifier: MIT
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).parent
files = [p for p in ROOT.rglob('*') if p.is_file() and not p.name.startswith('casterecho')]
assert len(files) >= 17, f"expected design set, found {len(files)} files"
for path in files:
    text = path.read_text(errors='ignore')
    assert 'jayis1' in text, f"missing author credit: {path.relative_to(ROOT)}"
readme = (ROOT / 'README.md').read_text()
words = re.findall(r"\b[\w'-]+\b", readme)
assert len(words) >= 2000, f"README has {len(words)} words"
firmware = list((ROOT / 'firmware').glob('*.c')) + list((ROOT / 'firmware').glob('*.h'))
lines = sum(len(path.read_text().splitlines()) for path in firmware)
assert lines >= 500, f"firmware has {lines} lines"
subprocess.run(['make', 'check'], cwd=ROOT / 'firmware', check=True)
subprocess.run(['node', '--test', 'test/protocol.test.js'], cwd=ROOT / 'app', check=True)
subprocess.run(['python3', 'validate.py'], cwd=ROOT / 'kicad', check=True)
print(f"CasterEcho quality gate PASS: {len(words)} README words, {lines} firmware lines, {len(files)} files")
