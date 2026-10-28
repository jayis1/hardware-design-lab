#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce software, CAD, content, and attribution gates without hardware."""
from pathlib import Path
import re
import subprocess
root=Path(__file__).resolve().parents[1]
for command in (["make","-C",str(root/'firmware'),"all","test"],
                ["xvfb-run","-a","python3",str(root/'tests/test_app.py')],
                ["/usr/bin/python3",str(root/'tools/check_hardware.py')]):
    subprocess.run(command,check=True)
words=len((root/'README.md').read_text().split())
assert words>=2000,words
code='\n'.join(p.read_text() for p in (root/'firmware').rglob('*') if p.suffix in ('.c','.h'))
stripped=re.sub(r'/\*.*?\*/|//[^\n]*','',code,flags=re.S)
lines=sum(bool(line.strip()) for line in stripped.splitlines())
assert lines>=500,lines
files=[]
for p in root.rglob('*'):
    if not p.is_file() or 'build' in p.parts or '__pycache__' in p.parts or p.suffix in ('.kicad_prl','.rpt'):
        continue
    text=p.read_text()
    assert 'jayis1' in text,p
    files.append(p)
print(f'PASS: {words} README words; {lines} nonblank noncomment C/header lines; {len(files)} attributed files')
