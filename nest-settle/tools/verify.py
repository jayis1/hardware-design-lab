#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproducible software/CAD gates; no physical-validation claim."""
from pathlib import Path
import re
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[1]
def run(*args):
    subprocess.run(args, cwd=ROOT, check=True)
words = len((ROOT / 'README.md').read_text().split())
assert words >= 2000, words
counts = {}
for extension in ('.c', '.h'):
    total = 0
    for path in (ROOT / 'firmware').rglob('*' + extension):
        text = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(), flags=re.S)
        total += sum(bool(line.strip()) for line in text.splitlines())
    counts[extension] = total
assert counts['.c'] >= 500, counts
# Ignore only declared build and local-preference products, not deliverables.
for path in ROOT.rglob('*'):
    if not path.is_file() or any(p in ('build', '__pycache__') for p in path.parts):
        continue
    if path.suffix in ('.pyc', '.kicad_prl'):
        continue
    assert 'jayis1' in path.read_text(), path
print(f'PASS README {words} words; nonblank/comment-free C {counts}', flush=True)
run('make', '-C', 'firmware', 'all', 'test')
run(sys.executable, '-m', 'py_compile', 'app/nest.py')
run(sys.executable, 'tests/test_app.py')
run('xvfb-run', '-a', sys.executable, 'tests/test_gui.py')
run('/usr/bin/python3', 'tools/check_hardware.py')
print('PASS all reproducible gates; physical hardware remains unverified')
