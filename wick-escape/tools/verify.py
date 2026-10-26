#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Run reproducible software and CAD gates; never implies physical testing."""
from pathlib import Path
import re
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[1]

def run(*args):
    subprocess.run(args, cwd=ROOT, check=True)

def main():
    words = len((ROOT/'README.md').read_text().split())
    assert words >= 2000, words
    counts = {'.c': 0, '.h': 0}
    for path in (ROOT/'firmware').rglob('*'):
        if path.suffix in counts:
            source = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(), flags=re.S)
            counts[path.suffix] += sum(bool(line.strip()) for line in source.splitlines())
    assert counts['.c'] >= 500, counts
    tracked = subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard',
                                      '--','wick-escape/'],cwd=ROOT.parent,text=True).splitlines()
    for relative in tracked:
        text = (ROOT.parent/relative).read_text()
        assert 'jayis1' in text, relative

    print(f'Author: jayis1; files {len(tracked)}; README words {words}; substantive lines {counts}',flush=True)
    run('make','-C','firmware','all','test')
    run(sys.executable,'tests/test_app.py')
    run('xvfb-run','-a',sys.executable,'tests/test_gui.py')
    run('/usr/bin/python3','tools/check_hardware.py')
    print('PASS: source, target link, host tests, GUI/PTY simulation, ERC/DRC and connectivity')

if __name__ == '__main__':
    main()
