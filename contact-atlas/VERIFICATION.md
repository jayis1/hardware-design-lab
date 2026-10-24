# ContactAtlas verification record

Author: jayis1

## Executed checks

- Arm GCC 14.2.1: `make -C contact-atlas/firmware clean all test` passed with `-Wall -Wextra -Werror`. Actual Cortex-M4 ELF linked, binary generated. ELF sections: text 2340 bytes, data 0, BSS 6020.
- Host GCC address/undefined-behavior sanitizer test passed: detent qualification, zero and multiple-contact intervals, directed transitions, self-return excursions, sampling gaps, ordinary timer wrap, saturated addition, queue overflow, invalid masks, CRC check vector and bounded serialization.
- Node.js v22.22.3: seven tests passed. Tests cover strict frame schema and CRC, fragmented/oversized input, interrupted-line recovery, status freshness, sequence gaps, conservative discontinuity handling, history bounds and ordinary uptime/sequence rollover.
- Real UI handlers exercised against explicit DOM/serial mocks: three screens, unavailable Web Serial, real C-generated synthetic replay, CSV Blob contents, deferred file cancellation, canceled port open, live serial reader, stale status and disconnect. No third-party dependency installation required. `npm --prefix contact-atlas/app run check` passed both JavaScript syntax checks.
- Real browser served from localhost: Live and Connection screens opened; the file chooser replayed the compiled C fixture. Transition map displayed `0001 -> 0010`, 210 ms window, 10 ms zero time; and `0010 -> 0001`, 207 ms window, 7 ms multi time. Source remained SYNTHETIC REPLAY and never live. Native CSV download was not tested; generated CSV content was tested separately.
- KiCad 9.0.2 ERC: zero violations. DRC: zero violations and zero unconnected items. Fresh schematic XML and PCB pad comparison passed for 14 components, 30 pins and 10 nets, including values and footprint identities.
- Schematic exported to SVG and visually inspected: input headers, four resistor paths, output headers, net labels and jayis1 title block present and legible.
- Mechanical thresholds: README 3115 whitespace-separated words; firmware C/header sources 595 total lines and 550 nonblank lines after block/line comments are removed. Tests and build products excluded from firmware count.
- Original deliverables scanned for jayis1 credit and common credential markers; no missing credit or matching secret marker found. Build products and KiCad user preferences are ignored. `git diff --check` passed.

## Review resolution

Read-only independent review identified ambiguous restart inference, stale status erasing evidence, an unclear cancellation state, and loss of the frame following a partial serial write. The owner changed the parser to resynchronize on a complete version marker, changed session handling to preserve evidence and freeze on regressions, and exposed a cancelling state while unabortable browser operations settle. Regression tests failed before these changes and passed afterward. No other agent wrote repository files.

## Limits and unperformed work

No PCB has been fabricated or assembled for this revision. No target has been flashed, no physical USB connection exercised, and no selector measured. Electrical limits, timing accuracy, contact wetting, ESD/EMC behavior, mechanical fit, safety and regulatory compliance are unverified. Carrier ERC cannot validate the purchased Nucleo or wiring. Manufacturer PDF downloads failed; references and inherited platform assumptions are documented, not represented as newly audited datasheets.

No persistent boot identifier exists. Some resets remain indistinguishable from forward telemetry, so known power loss requires a fresh session. Millisecond interval statistics include final qualification, not calibrated sub-millisecond switching time. Electrical signatures can merge different mechanical detents. The browser map covers retained received events only, and explicit loss counters must be considered.

The external prior-art search was limited and found related switch testing concepts; global novelty and patentability are not established. This is an original application design and reproducible supervised prototype, not a certified finished product.
