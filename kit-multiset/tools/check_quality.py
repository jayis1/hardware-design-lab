#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Format/attribution gates plus real target, host, GUI and CAD verification."""
from pathlib import Path
import re
import subprocess
import sys
root=Path(__file__).resolve().parents[1]
repo=root.parent
files=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','--',str(root)],cwd=repo,text=True).splitlines()
files=sorted(set(files))
assert files, 'no source files'
for name in files:
    path=repo/name
    assert path.is_file() and not path.is_symlink(), name
    text=path.read_text()
    assert 'jayis1' in text, 'missing author: '+name
    assert not re.search('-----' + r'BEGIN .*PRIVATE KEY-----',text), name
    if path.suffix in ('.c','.h') or path.parent.name=='app':
        assert not re.search(r'\b(?:TODO|FIXME)\b',text), name
cfiles=list((root/'firmware').rglob('*.c'))+list((root/'firmware').rglob('*.h'))
source='\n'.join(p.read_text() for p in cfiles)
stripped=re.sub(r'/\*.*?\*/|//[^\n]*','',source,flags=re.S)
lines=sum(bool(line.strip()) for line in stripped.splitlines())
words=len((root/'README.md').read_text().split())
assert lines>=500,lines
assert words>=2000,words
print(f'PASS {len(files)} attributed source files; {lines} nonblank/noncomment C/header lines; {words} README words',flush=True)
for cmd in [
    ['make','-C',str(root/'firmware'),'clean','all','test'],
    ['xvfb-run','-a',sys.executable,'-m','unittest','discover','-s',str(root/'tests'),'-p','test_*.py'],
    ['/usr/bin/python3',str(root/'tools/check_hardware.py')],
    ['git','diff','--check']
]:
    subprocess.run(cmd,cwd=repo,check=True)
print('PASS: all reproducible quality gates')
