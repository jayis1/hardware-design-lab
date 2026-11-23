#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""One-time, overwrite-refusing import of jayis1's reviewed platform."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = '9416166046264ce230d3620dc43793ce0f21dcbd'
FILES = ['firmware/board.h', 'firmware/registers.h', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/startup.c',
         'firmware/linker.ld', 'firmware/main.c', 'firmware/Makefile',
         'firmware/engine.c', 'firmware/engine.h', 'firmware/wire.c', 'firmware/wire.h',
         'tests/test_clock.c', 'tests/test_serial.c', 'app/transport.py', 'app/model.py',
         'app/workbench.py', 'tests/test_app.py', 'tests/test_ui.py',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'LICENSE', '.gitignore']
for name in FILES:
    if (ROOT / name).exists():
        raise SystemExit('Refusing overwrite: ' + name)
for name in FILES:
    text = subprocess.check_output(['git', 'show', f'{REV}:ghost-grid/{name}'],
                                   cwd=ROOT.parent, text=True)
    for old, new in [('ghost-grid', 'parity-sled'), ('Ghost Grid', 'Parity Sled'),
                     ('GHOST_', 'PARITY_'), ('GG1', 'PY1'), ('demo.gg1', 'demo.py1'),
                     ('KEY_00', 'DATA0'), ('KEY_01', 'DATA1'),
                     ('KEY_10', 'DATA2'), ('KEY_11', 'PARITY')]:
        text = text.replace(old, new)
    path = ROOT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
print('Imported platform from', REV, '; episode logic and UI require device-specific edits')
