# Rinse Ledger revision A verification

Author: jayis1

## Executed release gates

- `make -C rinse-ledger/firmware check`: actual Cortex-M4 ELF and binary linked with arm-none-eabi-gcc, freestanding startup and linker script. Text 2048 bytes, data 0 bytes, BSS 536 bytes. Host C assertions passed under AddressSanitizer and UndefinedBehaviorSanitizer.
- `xvfb-run -a python3 -m unittest discover -s rinse-ledger/tests -p 'test_*.py'`: seven tests passed. These include the production C encoder fixture, CRC/framing rejection, stream resynchronization, bounded pending data, duplicate/stale behavior, rollback blocking, impossible exposure rejection, private exclusive CSV creation, real Tk screens, live Linux PTY connection, hangup detection and offline replay.
- `/usr/bin/python3 rinse-ledger/tools/check_hardware.py`: schematic ERC zero violations; PCB DRC zero violations and zero unconnected items. Fresh exported netlist matches actual PCB references, values, footprints and pin nets: fourteen components, thirty pins, ten nets.
- `kicad-cli sch export svg`: successful export. Rendered with rsvg-convert and visually inspected: four input rows, power and MCU headers, resistor networks, labels and jayis1 title block are legible and separated. Shared net labels connect the intentionally separated component groups.
- `python3 rinse-ledger/tools/verify.py`: integrated checks pass, including attribution, source-file inventory, secret-pattern screening and minimum-size gates. README: 3073 words. Production C files: 551 lines total, 521 nonblank/noncomment lines, excluding headers and tests.

## Review corrections

A read-only source review identified two application acceptance gaps. Regression tests reproduced both failures before modification: a current-batch duration could regress while the batch identity remained unchanged, and exposure could be credited without any recorded batch or entry. The parser now rejects exposure without a declared bath and entry. The session model also blocks same-bath duration rollback and cumulative immersion increments greater than elapsed device uptime. All seven Python tests passed after these changes.

The C timing test was corrected to account for the documented qualification semantics: the first changed sample starts an age of zero, and eight subsequent milliseconds qualify the edge. This is nine samples at nominal one-millisecond sampling, not eight counted samples. No physical edge timestamps were used.

## Scope and limitations

No hardware has been fabricated, connected, flashed or immersed. No physical timing accuracy, contact durability, immunity, enclosure ingress rating or chemical outcome is established. The passive PCB requires a separate NUCLEO-F401RE and independently engineered dry fixture. The design does not measure water replacement or cleanliness; the button is an operator declaration.

All tests use synthetic switch states or pseudoterminals. Export checks verify actual locally written CSV contents and permissions. The application retains only 256 snapshots, does not recover historical batch boundaries after loss, and cannot always distinguish resets without a persistent boot identifier. Standard footprint assets retain upstream licensing; original work is credited to jayis1.

The web comparison retrieved commercial basket washers and DIY continuous-flow washers. It supports a bounded comparison, not a claim of worldwide originality. Manufacturer manuals are referenced for electrical bring-up; this run's web extraction backend could not extract their PDFs. Existing repository board-support mapping and the local CAD consistency checks were used; verify the exact physical board revision against those manuals before assembly.
