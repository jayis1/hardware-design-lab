# Flow Pause verification record

Author: jayis1

## Executed software checks

- `make -C flow-pause/firmware clean all test` passed with GNU Arm Embedded GCC 14.2.1. A real Cortex-M4 ELF linked without a vendor SDK; size output was 2668 bytes text, 0 initialized data, and 788 bytes BSS. ELF and binary products remain ignored, not committed.
- Host engine tests passed with AddressSanitizer and UndefinedBehaviorSanitizer. Coverage includes startup with enable held, independent qualification under unrelated chatter, complete and rejected pulses, exact silence and pulse-timeout boundaries, gap abandonment, timestamp wrap, prior versus simultaneous intervention attribution, transactional counter exhaustion and conservation across 100000 randomized input-mask sequences.
- Tests of the production clock driver passed bounded failure and input-configuration cases against simulated MMIO storage. Production serial-driver tests passed bounds, busy rejection, nonblocking drain, timestamp wrap and timeout recovery. These do not establish physical register timing.
- `xvfb-run -a python3 flow-pause/tests/test_app.py` passed all four test methods under Python 3.13.5. They cover real-C-encoder decoding, bytewise framing and resynchronization, EOF truncation, incompatible protocol, impossible counters, duplicate freshness, rollback, host reception pauses and gradual backlog; bounded replay, FIFO rejection, private CSV and overwrite refusal; and the actual Tk event loop driven through a Linux PTY, all three tabs, two history rows, duplicate-connect rejection, hangup detection and terminal restoration.
- `python3 flow-pause/tools/check_quality.py` passed: README 3042 words and 660 nonempty, noncomment production C/header lines, excluding tests. Authorship, source parsing, metadata JSON, file sizes, protocol field count and carrier signal assignments passed.
- `git diff --check` passed before staging. Build products, Python caches and KiCad preferences are ignored.

## Executed CAD checks

KiCad CLI 9.0.2 and the installed pcbnew bindings loaded the native design. `tools/check_hardware.py` ran schematic ERC with zero violations, PCB DRC with zero violations, and found zero unconnected items. A freshly exported XML schematic netlist matched the actual PCB and manifest for all 14 components, 30 pins and 10 nets, including values and full footprint library identities.

The schematic was exported to SVG, rasterized and visually inspected. Channel labels, connector pins, resistor paths and the jayis1 title block were legible and separated. The schematic is explicitly a passive carrier for an external Nucleo, so passive-only ERC does not establish validity of the external power supply or processor assembly. The board generator reuses the already routed topology with purpose-specific signal names.

## Review and interpretation

This run used standalone-owner implementation with advisory review, not full collective approval. A separate read-only software review reported no concrete blocking defects in the requested firmware and companion files. That advisory result does not replace the executed checks above.

The first application test run exposed an incorrect freshness expectation in the test: a sample received at 0.1 seconds is still within the one-second window at host time 1.0 seconds. The assertion was moved beyond the documented window; production freshness was not weakened. The complete suite then passed. A closed-serial-descriptor guard was also added before publication.

## Not performed / remaining engineering work

No carrier was fabricated, no MCU was programmed, no actual sensor was attached, and no bead-flow experiment or usability study was performed. No electrical noise, EMI, ESD, switch wetting, pulse capture accuracy, oscillator accuracy, power consumption, reed/magnet geometry, bucket mass threshold or mechanical durability was measured. The rocking receiver is a prototype proposal and needs independent video/mass validation before it can support an experiment.

The ST UM1724 PDF could not be freshly retrieved: HTTPS returned an HTTP/2 error and the HTTP/1.1 retry timed out. The documented board mapping is inherited from the repository's NUCLEO-F401RE platform; confirm it against the actual board revision before wiring. Manufacturer-document verification is not claimed.

Prior-art searching found related granular-flow and unjamming research. No global novelty or patent-clearance claim is made. Counts describe electrical proxies and operator declarations, not material flow certification or causation. The five-second silence criterion includes collection delay. Broken wires, empty supply, mechanical sticking and real blockage can appear similar. Reboots without an unambiguous counter rollback can remain undetected. There is no safety interlock, industrial rating or persistent on-device storage.
