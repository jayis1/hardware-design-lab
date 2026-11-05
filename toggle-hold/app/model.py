# Author: jayis1. SPDX-License-Identifier: MIT
"""TH1 conservative endpoint and session validation. No hardware authenticity claim."""
import csv
import os
import time
from collections import deque

FIELDS = ('sequence now state fault raw qualified valid origin age baseline hold '
          'reached_ms released_ms finish_ms result tx_errors gaps').split()
MASK = (1 << 32) - 1
STATES = ('WAIT', 'READY', 'PUSH', 'LOADED', 'HOLD', 'DONE', 'HALT')
RESULTS = ('none', 'retained', 'early release', 'rebound / recontact', 'conflict', 'timeout', 'aborted')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def validate(r):
    require(set(r) == set(FIELDS), 'field set')
    require(all(type(v) is int and 0 <= v <= MASK for v in r.values()), 'unsigned range')
    require(r['state'] <= 6 and r['result'] <= 6 and r['fault'] <= 31, 'enum')
    require(all(r[k] <= 15 for k in ('raw', 'qualified', 'valid')), 'contact mask')
    require(r['origin'] <= 2 and r['baseline'] <= 50 and r['hold'] <= 500, 'policy')
    require(r['age'] <= 10000, 'deadline')
    state = r['state']
    require((state == 6) == bool(r['fault']), 'halt/fault')
    if state == 0:
        require(not r['origin'] and not r['age'], 'waiting origin')
    if 1 <= state <= 5:
        require(r['origin'] in (1, 2), 'origin required')
    if state <= 1:
        require(not r['result'] and not r['age'], 'unstarted trial')
    if 2 <= state <= 4:
        require(not r['result'] and r['age'] < 10000, 'active result')
    if r['result']:
        require(state >= 5 and r['finish_ms'] == r['age'], 'finished time')
    else:
        require(r['finish_ms'] == MASK and state != 5, 'absent finish')
    reached, released = r['reached_ms'], r['released_ms']
    if reached != MASK:
        require(reached <= r['age'], 'future reach')
    if released != MASK:
        require(reached != MASK and reached < released <= r['age'], 'release order')
        require(r['hold'] <= r['age'] - released, 'hold credit')
    else:
        require(not r['hold'], 'unreleased hold')
    if state == 3:
        require(reached != MASK, 'loaded without arrival')
    if state == 4:
        require(released != MASK and r['hold'] < 500, 'holding without release')
    if r['result'] == 1:
        require(r['hold'] == 500 and r['age'] < 10000, 'retained boundary')
    if r['result'] == 5:
        require(r['age'] == 10000, 'timeout boundary')
    if r['result'] == 6:
        require(state == 6, 'abort without halt')
    if state <= 2:
        require(reached == MASK and released == MASK, 'premature milestone')
    if state == 3:
        require(released == MASK, 'loaded release')
    if 1 <= state <= 5:
        require(r['valid'] == 15 and r['baseline'] == 50, 'missing baseline')
    if state == 4 or r['result'] == 1:
        require(released != MASK and r['hold'] == r['age'] - released
                and r['raw'] == (3 ^ r['origin']) and r['qualified'] == r['raw'], 'retention evidence')
    if r['result'] == 2:
        require(reached == MASK, 'early arrival')


class Session:
    def __init__(self, source):
        self.source = source
        self.rows = deque(maxlen=2000)
        self.connected = False
        self.blocked = False
        self.warning = ''
        self.unreported_samples = 0
        self.last_host = None
        self.base_host = None
        self.device_elapsed = 0
        self.progress = False

    def accept(self, row, host=None):
        if self.blocked:
            raise ValueError('session blocked; reconnect explicitly')
        host = time.monotonic() if host is None else host
        try:
            validate(row)
            if self.rows:
                old = self.rows[-1]
                ds = (row['sequence'] - old['sequence']) & MASK
                dt = (row['now'] - old['now']) & MASK
                if ds == 0 and dt == 0:
                    require(row == old, 'contradictory duplicate')
                    return
                # Sequence saturates only when the device halts from exhaustion.
                require(0 < dt < (1 << 31), 'reset or reordering')
                require(0 < ds < (1 << 31) or
                        (row['sequence'] == MASK and row['state'] == 6 and row['fault'] & 4),
                        'sequence rollback')
                require(ds <= dt, 'sample rate')
                if row['state'] < 5:
                    require(ds == dt, 'unreported acquisition gap')
                if old['state'] < 5 and dt < 5:
                    changed = (row['qualified'] ^ old['qualified']) & old['valid']
                    require(not (changed & (old['raw'] ^ row['qualified'])),
                            'qualification faster than five samples')
                for k in ('tx_errors', 'gaps'):
                    require(row[k] >= old[k], 'diagnostic rollback')
                require(row['fault'] | old['fault'] == row['fault'], 'fault disappeared')
                if old['state'] >= 2:
                    require(row['state'] >= old['state'], 'trial phase regression')
                    require(row['origin'] == old['origin'], 'origin changed')
                    require(old['age'] <= row['age'] <= old['age'] + ds, 'acquired trial age')
                if 2 <= old['state'] <= 4 and 2 <= row['state'] <= 4:
                    require(row['age'] == old['age'] + dt, 'continuing age')
                if old['state'] < 2:
                    require(row['age'] <= ds, 'invented trial time')
                for k in ('reached_ms', 'released_ms', 'finish_ms'):
                    if old[k] != MASK:
                        require(row[k] == old[k], 'milestone changed')
                    elif row[k] != MASK and 2 <= old['state'] <= 4:
                        # ABORT freezes the last acquired age on loss/exhaustion.
                        require(row[k] > old['age'] or (k == 'finish_ms' and row['result'] == 6),
                                'backdated milestone')
                require(old['hold'] <= row['hold'] <= old['hold'] + ds, 'hold progression')
                if old['state'] >= 5:
                    for k in ('raw', 'qualified', 'valid', 'origin', 'age', 'baseline', 'hold',
                              'reached_ms', 'released_ms', 'finish_ms', 'result'):
                        require(row[k] == old[k], 'terminal evidence changed')
                if self.source == 'live':
                    assert self.last_host is not None and self.base_host is not None
                    require(host - self.last_host <= 2, 'host pause; export and reconnect')
                    require(abs((self.device_elapsed + dt) / 1000 - (host - self.base_host)) <= 2,
                            'device/host progress mismatch')
                self.device_elapsed += dt
                self.unreported_samples += max(0, ds - 1)
                self.progress = True
            else:
                self.base_host = host
            self.last_host = host
            self.rows.append(dict(row))
        except ValueError as exc:
            self.blocked = True
            self.warning = str(exc)
            raise

    def live(self, host=None):
        host = time.monotonic() if host is None else host
        return (self.source == 'live' and self.connected and not self.blocked and self.progress
                and self.last_host is not None and host - self.last_host <= 2
                and bool(self.rows) and self.rows[-1]['state'] < 5)

    def export(self, path, errors=0):
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w', newline='') as handle:
            writer = csv.writer(handle)
            writer.writerow(['author', 'kind', 'source', 'connected', 'blocked', 'live',
                             'parser_errors', 'unreported_samples', 'warning', 'age_seconds'] + FIELDS)
            metadata = ['jayis1', 'metadata', self.source, int(self.connected), int(self.blocked),
                        int(self.live()), errors, self.unreported_samples, self.warning,
                        '' if self.last_host is None else max(0, time.monotonic() - self.last_host)]
            writer.writerow(metadata)
            for row in self.rows:
                writer.writerow([metadata[0], 'snapshot'] + metadata[2:] + [row[k] for k in FIELDS])
