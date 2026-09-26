#!/usr/bin/env python3
"""Structural checks for CasterEcho KiCad sources. Author: jayis1."""
# Copyright (c) 2026 jayis1; SPDX-License-Identifier: MIT
from pathlib import Path
import json
import re

ROOT = Path(__file__).parent

def balanced(text: str) -> bool:
    depth = 0
    quoted = False
    escaped = False
    for char in text:
        if escaped:
            escaped = False
        elif char == "\\" and quoted:
            escaped = True
        elif char == '"':
            quoted = not quoted
        elif not quoted and char == '(':
            depth += 1
        elif not quoted and char == ')':
            depth -= 1
            if depth < 0:
                return False
    return depth == 0 and not quoted

sch = (ROOT / "device.kicad_sch").read_text()
pcb = (ROOT / "device.kicad_pcb").read_text()
project = json.loads((ROOT / "device.kicad_pro").read_text())
assert balanced(sch) and balanced(pcb), "unbalanced KiCad S-expression"
assert project["meta"]["author"] == "jayis1"
for reference in ("U1", "U2", "U3", "U4", "U5", "J1", "J2", "D1", "C1"):
    assert f'"{reference}"' in sch and f'"{reference}"' in pcb, reference
nets = set(re.findall(r'\(net \d+ "([^"]*)"\)', pcb))
required = {"GND", "3V3", "I2C_SDA", "I2C_SCL", "HALL_A", "HALL_B", "PIEZO_ADC", "GEN_DC", "SWDIO", "SWCLK"}
assert required <= nets, required - nets
assert pcb.count('(segment ') >= 10
assert pcb.count('(footprint ') >= 9
print(f"KiCad structure PASS: {len(nets)} nets, {pcb.count('(footprint ')} footprints, {pcb.count('(segment ')} routed segments")
