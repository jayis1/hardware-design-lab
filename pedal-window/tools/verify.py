#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Run the device's local release gates; no network or Git writes."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parent


def run(*args):
    subprocess.run(args, cwd=REPO, check=True)


run('make', '-C', str(ROOT/'firmware'), 'check')
run('xvfb-run', '-a', 'python3', '-m', 'unittest', 'discover', '-s', str(ROOT/'tests'), '-p', 'test_*.py')
run('/usr/bin/python3', str(ROOT/'tools/check_hardware.py'))
paths = subprocess.check_output(['git', 'ls-files', '--cached', '--others',
                                 '--exclude-standard', '--', ROOT.name], cwd=REPO, text=True).splitlines()
paths = sorted(set(paths))
for relative in paths:
    text = (REPO/relative).read_text()
    assert 'jayis1' in text, 'Missing author: ' + relative
    authors = re.findall(r'(?im)^(?:\s*(?:#|/\*|//|<!--)\s*)?Author(?: and creator)?\s*:\s*(.*)$', text)
    assert all('jayis1' in author for author in authors), relative
    assert not re.search(r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----', text), relative
    assert not re.search(r'gh[pousr]_[A-Za-z0-9]{30,}', text), relative
    assert not any(part in ('build', '__pycache__', 'node_modules') for part in Path(relative).parts), relative
words = len((ROOT/'README.md').read_text().split())
sources = list((ROOT/'firmware').rglob('*.c'))
lines = sum(len(path.read_text().splitlines()) for path in sources)
substantive = sum(sum(bool(line.strip()) for line in re.sub(r'/\*.*?\*/', '', path.read_text(), flags=re.S).splitlines()) for path in sources)
assert words >= 2000, words
assert substantive >= 500, substantive
run('git', 'diff', '--check')
print(f'PASS: {len(paths)} attributed source/design files; README {words} words; C {lines} lines, {substantive} nonblank/noncomment lines')
