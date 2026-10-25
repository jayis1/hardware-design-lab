# Verification record

Author: jayis1

Scope: Swing Envelope revision A. These are executed software and CAD checks, not physical measurements. No target board was programmed and no pendulum was used.

## Executed checks

- ARM GNU GCC 14.2.1: `make -C firmware all test` compiled and linked for Cortex-M4 with `-Wall -Wextra -Werror`, a freestanding reset handler and STM32F401RE linker layout. Image section report: 1,892 bytes text, zero initialized data, 276 bytes BSS. Stack/foreground engine storage is additional to BSS. ELF and binary are ignored build outputs.
- Host GCC: engine and wire tests passed under AddressSanitizer and UndefinedBehaviorSanitizer. Cases cover directional pairs, completed bilateral threshold evidence, missed thresholds, ambiguous simultaneous gates, rearming, acquisition gaps, pending timeout, short glitches, repeated direction, timestamp wrap, saturating transport counter, CRC vector and short output buffer.
- Python 3.13.5: eight unittest methods passed under `xvfb-run`. The real Tk event loop received the real C encoder's synthetic sequence through a Linux PTY, rendered retained rows, switched all three tabs, wrote a private CSV, handled hangup and loaded labelled replay. Parser tests include partial frames, truncated-frame resynchronization, invalid versions, bad checksums, oversized input, suspected resets, retention, stale time and modular wrap. No physical USB endpoint was used.
- Independent read-only source review found no blocking measurement or framing issue. It identified terminal restoration missing from the failed-open cleanup path after a successful termios update. That cleanup was fixed and a fault-injected `tcflush` regression verifies restoration.
- KiCad CLI 9.0.2: schematic ERC zero violations; PCB DRC zero violations and zero unconnected items. A fresh schematic XML export, actual PCB pads and the manifest agree on 14 components, 30 pins and 10 nets, including component values and full footprint identities.
- Schematic SVG export was rendered and visually inspected. Header labels, four independent resistor networks and the author/title block are present without clipped components. No MCU or regulator is falsely depicted on the passive carrier.
- README measured 2,968 whitespace-delimited words. Firmware C/header sources measured 551 nonblank lines after removal of block and line comments; tests and generated artifacts are excluded.
- Authored text files were checked for jayis1 credit and for prohibited alternative author names, private-key markers and common credential prefixes. Build products, Python caches and KiCad local preferences are ignored.

## Reproduction

Run from this device directory:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v
/usr/bin/python3 tools/check_hardware.py
```

For CAD regeneration, use `/usr/bin/python3 tools/generate_hardware.py` with the distribution pcbnew bindings and KiCad standard footprint libraries, then rerun the checker. Generated native CAD files embed jayis1 credit. The original passive board support and layout were reused from jayis1's Slip Window design; the new measurement engine, protocol, app and experiment description implement Swing Envelope rather than renaming the previous state machine.

## Unverified engineering work

Fabrication, solderability, physical footprint fit, external connector wiring, Nucleo bridge settings on the purchased board, flashing, electrical input levels, contact wetting reliability, ESD immunity, timing drift, gate spatial hysteresis, magnet disturbance, fixture repeatability and real pendulum performance remain unverified. Zero CAD rule violations do not resolve those questions. There is no metrology certification, safety certification, patentability conclusion, supplier-stock guarantee or measured power budget. Follow the README commissioning sequence before interpreting physical experiments.
