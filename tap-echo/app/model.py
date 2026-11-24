# Author: jayis1. SPDX-License-Identifier: MIT
"""Strict TE1 data contract and bounded, explicit local session history."""
import json
import os
FIELDS = list[str](('now samples missed tx_errors fault raw qualified valid armed active recording '
          'references trials matches aborted timeouts reference_count count result max_error wrong age').split())
for prefix in ('reference', 'event'):
    for i in range(16):
        FIELDS.extend((f'{prefix}_pad{i}', f'{prefix}_interval{i}'))
RESULTS = ('capturing / no result', 'reference saved', 'match', 'mismatch', 'aborted')

def validate(row):
    if set(row) != set(FIELDS):
        raise ValueError('schema mismatch')
    if any(type(v) is not int or not 0 <= v <= 0xffffffff for v in row.values()):
        raise ValueError('integer range')
    for key in ('valid', 'armed', 'active', 'recording'):
        if row[key] > 1: raise ValueError('boolean range')
    if row['qualified'] > 15 or (row['raw'] > 15 and not row['fault'] & 16):
        raise ValueError('input range')
    if row['fault'] & ~31 or row['result'] >= len(RESULTS):
        raise ValueError('status range')
    if row['active'] and (row['armed'] or row['fault']):
        raise ValueError('inconsistent active state')
    if row['matches'] > row['trials'] or row['wrong'] > 16:
        raise ValueError('counter relationship')
    for prefix, key in (('reference', 'reference_count'), ('event', 'count')):
        count = row[key]
        if count > 16: raise ValueError('event capacity')
        for i in range(16):
            pad, interval = row[f'{prefix}_pad{i}'], row[f'{prefix}_interval{i}']
            if i < count:
                if pad not in (1, 2): raise ValueError('pad identity')
            elif pad or interval:
                raise ValueError('nonzero unused event')
            if interval > 30000 or (i == 0 and interval):
                raise ValueError('interval range')
    if row['reference_count'] == 1:
        raise ValueError('short reference')

class Session:
    def __init__(self, source):
        self.source = source
        self.rows = []
        self.epochs = 1
        self.dropped = 0
        self.previous = None
    def accept(self, row):
        validate(row)
        if self.previous is not None and row['samples'] < self.previous['samples']:
            self.epochs += 1
        self.previous = row.copy()
        self.rows.append(row.copy())
        if len(self.rows) > 1000:
            self.rows.pop(0)
            self.dropped += 1
    def export(self, path, metadata=None):
        payload = dict(author='jayis1', protocol='TE1', source=self.source,
                       epochs=self.epochs, dropped=self.dropped, rows=self.rows,
                       metadata=metadata or {'freshness': 'historical / unspecified'})
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w') as handle:
            json.dump(payload, handle, indent=2)
            handle.write('\n')
