#!/usr/bin/env python3
# HingeScribe KiCad structural validator. Author: jayis1. Copyright (c) 2026 jayis1.
from pathlib import Path
import json,re,sys
root=Path(__file__).resolve().parent
required=['device.kicad_pro','device.kicad_sch','device.kicad_pcb']
for name in required:
 p=root/name
 if not p.is_file(): raise SystemExit(f'missing {name}')
 text=p.read_text()
 if 'jayis1' not in text: raise SystemExit(f'author missing in {name}')
 if name.endswith(('.kicad_sch','.kicad_pcb')):
  depth=0; quoted=False; escaped=False
  for index,ch in enumerate(text):
   if escaped: escaped=False; continue
   if ch=='\\' and quoted: escaped=True; continue
   if ch=='"': quoted=not quoted
   elif not quoted and ch=='(': depth+=1
   elif not quoted and ch==')': depth-=1
   if depth<0: raise SystemExit(f'unbalanced {name} near line {text[:index].count(chr(10))+1}')
  if depth!=0 or quoted: raise SystemExit(f'unbalanced {name}: depth={depth}, quoted={quoted}')
json.loads((root/'device.kicad_pro').read_text())
sch=(root/'device.kicad_sch').read_text(); pcb=(root/'device.kicad_pcb').read_text()
refs=['U1','U2','U3','U4','U5','J1','J2','BT1','D1','F1']
for ref in refs:
 if f'"{ref}"' not in sch or f'"{ref}"' not in pcb: raise SystemExit(f'reference mismatch {ref}')
for net in ['3V3','GND','I2C_SDA','I2C_SCL','LOAD_DOUT','LOAD_SCK','VBAT','HARVEST_DC']:
 if net not in sch or net not in pcb: raise SystemExit(f'net mismatch {net}')
print(f'KiCad structural check passed: {len(refs)} matched references, 8 critical nets')
print('Note: run kicad-cli sch erc and pcb drc before fabrication.')
