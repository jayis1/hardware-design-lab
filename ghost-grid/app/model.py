# Author: jayis1. SPDX-License-Identifier: MIT
"""GG1 evidence checks; sparse frames cannot prove an unobserved history."""
import time
FIELDS = ['now', 'samples', 'missed', 'unknown', 'tx_errors', 'fault',
          'raw', 'qualified', 'valid', 'run']
FIELDS += [f'{name}{i}' for name in ('bins', 'segments', 'peaks') for i in range(16)]
CUMULATIVE = ['samples', 'missed', 'unknown', 'tx_errors'] + FIELDS[10:]


def predicted(mask):
    return 15 if mask.bit_count() == 3 else mask


def validate(row):
    if set(row) != set(FIELDS):
        raise ValueError('schema mismatch')
    if any(type(v) is not int or not 0 <= v <= 0xffffffff for v in row.values()):
        raise ValueError('not uint32')
    if row['raw'] > 15 or row['qualified'] > 15 or row['valid'] > 1 or row['fault'] > 31:
        raise ValueError('state range')
    if row['valid'] and row['raw'] != row['qualified']:
        raise ValueError('qualified state mismatch')
    if row['valid'] and row['samples'] < 6:
        raise ValueError('qualification without enough observations')
    if row['run'] and (not row['valid'] or row['run'] > row[f"peaks{row['raw']}"]):
        raise ValueError('invalid active run')
    if row['samples'] != row['unknown'] + sum(row[f'bins{i}'] for i in range(16)):
        raise ValueError('observation conservation')
    if row['unknown'] < min(row['samples'], 6):
        raise ValueError('missing startup qualification')
    for i in range(16):
        total, count, peak = (row[f'{name}{i}'] for name in ('bins', 'segments', 'peaks'))
        if not total:
            if count or peak:
                raise ValueError('empty histogram metadata')
        elif not count or not peak or peak+count-1 > total or peak*count < total:
            raise ValueError('impossible segment bounds')


class Session:
    def __init__(self, source):
        self.source = source
        self.rows = []
        self.error = ''
        self.first_host = None
        self.last_host = None
        self.last_advance = None
        self.elapsed = 0
        self.advances = 0

    def reject(self, reason):
        self.error = reason
        raise ValueError(reason)

    def accept(self, row, host=None):
        if self.error:
            raise ValueError(self.error)
        try:
            validate(row)
            self._accept(row, time.monotonic() if host is None else host)
        except ValueError as exc:
            self.reject(str(exc))

    def _accept(self, row, host):
        if self.rows:
            old = self.rows[-1]
            dt = (row['now'] - old['now']) & 0xffffffff
            if dt == 0:
                if row != old:
                    raise ValueError('contradictory duplicate')
                return
            if dt >= 0x80000000:
                raise ValueError('device time rollback; reconnect')
            if any(row[k] < old[k] for k in CUMULATIVE):
                raise ValueError('counter rollback; reconnect')
            if old['fault'] & ~row['fault']:
                raise ValueError('fault disappeared')
            ds = row['samples'] - old['samples']
            dm = row['missed'] - old['missed']
            if ds + dm > dt:
                raise ValueError('acquisition faster than device time')
            if old['samples'] and not row['fault'] and ds + dm != dt:
                raise ValueError('unaccounted device time')
            if old['fault']:
                if any(row[k] != old[k] for k in FIELDS if k not in ('now', 'tx_errors', 'fault')):
                    raise ValueError('halted evidence changed')
            for i in range(16):
                db = row[f'bins{i}'] - old[f'bins{i}']
                dn = row[f'segments{i}'] - old[f'segments{i}']
                dp = row[f'peaks{i}'] - old[f'peaks{i}']
                if db > ds or dn > db or dp > db:
                    raise ValueError('histogram grew without observations')
                if dp and row[f'peaks{i}'] > ds + (old['run'] if old['raw'] == i else 0):
                    raise ValueError('peak grew beyond available run')
            if ds == 0 and any(row[k] != old[k] for k in FIELDS
                               if k not in ('now', 'tx_errors', 'fault')):
                raise ValueError('evidence without acquisition')
            if row['run'] > ds + (old['run'] if old['raw'] == row['raw'] else 0):
                raise ValueError('run grew too quickly')
            if ds == 1 and dm == 0:
                credited = (old['valid'] and row['valid'] and old['raw'] == row['raw'])
                run = old['run'] + 1 if credited else 0
                if row['run'] != run:
                    raise ValueError('single-interval run attribution')
                for i in range(16):
                    expected = int(bool(credited) and i == row['raw'])
                    if row[f'bins{i}'] - old[f'bins{i}'] != expected:
                        raise ValueError('single-interval attribution')
                    if row[f'segments{i}'] - old[f'segments{i}'] != int(expected and old['run'] == 0):
                        raise ValueError('single-interval segment attribution')
                    if row[f'peaks{i}'] != max(old[f'peaks{i}'], run if expected else 0):
                        raise ValueError('single-interval peak attribution')
            self.elapsed += dt
            if self.source == 'live':
                if host - self.last_host > 1:
                    raise ValueError('host pause: reconnect, queued evidence not live')
                if abs(self.elapsed/1000 - (host-self.first_host)) > 1:
                    raise ValueError('device/host progress diverged')
            if ds:
                self.advances += 1
                self.last_advance = host
        else:
            self.first_host = host
        self.last_host = host
        self.rows.append(row.copy())
        if len(self.rows) > 1000:
            del self.rows[:-1000]

    def fresh(self, host=None, connected=False):
        host = time.monotonic() if host is None else host
        return bool(connected and self.source == 'live' and not self.error
                    and self.rows and not self.rows[-1]['fault'] and self.advances >= 2
                    and self.first_host is not None
                    and self.last_advance is not None and host-self.last_advance < .5
                    and abs(self.elapsed/1000 - (host-self.first_host)) <= 1)
