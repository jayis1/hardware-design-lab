# Verification record

Author: jayis1

## Executed checks

Run `python3 tools/verify.py` from this folder to reproduce the aggregate gate. Build products, Python caches, and KiCad local preferences are excluded from publication.

- Actual target: `make -C firmware check`, arm-none-eabi-gcc 14.2.1, Cortex-M4/thumb, freestanding link with warnings as errors. Image: 1968 bytes text, zero initialized data, 536 bytes static BSS; engine and frame buffers additionally occupy stack. Binary produced successfully. No target flashing was performed.
- Native C: GCC 14.2.0 with AddressSanitizer and UndefinedBehaviorSanitizer. Assertions passed for release compensation, simultaneous arrivals, ordinary timestamp wrap, same-vector ambiguous arrival, return-to-start rejection, skew and total-span boundaries, timeout, missed sampling, baseline recovery, chatter independence, UART loss, saturation, CRC reference vector, and bounded encoding.
- Python: Python 3.13.5, nine unit tests passed. Covered actual C encoder fixtures, arbitrary byte segmentation, corrupt records, marker resynchronization, duplicate freshness, suspected reset blocking, ABBA arithmetic, forbidden replay devices, stale-live collection, and empty replay rejection.
- GUI: `xvfb-run -a python3 tests/gui_smoke.py` passed using a real Tk event loop and Linux pseudo-terminal. Exercised all three tabs, serial open, pending-study boundary, queued pre-boundary result exclusion, new baseline, C-encoded trial rows, stale collection stop, PTY hangup, preserved live provenance, mode-0600 CSV, overwrite refusal, replay of a complete four-run fixture, contrast display, and FIFO rejection. Serial data was synthetic; this is not a physical USB or sensor test.
- Replay CLI: actual C fixture yielded four trial results, zero parser errors, no reset block, and contrasts `(0.0, 0.0)`. These are generated test values, not measured cart performance.
- CAD: KiCad 9.0.2 schematic ERC reported zero violations; PCB DRC reported zero violations and zero unconnected items. Independent exported schematic netlist and loaded PCB agreed with the manifest: 14 components, 30 numbered pins, 10 nets. Reference values and full footprint library identities matched.
- Visual: schematic exported to SVG and rendered to PNG for inspection. Four labeled channels, J1 supply, J2 sense header, resistor chains, switch headers, and jayis1 title block are present and legible. Electrical connectivity uses labeled nets, not a continuous drawing between all components.
- Structure: README has 3206 whitespace-separated words; firmware C and headers have 579 nonblank lines after stripping comments, excluding tests. All publishable device files contain jayis1. Aggregate gate checks source file size, common credential patterns, attribution fields, and whitespace errors. Python sources also passed compileall.
- Electrical arithmetic checked in Python: ideal closed-contact current 0.30 mA, sense-low voltage 0.30 V, four-contact total 1.20 mA and 3.96 mW. These are circuit calculations, not bench measurements.

## Review disposition and evidence limits

A read-only independent code review identified stale-study continuation, queued pre-Begin results, acceptance of empty replay input, and reset-identity ambiguity. The first three were addressed with regressions and GUI/PTY checks. The final issue remains an explicit design limitation: no persistent boot identifier is implemented. Sequence/time/counter regression checks are heuristic, and a missed reset can evade them if subsequent state is compatible. The instrument is not an audit-grade uninterrupted recorder. Restart the study after every known reset or power event.

The plain dual-lane timer idea was rejected because comparable products and published projects exist. The delivered design's distinction is its guided ABBA experiment and qualified evidence workflow. No worldwide novelty or patent claim is made. The existing repository purpose inventory was reviewed; the root index gained only the new device entry. Shared board support and carrier routing were reused from existing jayis1 designs rather than copied from vendor implementation source.

The ST manual download endpoint failed during this run. The F401RE connector map and ST-LINK virtual COM bridge configuration were checked against an already available local copy of UM1724 revision 13, Table 16 and section 6.8. Register definitions are inherited from the repository's STM32F401 support and cite RM0368. A successful target link does not validate analog levels, oscillator tolerance, or external wiring.

Not performed: fabrication, physical circuit inspection, programming a target, oscilloscope timing, magnet/reed geometry calibration, sensor disconnect diagnosis, cable-noise tests, ESD testing, environmental or regulatory qualification, or supervised user trials. Reed pulse widths and asymmetric placement can invalidate practical measurement accuracy despite all software tests passing. Carrier ERC does not verify the external controller supply, and software cannot establish cart identity or correct lane swaps.
