# Validation record

Author: jayis1

Scope: standalone-owner design and implementation, with advisory review; not a full collective hardware review. This record concerns source artifacts and simulated inputs. It does not certify a physical assembly.

## Executed checks

- `make -C firmware all test`: ARM Cortex-M4 freestanding image compiled and linked with arm-none-eabi-gcc. Image sections: text 2836 bytes, data 0, BSS 788. ELF and raw binary generated under ignored build output.
- Host engine tests passed with AddressSanitizer and UndefinedBehaviorSanitizer. The input suite enumerates every word over four signed adjacent-crossing symbols through length six, comparing a separate stack/permutation oracle after each operation. Additional cases cover inverse pairs, unchanged endpoint permutation with a nonempty word, both invalid detents, raw selector changes, sequential paddle handoff, simultaneous paddles, exact timeout, gap abandonment, capacity, counter exhaustion, timestamp wrap and bounded serialization.
- Production clock-driver tests passed bounded failure paths and GPIO configuration checks. Production UART tests passed buffer bounds, busy rejection, nonblocking drain, timeout recovery and timestamp wrap.
- Twelve Python unittest methods passed under Xvfb, including real Tk event loops. Tests exercise semantic parsing, CRC-valid impossible records, immutable prefixes, halted evidence, active capture identity, impossible completion timing, exact rejecting deadlines and their permitted neighbor, duplicate timestamps, stale traffic, cumulative backlog, truncated streams, replay EOF, FIFO refusal, private export, overwrite refusal and POSIX serial restoration.
- GUI tests replayed the real C encoder's synthetic six-crossing trace. They checked six drawn crossings, A B C endpoint order, retained nontrivial-word warning, history rows and all four screens. A Linux PTY exercised actual live serial reception, duplicate-connect refusal, unplug detection, failed-open preservation and clearing stale visuals on replacement.
- KiCad 9 ERC: zero violations. PCB DRC: zero violations and zero unconnected items.
- Fresh schematic XML export matched the PCB and connectivity manifest exactly: 14 components, 30 pins and 10 nets, including values and full footprint library identities.
- Schematic exported as SVG, converted to PNG and visually inspected. The four left/right/over/under channels, power and input connectors are readable with separate net labels and author title block.
- README: 3118 whitespace-delimited words. Firmware: 616 nonblank C/header lines after removal of block and line comments, excluding tests. Every delivered source includes jayis1 attribution; generated build output and local KiCad preferences are ignored.

Reproduction: from the device directory run `python3 tools/verify.py`. That command does not regenerate hardware, flash, connect to a device, commit or publish. For a clean target rebuild first run `make -C firmware clean`. To regenerate hardware after reviewing the generator use `/usr/bin/python3 tools/generate_hardware.py`, then rerun verification.

## Review dispositions

A read-only implementation review identified missing sparse-completion timing checks in the companion. A regression first reproduced acceptance of a newly completed stroke one millisecond after READY. The model now requires the minimum start/release spacing, validates retained completion age, and rejects acceptance at the firmware's ten-second deadline. Tests also permit the immediately preceding valid boundary. The reviewer observed the documented verifier before it had been created; the supplied verifier now exists and has been exercised end to end.

## Limits and remaining physical gates

No physical board, programming operation, cord guide, switch actuation, power measurement, contact repeatability test, ESD test, EMC test, classroom study or regulatory evaluation was performed. The external Nucleo connector and register mapping reuse the existing repository platform; purchased board revision and solder bridges still require manufacturer-document comparison during bring-up. Prices are estimates and no availability promise is made.

The core input is a declared crossing, not detected cord topology. Immediate-inverse reduction is not a complete braid normal form; equal permutations or equal exponent sums do not establish equivalent braids. All useful-output claims depend on the operator maintaining the stated physical and viewing conventions. Hardware fabrication and supervised validation remain separate release gates.
