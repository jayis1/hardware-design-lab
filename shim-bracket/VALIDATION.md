# Shim Bracket verification record

Author: jayis1

This run used standalone-owner execution with advisory review, not full collective approval. The verified destination is jayis1/hardware-design-lab, configured default branch main. Initial git pull --ff-only reported already up to date at b58793b. Device directories and README titles were inventoried; closest contracts inspected included ContactAtlas, Pattern Sled and Duplex Witness. No daily-ideas files were touched.

## Executed checks

- `make -C firmware clean all test`: ARM Cortex-M4 ELF and binary linked successfully; text 2460 bytes, data 0, BSS 788. These sizes exclude stack use and do not demonstrate hardware operation.
- Sanitized host engine tests: all 512 three-declaration words checked against an independent mask/monotonicity oracle; simultaneous paddles, raw selector and opposite-button glitches, gap abort, held-input restart, exact timeout, saturation, wrap, CRC and buffer bounds passed.
- Production clock and UART host tests passed, including bounded waits, input configuration checks, busy rejection, nonblocking drain and timeout recovery.
- Seven Python unittest cases passed under Xvfb, including all 256 fit/refusal mask combinations, impossible snapshots, corruption/truncated replay, FIFO rejection, freshness, rollback, private export and no-overwrite behavior.
- Real Tk event loop and Linux PTY exercised production C-encoder synthetic compatible, contradictory, rejected and aborted declarations. All four pages, disconnect, replay, failed connection preservation and CSV export were exercised. No real USB device was connected.
- KiCad 9 ERC: zero violations. PCB DRC: zero violations and zero unconnected items. Fresh schematic XML netlist matched 14 components, 30 pins and 10 nets against PCB pads and the manifest; values and full footprint identities matched.
- Schematic exported to SVG, rasterized and visually inspected for clear labels, channel routing and jayis1 title attribution.
- Quality checker measured 3048 README words and 592 nonblank, noncomment C/header lines, excluding tests. Every publication file carries jayis1 attribution. Shared support code reuse is explicit.

## Review corrections

Advisory read-only review found that snapshot validators allowed an active declaration at its rejecting deadline or an untainted simultaneous-button result. It also found that sparse companion snapshots could credit a different identity, or accept an already tainted declaration, when an observed active press completed without another start. Regression tests failed before corrections; both encoder and companion bounds were tightened and the complete test suite rerun successfully. Sparse snapshots remain necessary consistency checks, not a proof of every hidden event.

## Release limits

This is a source-verified engineering prototype. Fabrication, target programming, physical power measurements, switch timing, gauge calibration, mechanical alignment, EMC/ESD, reliability and actual workshop benefit are unverified. Thickness labels are nominal; button declarations are not measured clearance. No claim of universal novelty, production readiness or safety certification is made. The mechanical assembly is a documented proposal rather than a manufactured fixture.
