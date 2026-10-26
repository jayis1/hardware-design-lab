#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce software/CAD gates; does not flash hardware or publish commits."""
from pathlib import Path
import re
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[1]
def run(*args):
    subprocess.run(args, cwd=ROOT, check=True)
run('make', '-C', 'firmware', 'all', 'test')
run(sys.executable, 'tests/test_app.py')
run('xvfb-run', '-a', sys.executable, 'tests/test_gui.py')
run('/usr/bin/python3', 'tools/check_hardware.py')
readme = (ROOT / 'README.md').read_text()
words = len(re.sub(r'```.*?```', '', readme, flags=re.S).split())
assert words >= 2000, words
sources = list((ROOT / 'firmware').rglob('*.c')) + list((ROOT / 'firmware').rglob('*.h'))
code = '\n'.join(p.read_text() for p in sources)
code = re.sub(r'/\*.*?\*/|//[^\n]*', '', code, flags=re.S)
lines = sum(bool(line.strip()) for line in code.splitlines())
assert lines >= 500, lines
files = [p for p in ROOT.rglob('*') if p.is_file() and
         not any(part in ('build', '__pycache__') for part in p.parts) and
         p.suffix != '.kicad_prl']
for path in files:
    assert path.stat().st_size < 1_000_000, path
    assert 'jayis1' in path.read_text(), path
required = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
            'firmware/Makefile', 'kicad/device.kicad_sch',
            'kicad/device.kicad_pcb', 'kicad/device.kicad_pro', 'app/cam.py']
for path in required:
    assert (ROOT/path).is_file(), path
print(f'PASS {words} README words, {lines} nonblank comment-stripped C/header lines, {len(files)} attributed source/design files')
