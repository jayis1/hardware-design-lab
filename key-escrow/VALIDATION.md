# Key Escrow validation record

Author: jayis1

## Scope and status

Revision A is a source-complete, software-tested and CAD-checked engineering prototype. It has not been manufactured, flashed onto a physical target, calibrated or validated as a mechanical measurement system. It has no protective function. This run used standalone-owner implementation and verification; the supplied peer reports were advisory, not a completed collective review.

Repository routing was verified as jayis1/hardware-design-lab, default branch main. The starting checkout was clean, and git pull --ff-only reported already up to date at d4c8c7097b69a50aba4d062a33127ff7aeb32da3. No daily-ideas repository or systems tree was changed. The directory/title inventory and the purpose contracts of Catch Witness, Clamp Relay and Vacancy Walk informed concept differentiation. Global novelty is not established.

## Reproduction

From the repository root:

    python key-escrow/tools/verify.py

The verifier performs a clean target build, sanitizer-backed C tests, synthetic C fixture generation, Python/Tk tests under Xvfb, KiCad ERC/DRC and schematic/PCB/manifest comparison. It also checks required artifact presence, author credit and substantive source/documentation counts. Build products and local KiCad preferences are ignored rather than committed.

## Executed results

- Cortex-M4 target ELF and binary linked with arm-none-eabi-gcc, freestanding C11, -Wall -Wextra -Werror. Reported image: 2560 bytes text, zero initialized data, 788 bytes BSS. The engine is stack allocated; BSS is not total runtime memory usage. The linker reserves at least 4096 bytes above static storage for stack.
- Engine tests passed under AddressSanitizer and UndefinedBehaviorSanitizer: completed access, return without opening, early key removal, contradictory endpoints, unkeyed movement, invalid opening and extension, acquisition loss, timestamp wrap, sequence exhaustion and bounded serialization.
- A separate explicit truth table checks every outgoing stable mask at each of four admissible active positions, covering 64 position/mask combinations. All 65536 four-vector words from READY are also exercised for state/accounting/serialization invariants. The latter is an exhaustive bounded consistency test, not a complete temporal-logic proof.
- Boundary regressions passed for a qualifying key release at 59999 ms and at the rejecting 60000 ms deadline, closing tied to bolt extension, retraction tied to opening, extension tied to key removal, independent endpoint chatter during key removal, and raw key loss on the qualifying flap-opening sample.
- Clock/configuration host tests passed, including bounded failure paths and receive-only UART configuration. Production UART tests passed for busy rejection, buffer limits, nonblocking draining, timestamp wrap, timeout and recovery. These tests do not emulate analog silicon behavior.
- Seven Python tests passed, including CRC/decimal framing, one-byte streaming, truncation recovery, invalid semantic records, frozen HALT evidence, reset suspicion, duplicate freshness, timing drift rejection, private export/overwrite refusal, replay type/size rules and serial-attribute restoration.
- The GUI test ran the actual Tk event loop under Xvfb. It visited all three tabs, replayed the actual C encoder's synthetic output, checked cycle/return/abort counts and retained trace rows, exported CSV, connected a Linux pseudoterminal, received real C frames with an attachment fragment, rejected double-connect, cleared old summaries and detected hangup. No physical USB device was involved.
- KiCad schematic ERC: zero violations. PCB DRC: zero violations and zero unconnected items. Fresh XML netlist versus PCB and manifest: exact match for 14 components, 30 pins and 10 nets, including values and full footprint identifiers.
- The schematic was exported to SVG, rendered to PNG and visually inspected. Four distinct resistor/contact rows, connector labels, references and jayis1 title-block attribution were readable without observed collisions. The circuit is a passive carrier; ERC cannot establish correctness of the external supply or Nucleo wiring.
- README count: 3288 whitespace-separated words. Production firmware count excluding blank lines and comments: 558 C source lines plus 81 header lines. Tests are excluded from those counts.

## Provenance and limitations

The one-time import script records precisely which jayis1 platform files were reused from Vacancy Walk. Engine, wire payload, companion model/presentation, experiment tests and documentation were authored for Key Escrow. Reused STM32 drivers are actual production code, not placeholders added to meet a line target. The hardware generator emits local jayis1 symbols and metadata; KiCad's installed footprint geometry remains upstream material under its original licensing.

Public search established trapped-key systems as existing prior art, not novelty evidence. The configured extraction service was search-only, and direct ST manual downloads failed with HTTP/2 failure and a subsequent HTTP/1.1 timeout. The indexed distributor-hosted UM1724 copy supplied the NUCLEO-F401RE connector mapping. No claim is made that the full current ST manual was freshly downloaded during this run. Component stock, prices and current manufacturer approval for any particular installation were not verified.

Remaining physical work includes fabrication review, assembly inspection, supply/current measurements, contact wetting and magnet cross-talk tests, Nucleo flashing, oscillator/timing characterization, USB reliability, mechanical force/travel checks and independent video comparison. No EMC, ESD, functional-safety, security certification or environmental qualification was performed. There is no authenticated device identity, persistent boot ID, flash evidence log or key-code reader. Sparse telemetry validation supplies necessary consistency bounds only. Never attach this prototype to a real protective interlock or infer that counted cycles establish machine safety.
