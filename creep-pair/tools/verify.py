#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce software/CAD acceptance gates, not physical qualification."""
from pathlib import Path
import re
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]
def run(*args):
    subprocess.run(args,cwd=ROOT,check=True)
def main():
    words=len((ROOT/'README.md').read_text().split())
    assert words>=2000,words
    counts={'.c':0,'.h':0}
    for ext in counts:
        for p in (ROOT/'firmware').rglob('*'+ext):
            source=re.sub(r'/\*.*?\*/|//[^\n]*','',p.read_text(),flags=re.S)
            counts[ext]+=sum(bool(line.strip()) for line in source.splitlines())
    assert counts['.c']>=500,counts
    files=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','--','creep-pair/'],cwd=ROOT.parent,text=True).splitlines()
    for f in files:
        text=(ROOT.parent/f).read_text()
        assert 'jayis1' in text,f
        assert not re.search(r'-----BEGIN [A-Z ]*PRIVATE KEY-----|ghp_[A-Za-z0-9]{20}',text),f
    print(f'Author jayis1; files={len(files)}, README words={words}, substantive lines={counts}',flush=True)
    run('make','-C','firmware','all','test')
    run('xvfb-run','-a',sys.executable,'tests/test_app.py')
    run('/usr/bin/python3','tools/check_hardware.py')
    run('git','diff','--check')
    print('PASS: target link, host engine, GUI/PTY, ERC, DRC, net consistency and source gates')
if __name__=='__main__': main()
