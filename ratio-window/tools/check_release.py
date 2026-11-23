#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local release gates; excludes ignored outputs, never publishes anything."""
from pathlib import Path
import re
import subprocess
ROOT = Path(__file__).resolve().parents[1]

def run(*args):
    subprocess.run(args, cwd=ROOT, check=True)

run('make','-C','firmware','all','test','fixture')
run('xvfb-run','-a','python3','-m','unittest','discover','-s','tests','-p','test_*.py')
run('/usr/bin/python3','tools/check_hardware.py')
paths = [p for p in ROOT.rglob('*') if p.is_file() and
         'build' not in p.parts and '__pycache__' not in p.parts and
         p.suffix not in ('.pyc','.kicad_prl') and not p.name.startswith('~')]
for path in paths:
    text = path.read_text()
    assert 'jayis1' in text, f'Author missing: {path}'
    assert not re.search(r'(?:api_key|password|secret)\s*=\s*[\"\x27][^\"\x27]{6,}', text, re.I), path
words = len((ROOT/'README.md').read_text().split())
source = '\n'.join(p.read_text() for p in paths if p.suffix in ('.c','.h') and
                   'firmware' in p.parts)
source = re.sub(r'/\*.*?\*/','',source,flags=re.S)
lines = sum(bool(line.strip()) for line in source.splitlines())
assert words >= 2000, words
assert lines >= 500, lines
required = ['firmware/main.c','firmware/board.h','firmware/registers.h','firmware/Makefile',
            'kicad/device.kicad_sch','kicad/device.kicad_pcb','kicad/device.kicad_pro',
            'app/workbench.py','README.md','VALIDATION.md']
assert all((ROOT/p).is_file() for p in required)
run('git','diff','--check','--',str(ROOT))
print(f'PASS: {len(paths)} attributed source/design files; {words} README words; '
      f'{lines} nonblank firmware C/header lines excluding block comments')
