#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Quantitative/source-contract checks; complements builds, tests and CAD."""
import ast
import json
from pathlib import Path
import re
import subprocess
ROOT = Path(__file__).resolve().parents[1]
words = len((ROOT/'README.md').read_text().split())
assert words >= 2000, words
sources = sorted((ROOT/'firmware').rglob('*.c')) + sorted((ROOT/'firmware').rglob('*.h'))
code = '\n'.join(p.read_text() for p in sources)
substantive = re.sub(r'/\*.*?\*/|//[^\n]*', '', code, flags=re.S)
lines = sum(bool(line.strip()) for line in substantive.splitlines())
assert lines >= 500, lines
for path in ROOT.rglob('*'):
    if not path.is_file() or any(part in ('build','__pycache__') for part in path.parts):
        continue
    if path.suffix == '.kicad_prl':
        continue
    assert not path.is_symlink(), path
    text = path.read_text()
    assert 'jayis1' in text, f'Missing authorship: {path}'
    assert path.stat().st_size < 1048576, path
    if path.suffix == '.py': ast.parse(text)
    if path.suffix in ('.json','.kicad_pro'): json.loads(text)
    assert not re.search(r'BEGIN (?:RSA |OPENSSH )?PRIVATE KEY|AKIA[A-Z0-9]{16}',text), path
wire = (ROOT/'firmware/wire.c').read_text()
assert wire.count('number(&w,') == 25
sys_path = ROOT/'app/protocol.py'
module = ast.parse(sys_path.read_text())
assert any(isinstance(n, ast.ClassDef) and n.name == 'Session' for n in module.body)
manifest = json.loads((ROOT/'kicad/connectivity.json').read_text())
parts = {p['ref']:p for p in manifest['components']}
assert parts['J2']['nets'] == ['SENSE0','SENSE1','SENSE2','SENSE3']
for i, name in enumerate(('DISCHARGE','SUPPLY','INTERVENE','ENABLE')):
    assert parts[f'J{i+3}']['nets'] == [name,'GND']
    assert parts[f'R{2*i+1}']['value'] == '10k'
    assert parts[f'R{2*i+2}']['value'] == '1k'
print(f'PASS README {words} words; production C/header {lines} nonempty noncomment lines; authorship, sizes, syntax, metadata, protocol and carrier contract')
