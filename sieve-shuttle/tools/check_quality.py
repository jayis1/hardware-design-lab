#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Release gates for source sizes, authorship, protocol and file hygiene."""
from pathlib import Path
import re
import sys
root=Path(__file__).resolve().parents[1]
files=[p for p in root.rglob('*') if p.is_file() and not any(
    part in ('build','__pycache__') for part in p.relative_to(root).parts)
    and p.suffix not in ('.pyc','.kicad_prl')]
words=len((root/'README.md').read_text().split())
assert words>=2000,words
sources=[p for p in files if p.suffix in ('.c','.h') and 'firmware' in p.parts]
lines=sum(sum(bool(line.strip()) for line in re.sub(r'/\*.*?\*/|//[^\n]*','',p.read_text(),flags=re.S).splitlines()) for p in sources)
assert lines>=500,lines
for p in files:
    assert not p.is_symlink(),p
    text=p.read_text()
    assert 'jayis1' in text,p
    assert not re.search(r'(?im)^\s*(?:(?:#|//|/\*|\*|<!--)\s*)?(?:author|copyright)\b[^\n]*(?:Nous Research|Anthropic|Claude|OpenAI|Hermes)',text),p
    assert not re.search(r'-----BEGIN (?:RSA |OPENSSH |EC )?PRIVATE KEY-----',text),p
required=['firmware/main.c','firmware/board.h','firmware/registers.h','firmware/Makefile',
          'kicad/device.kicad_sch','kicad/device.kicad_pcb','kicad/device.kicad_pro','app/workbench.py']
for name in required: assert (root/name).is_file(),name
sys.path.insert(0,str(root/'app'))
from model import FIELDS
assert len(FIELDS)==37
assert '"SS1"' in (root/'firmware/wire.c').read_text()
assert "b'SS1,'" in (root/'app/transport.py').read_text()
print(f'PASS: {words} README words; {lines} substantive firmware C/header lines; {len(files)} attributed deliverables; 37 protocol fields')
print('Nominal contact current:',3.3/11000*1000,'mA; all-closed power:',4*3.3**2/11000*1000,'mW')
