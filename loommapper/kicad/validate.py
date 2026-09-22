#!/usr/bin/env python3
# LoomMapper KiCad structural validator. Author: jayis1. Copyright (c) 2026 jayis1.
from pathlib import Path
import json
root=Path(__file__).resolve().parent
for name in ('device.kicad_pro','device.kicad_sch','device.kicad_pcb'):
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
   if depth<0: raise SystemExit(f'unbalanced {name} near byte {index}')
  if depth or quoted: raise SystemExit(f'unbalanced {name}: depth={depth} quote={quoted}')
json.loads((root/'device.kicad_pro').read_text())
sch=(root/'device.kicad_sch').read_text();pcb=(root/'device.kicad_pcb').read_text()
refs=('U1','U2','U3','U4','U5','J1','J2','BT1','D1','F1')
nets=('3V3','GND','SPI_SCK','SPI_MOSI','SPI_MISO','ADC_DRDY','EXCITE_OUT','SENSE_IN','I2C_SDA','I2C_SCL','VBAT','CLIP1','CLIP2','CLIP3','CLIP4','CLIP5','CLIP6','CLIP7','CLIP8')
for ref in refs:
 if f'"{ref}"' not in sch or f'"{ref}"' not in pcb: raise SystemExit(f'reference mismatch: {ref}')
for net in nets:
 if net not in sch or net not in pcb: raise SystemExit(f'net mismatch: {net}')
if pcb.count('(segment ')<8: raise SystemExit('insufficient routed signal segments')
print(f'KiCad structural check passed: {len(refs)} matched references, {len(nets)} critical nets, {pcb.count("(segment ")} signal segments')
print('Engineering reference: run kicad-cli ERC/DRC after footprint and production-rule review.')
