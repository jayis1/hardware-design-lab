#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""One-time import of jayis1 infrastructure from case-delta at a63a007.
No measurement engine or app interpretation is imported. Refuse overwrites.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=root.parent/'case-delta'
names=['firmware/main.c','firmware/board.h','firmware/registers.h',
       'firmware/startup.c','firmware/linker.ld','firmware/drivers/board.c',
       'firmware/drivers/clock.c','firmware/drivers/serial.c','firmware/Makefile',
       'firmware/wire.h','tools/generate_hardware.py','tools/check_hardware.py',
       'app/transport.py','tests/test_clock.c','tests/test_serial.c','LICENSE','.gitignore']
for name in names:
    target=root/name
    if target.exists(): raise SystemExit('Refusing overwrite '+str(target))
    text=(source/name).read_text()
    for old,new in [('case-delta','bin-phrase'),('Case Delta','Bin Phrase'),
                    ('CASE_BOARD','PHRASE_BOARD'),('CASE_WIRE','PHRASE_WIRE'),
                    ('CD1','BQ1'),('demo.cd1','demo.bq1'),
                    ('case inventory contact','bin opening contact'),
                    ('POCKET_A','OPEN_A'),('POCKET_B','OPEN_B'),
                    ('POCKET_C','OPEN_C'),('LID','TRAY')]:
        text=text.replace(old,new)
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(text)
print('Imported jayis1 infrastructure; engine and interpretation are new')
