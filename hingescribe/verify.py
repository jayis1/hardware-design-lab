#!/usr/bin/env python3
# HingeScribe repository quality gate. Author: jayis1. Copyright (c) 2026 jayis1.
from pathlib import Path
import re,sys
root=Path(__file__).resolve().parent
required=[root/'README.md',root/'firmware/main.c',root/'firmware/board.h',root/'firmware/registers.h',root/'firmware/Makefile',root/'kicad/device.kicad_sch',root/'kicad/device.kicad_pcb',root/'kicad/device.kicad_pro',root/'app/index.html',root/'app/app.js',root/'app/protocol.js',root/'app/package.json']
missing=[str(p.relative_to(root)) for p in required if not p.is_file()]
if missing: raise SystemExit('missing: '+', '.join(missing))
words=re.findall(r"[A-Za-z0-9][A-Za-z0-9'_-]*",(root/'README.md').read_text())
if len(words)<2000: raise SystemExit(f'README too short: {len(words)} words')
text_suffixes={'.md','.c','.h','.js','.json','.html','.css','.py','.pro','.sch','.pcb',''}
files=[p for p in root.rglob('*') if p.is_file() and p.suffix in text_suffixes and not p.name.endswith(('.o','.bin','.elf'))]
forbidden=('Nous'+' Research','Hermes'+' Agent','Anthro'+'pic','Clau'+'de')
for p in files:
 text=p.read_text(errors='strict')
 if 'jayis1' not in text: raise SystemExit(f'jayis1 attribution missing: {p.relative_to(root)}')
 for phrase in forbidden:
  if phrase in text: raise SystemExit(f'forbidden attribution {phrase}: {p.relative_to(root)}')
c_files=list((root/'firmware').glob('*.c'))
raw=sum(len(p.read_text().splitlines()) for p in c_files)
substantive=0
for p in c_files:
 in_block=False
 for line in p.read_text().splitlines():
  stripped=line.strip()
  if in_block:
   if '*/' in stripped: in_block=False
   continue
  if stripped.startswith('/*'):
   if '*/' not in stripped: in_block=True
   continue
  if stripped and not stripped.startswith('//'): substantive+=1
if raw<500 or substantive<500: raise SystemExit(f'firmware line gate failed: raw={raw}, substantive={substantive}')
print(f'quality gate passed: README={len(words)} words, C={raw} raw/{substantive} substantive lines, authored files={len(files)}')
