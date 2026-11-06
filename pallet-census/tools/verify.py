#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce software/CAD checks and acceptance counts without network access."""
from pathlib import Path
import re
import subprocess
ROOT = Path(__file__).resolve().parents[1]

def run(*command):
    subprocess.run(command, cwd=ROOT, check=True)

run('make', '-C', 'firmware', 'clean', 'all', 'test', 'fixture')
run('xvfb-run', '-a', '/usr/bin/python3', '-m', 'unittest', 'discover', '-s', 'tests', '-p', 'test_*.py')
run('/usr/bin/python3', 'tools/check_hardware.py')
words = len((ROOT / 'README.md').read_text().split())
assert words >= 2000, words
counts = {}
for suffix in ('*.c', '*.h'):
    count = 0
    for path in (ROOT / 'firmware').rglob(suffix):
        text = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(), flags=re.S)
        count += sum(bool(line.strip()) for line in text.splitlines())
    counts[suffix] = count
assert counts['*.c'] >= 500, counts
required = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
            'firmware/Makefile', 'kicad/device.kicad_sch', 'kicad/device.kicad_pcb',
            'kicad/device.kicad_pro', 'app/workbench.py']
assert all((ROOT / name).is_file() for name in required)
paths = subprocess.check_output(['git', 'ls-files', '--cached', '--others',
                                 '--exclude-standard', '--', '.'], cwd=ROOT, text=True).splitlines()
for name in paths:
    text = (ROOT / name).read_text()
    assert 'jayis1' in text, ('missing author', name)
    for line in text.splitlines():
        if re.match(r'^(?:#|/\*)?\s*(?:Author|Creator):', line, re.I):
            assert 'jayis1' in line, ('incorrect attribution', name)
print('PASS requirements:', words, 'README words;', counts, 'nonblank noncomment production lines;', len(paths), 'authored files')
