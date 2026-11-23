# Author: jayis1. SPDX-License-Identifier: MIT
"""PY1 evidence checks; sparse frames cannot prove an unobserved history."""
import time
FIELDS = ['now', 'samples', 'missed', 'unknown', 'tx_errors', 'fault',
          'raw', 'qualified', 'valid', 'run']
FIELDS += [f'{name}{i}' for name in ('bins', 'segments', 'peaks') for i in range(16)]
CUMULATIVE = ['samples', 'missed', 'unknown', 'tx_errors'] + FIELDS[10:]
EPISODES = ['starts', 'same', 'changed', 'orphan', 'aborted', 'active', 'known',
            'anchor', 'age', 'last_age', 'last_from', 'last_to', 'last_kind']
FIELDS += EPISODES
CUMULATIVE += EPISODES[:5]
RESULT = ['last_age', 'last_from', 'last_to', 'last_kind']


def parity_odd(mask):
    return mask.bit_count() % 2


def validate_episodes(r):
    if r['active'] > 1 or r['known'] > 1 or r['anchor'] > 15 or parity_odd(r['anchor']):
        raise ValueError('episode state range')
    if not r['known'] and r['anchor']:
        raise ValueError('unknown anchor is not zero')
    if (not r['active'] and r['age']) or (r['fault'] and r['active']) or r['age'] > r['samples']:
        raise ValueError('invalid episode age')
    if sum(r[k] for k in ('same','changed','orphan','aborted','active')) != r['starts']:
        raise ValueError('episode conservation')
    if r['starts'] > r['samples']//6:
        raise ValueError('episode without qualification')
    if not r['fault'] and r['valid']:
        if r['active'] != parity_odd(r['qualified']):
            raise ValueError('parity disagrees with active episode')
        if not r['active'] and (not r['known'] or r['anchor'] != r['qualified']):
            raise ValueError('even word missing anchor')
    recovered = sum(r[k] for k in ('same','changed','orphan'))
    if not recovered:
        if any(r[k] for k in RESULT): raise ValueError('result without recovery')
    else:
        kind = r['last_kind']
        if kind not in (1,2,3) or not 6 <= r['last_age'] <= r['samples']:
            raise ValueError('invalid recovery result')
        if any(r[k] > 15 or parity_odd(r[k]) for k in ('last_from','last_to')):
            raise ValueError('recovery endpoints must be even words')
        if not r[('same','changed','orphan')[kind-1]]:
            raise ValueError('result class not counted')
        if (kind == 1 and r['last_from'] != r['last_to'] or
            kind == 2 and r['last_from'] == r['last_to'] or
            kind == 3 and r['last_from'] != 0):
            raise ValueError('recovery classification contradicts endpoints')


def episode_transition(old, row, ds, dm):
    new_starts = row['starts'] - old['starts']
    recovered = sum(row[k]-old[k] for k in ('same','changed','orphan'))
    abandoned = row['aborted'] - old['aborted']
    if new_starts + recovered > (ds+5)//6:
        raise ValueError('episodes faster than word qualification')
    if abandoned > dm + int(bool(row['fault'] and not old['fault'])):
        raise ValueError('abandonment without gap or fault')
    if not recovered and any(row[k] != old[k] for k in RESULT):
        raise ValueError('retained result changed')
    if recovered:
        kind = row['last_kind']
        if row[('same','changed','orphan')[kind-1]] == old[('same','changed','orphan')[kind-1]]:
            raise ValueError('latest result class did not advance')
        if row['last_age'] > old['age'] + ds:
            raise ValueError('recovery duration exceeds acquired time')
    if not (new_starts or recovered or abandoned) and old['active']:
        if row['age'] != old['age'] + ds or any(row[k] != old[k] for k in ('anchor','known','active')):
            raise ValueError('continuing episode changed origin or age')
    if row['age'] > old['age'] + ds:
        raise ValueError('active age exceeds acquired time')
    if row['active'] and new_starts:
        # An already pending odd candidate can qualify next sample; otherwise
        # a fresh odd endpoint needs six observations before its age can grow.
        earliest = 1 if parity_odd(old['raw']) and not old['valid'] and not old['active'] else 6
        if row['age'] > max(0, ds-earliest):
            raise ValueError('episode age includes prequalification time')
    if recovered == 1 and not new_starts and old['active'] and not dm:
        kind = 3 if not old['known'] else 1 if old['anchor'] == row['last_to'] else 2
        if row['last_kind'] != kind or row['last_from'] != old['anchor']:
            raise ValueError('recovery changed captured origin')
    # Exact visible endpoint evolution; sparse checks above are only bounds.
    if ds == 1 and dm == 0 and not row['fault']:
        expected = {k: old[k] for k in EPISODES}
        if expected['active']: expected['age'] += 1
        if row['valid']:
            word = row['qualified']
            if parity_odd(word):
                if not expected['active']:
                    expected['starts'] += 1; expected['active'] = 1; expected['age'] = 0
            else:
                if expected['active']:
                    kind = 3 if not expected['known'] else 1 if expected['anchor'] == word else 2
                    expected[('same','changed','orphan')[kind-1]] += 1
                    expected.update(last_kind=kind, last_age=expected['age'],
                                    last_from=expected['anchor'], last_to=word, active=0, age=0)
                expected.update(known=1, anchor=word)
        if any(row[k] != expected[k] for k in EPISODES):
            raise ValueError('single-sample episode attribution')


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
    validate_episodes(row)


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
            if row['valid'] and row['raw'] != old['raw'] and ds < 6:
                raise ValueError('new word qualified before six observations')
            if (not row['valid'] and row['qualified'] != old['qualified'] and
                ds < (2 if old['raw'] == row['qualified'] else 7)):
                raise ValueError('hidden qualification lacks observation time')
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
                reserve = 6 if old['raw'] != i else 0 if old['valid'] else 1
                if db > max(0, ds-reserve):
                    raise ValueError('histogram credited qualification time')
                if dp and row[f'peaks{i}'] > ds + (old['run'] if old['raw'] == i else 0):
                    raise ValueError('peak grew beyond available run')
            if ds == 0:
                expected = old.copy()
                if row['fault'] and not old['fault']:
                    expected['aborted'] += expected['active']
                    expected['active'] = 0; expected['age'] = 0
                if any(row[k] != expected[k] for k in FIELDS if k not in ('now','tx_errors','fault')):
                    raise ValueError('evidence without acquisition')
            if ds == 1 and dm and old['samples']:
                # With only one new acquisition, all missed ticks belong to its
                # gap. No recovery, word dwell or retained anchor is possible.
                expected = old.copy()
                expected.update(samples=old['samples']+1, missed=old['missed']+dm,
                                unknown=old['unknown']+1, valid=0, run=0,
                                active=0, age=0, known=0, anchor=0,
                                aborted=old['aborted']+old['active'])
                if any(row[k] != expected[k] for k in FIELDS
                       if k not in ('now','tx_errors','fault','raw')):
                    raise ValueError('single-gap attribution')
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
            episode_transition(old, row, ds, dm)
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
