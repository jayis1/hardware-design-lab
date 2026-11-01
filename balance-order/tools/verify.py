#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce software and electrical CAD gates; never flash or publish."""
from pathlib import Path
import os
import re
import subprocess
import sys
root=Path(__file__).resolve().parents[1]
os.chdir(root)
def run(*args):
    subprocess.run(args,check=True)
run('make','-C','firmware','all','test')
run('gcc','-std=c11','-Wall','-Wextra','-Werror','-Ifirmware',
    'tests/fixture.c','firmware/engine.c','firmware/wire.c','-o','firmware/build/fixture')
with (root/'firmware/build/demo.bo1').open('wb') as output:
    subprocess.run(['firmware/build/fixture'],stdout=output,check=True)
run(sys.executable,'-m','compileall','-q','app','tests','tools')
run('xvfb-run','-a',sys.executable,'-m','unittest','discover','-s','tests','-p','test_*.py')
run('/usr/bin/python3','tools/check_hardware.py')
words=len((root/'README.md').read_text().split())
assert words>=2000,words
sources=list((root/'firmware').rglob('*.c'))+list((root/'firmware').rglob('*.h'))
text='\n'.join(p.read_text() for p in sources)
code=re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)
lines=sum(bool(line.strip()) for line in code.splitlines())
assert lines>=500,lines
count=0
for p in root.rglob('*'):
    if not p.is_file() or any(x in p.parts for x in ('build','__pycache__')) or p.suffix=='.kicad_prl':
        continue
    assert not p.is_symlink(),p
    content=p.read_text()
    assert 'jayis1' in content,('missing attribution',p)
    assert not re.search(r'BEGIN (?:RSA |OPENSSH )?PRIVATE KEY',content),p
    count+=1
print(f'PASS: {words} README words; {lines} substantive C/header lines; {count} attributed source files')
