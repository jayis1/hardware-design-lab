#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce software/CAD gates; never flashes or claims physical validation."""
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]

def run(*args):
    print('+ '+' '.join(map(str,args)),flush=True)
    subprocess.run(list(map(str,args)),cwd=ROOT,check=True)

def main():
    run('make','-C','firmware','clean','all','test')
    flags=['gcc','-std=c11','-Wall','-Wextra','-Werror','-Ifirmware']
    for source,name in [('test_oracle','oracle'),('fixture','fixture')]:
        run(*flags,'-fsanitize=address,undefined',f'tests/{source}.c',
            'firmware/engine.c','firmware/wire.c','-o',f'firmware/build/{name}')
    run('firmware/build/oracle')
    with (ROOT/'firmware/build/demo.dl1').open('wb') as stream:
        subprocess.run([str(ROOT/'firmware/build/fixture')],stdout=stream,check=True,cwd=ROOT)
    run('xvfb-run','-a',sys.executable,'-m','unittest','discover','-s','tests','-p','test_*.py')
    run(sys.executable,'-m','compileall','-q','app','tools','tests')
    run(sys.executable,'tools/check_hardware.py')
    words=len((ROOT/'README.md').read_text().split())
    assert words>=2000,words
    lines=0
    for path in (ROOT/'firmware').rglob('*.c'):
        text=re.sub(r'/\*.*?\*/|//[^\n]*','',path.read_text(),flags=re.S)
        lines+=sum(bool(line.strip()) for line in text.splitlines())
    assert lines>=500,lines
    files=[]
    for path in ROOT.rglob('*'):
        if not path.is_file() or any(part in ('build','__pycache__') for part in path.parts):
            continue
        if path.suffix in ('.pyc','.kicad_prl'):
            continue
        assert not path.is_symlink(),path
        text=path.read_text()
        assert 'jayis1' in text,path
        files.append(path)
    run('git','diff','--check')
    print(f'PASS: README {words} whitespace-delimited words; {lines} nonblank/noncomment production C lines; '
          f'{len(files)} author-attributed deliverables')
    print('PASS: automated gates only; physical assembly, flashing and sensing remain unverified')

if __name__=='__main__':
    main()
