#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproducible source, attribution, thresholds and interface checks."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
paths = subprocess.check_output(['git', 'ls-files', '--cached', '--others',
                                 '--exclude-standard', str(root)], text=True).splitlines()
assert paths, 'No deliverables'
for name in paths:
    path = Path(name)
    text = path.read_text()
    assert 'jayis1' in text, f'Attribution missing: {name}'
    assert not re.search(r'^\s*(?:(?:#|/\*|\*|<!--)\s*)?(?:author(?: and creator)?|creator|copyright)\b[^\n]*(?:Hermes|Nous|Anthropic|Claude)', text, re.I | re.M), name
    assert not re.search(r'-----BEGIN (?:RSA |OPENSSH |EC )?PRIVATE KEY', text), name
    assert path.stat().st_size < 1024 * 1024, name
words = len((root/'README.md').read_text().split())
assert words >= 2000
counts = {}
for extension in ('c', 'h'):
    counts[extension] = 0
    for path in (root/'firmware').rglob('*.'+extension):
        if 'build' in path.parts:
            continue
        text = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(), flags=re.S)
        counts[extension] += sum(bool(line.strip()) for line in text.splitlines())
assert sum(counts.values()) >= 500
board = (root/'firmware/board.h').read_text()
for mapping in ('SENSE0/PA0', 'SENSE1/PA1', 'SENSE2/PA4', 'SENSE3/PB0'):
    assert mapping in board
assert '#define CORE_HZ 16000000u' in board
assert '#define SERIAL_BAUD 115200u' in board
print(f'PASS {len(paths)} attributed files; README {words} words; production C/header lines {counts}')
print('Counts exclude comments, blanks, tests and generated build products.')
