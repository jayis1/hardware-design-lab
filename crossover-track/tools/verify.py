#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Run from any directory; generated binary outputs remain ignored."""
from pathlib import Path
import re
import subprocess
ROOT = Path(__file__).resolve().parents[1]

def run(*args):
    subprocess.run(args, cwd=ROOT, check=True)

run('make','-C','firmware','check')
run('python3','-m','unittest','discover','-s','tests','-p','test_app.py')
run('xvfb-run','-a','python3','tests/gui_smoke.py')
run('python3','app/app.py','--check-replay','firmware/build/fixture.txt')
run('/usr/bin/python3','tools/check_hardware.py')
words = len((ROOT/'README.md').read_text().split())
lines = 0
for path in (ROOT/'firmware').rglob('*'):
    if path.suffix in ('.c','.h'):
        code = re.sub(r'/\*.*?\*/','',path.read_text(),flags=re.S)
        code = re.sub(r'//[^\n]*','',code)
        lines += sum(bool(line.strip()) for line in code.splitlines())
assert words >= 2000, words
assert lines >= 500, lines
files = subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','--',str(ROOT)],cwd=ROOT.parent,text=True).splitlines()
for filename in set(files):
    p = ROOT.parent / filename
    assert p.stat().st_size < 1_000_000, p
    text = p.read_text()
    assert 'jayis1' in text, p
    assert not re.search(r'ghp_[A-Za-z0-9]{36}|github_pat_[A-Za-z0-9_]{50}|-----BEGIN [A-Z ]*PRIVATE KEY-----|AKIA[0-9A-Z]{16}|xox[bp]-[0-9]{10}',text), p
    assert not re.search(r'^(?:[/#*\s]|<!--)*(?:author|creator|copyright)[^\n]{0,80}\b(?:Hermes|Nous Research|Anthropic|Claude|OpenAI)',text,re.I | re.M), p
run('git','diff','--check')
print(f'PASS: {words} README words; {lines} substantive C/header lines; {len(set(files))} authored source/design files')
