#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import reviewed framing and POSIX serial code from maze-return, commit 5fd0821.
Keep the transport unchanged except marker and model imports. Refuse overwrite.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=(root.parent/'maze-return/app/protocol.py').read_text()
text='# Author: jayis1. SPDX-License-Identifier: MIT\n'
text+='"""Bounded SS1 framing, POSIX serial and strict replay; stdlib only."""\n'
text+='import binascii, os, re, select, stat, termios\nfrom model import FIELDS, validate, Session\n\n'
text+=source[source.index('class Decoder:'):source.index('class Session:')]
text+=source[source.index('class Serial:'):]
text=text.replace('MR1','SS1')
path=root/'app/transport.py'
path.parent.mkdir(exist_ok=True)
with path.open('x') as handle: handle.write(text)
