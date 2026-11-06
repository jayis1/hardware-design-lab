# Validation — Pallet Census revision A

Author: jayis1

This report describes actual file-level execution, not physical hardware validation. Reproduce with `/usr/bin/python3 tools/verify.py` from this directory. The run was performed in standalone-owner mode with advisory review, not full collective review.

## Executed checks

- GNU Arm Embedded GCC 14.2.1: clean freestanding Cortex-M4 compile and link with `-Wall -Wextra -Werror`; binary generation passed. The final ELF reports 2800 bytes text, zero initialized data and 788 bytes BSS. The linker reserves at least 4096 bytes for stack headroom. This is not a measurement of worst-case runtime stack use.
- Host GCC with address and undefined-behavior sanitizers: engine, clock and serial test executables passed. The engine enumerates all 4096 six-event words over the four stable pallet codes against an independent counter and previous-letter oracle.
- Engine regressions passed for held-RUN startup, exact baseline qualification, tied pallet/index transitions, raw pallet loss, raw RUN loss, independent-channel chatter, rejecting deadline equality, completion immediately before deadline, timestamp wrap, skipped acquisition, sequence exhaustion, CRC known answer, undersized buffers and invalid telemetry.
- Fourteen Python unittest cases passed under the actual Tk event loop in Xvfb where applicable. Coverage includes actual C-encoder replay, CRC damage and framing recovery, partial tails, invalid counters, duplicates, freshness, rollback, frozen run/halt evidence, pair source budgets, impossible one-sample completion, private CSV exports, FIFO/empty replay rejection, serial attribute restoration and setup-failure rollback.
- GUI integration used real Linux pseudo-terminals with synthetic bytes from the C encoder. Live receipt, all retained trace rows, counters, connection exclusivity, hangup, retained provenance, errors in export, overwrite refusal, replay and failed reconnection were exercised. No physical USB device was attached.
- KiCad CLI 9.0.2: schematic ERC returned zero violations. PCB DRC returned zero violations and zero unconnected items. No waiver or excluded violation was required.
- Fresh KiCad XML netlist compared against actual PCB pads and the authored connectivity manifest: 14 components, 30 pins, 10 nets. Reference, pin, net, value and full footprint identity matched.
- Schematic exported to SVG, rasterized with rsvg-convert and visually inspected: symbols, names, net labels and title block are legible without observed overlap. Render previews are scratch artifacts, not source deliverables.
- Acceptance script counted 3082 whitespace-delimited README words and 573 nonblank, noncomment production C lines, excluding tests and headers. Production headers add 80 nonblank, noncomment lines. All authored deliverables contain jayis1 attribution in valid comments or supported metadata.

## Review corrections

Read-only advisory review identified that pair validators bounded destinations but not sources. Matching C and Python regressions first demonstrated acceptance of a B-to-A pair with no B arrival. Both validators now bound outgoing pair counts plus the retained anchor against the source-side arrival count. The exhaustive real-engine oracle still passes.

A separate regression demonstrated that the companion accepted a newly completed zero-duration run. Both validators now require at least five acquired intervals for completed runs; the session additionally bounds a new run's age by the interval since the previous snapshot. Regression cases failed before correction and pass afterward.

Baseline tests also exposed early credit on the first qualified sample. The implementation now requires prior validity and raw/qualified agreement at both interval endpoints before idle-baseline credit. Exactly 54 initial low samples leave WAIT with 49 credited intervals; the next enters READY.

## Evidence boundaries

The fixture is synthetic and labeled as such. Automated checks do not establish physical engagement, exact tooth count, calibration, mechanical usefulness, elapsed-time accuracy, EMI immunity or electrical safety. Sparse snapshot validation uses necessary bounds, not complete reconstruction of hidden inputs. CRC does not authenticate a source. Reset inference lacks a persistent boot identifier. All those limitations are part of the interface contract, not undocumented exceptions.

The following were not performed: fabrication, solder assembly, physical target flashing, electrical bring-up, USB interoperability with a real board, acquisition-jitter measurement, contact wetting tests, magnetic cross-trigger measurements, mechanical fixture assembly, video correlation, enclosure fit testing, EMC/ESD qualification or regulatory review. These remain prerequisites for deployment, not failed file-level tests.

The reusable board support, transport and carrier-generation source were imported from the repository's reviewed jayis1 passive-carrier platform. The measurement engine, PC1 schema, companion validation, tests and application content implement this device's separate contract. `tools/import_platform.py` refuses overwrites and is a provenance/bootstrap utility, not part of routine builds.
