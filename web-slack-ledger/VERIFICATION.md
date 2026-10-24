# Verification record

Author: jayis1

## Executed checks

- `make -C firmware clean all test`: passed. ARM GNU 14.2.1 compiled and linked the actual STM32F401RE target with `-Wall -Wextra -Werror`, freestanding startup, linker script, and no SDK download. Result: 2,836 bytes text, zero initialized data, 660 bytes BSS; stack budget separately reserved by linker assertion.
- Native GCC 14.2.0 ran engine and encoder assertions with AddressSanitizer and UndefinedBehaviorSanitizer. Cases cover forward/reverse decoding, invalid two-bit transitions, sample gaps, millisecond wrap, signed saturation, widened subtraction, CRC known vector, output bounds, 40,000 matched edges, and simultaneous matched reversal. Passed.
- `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v`: six tests passed under Python 3.13.5. Five protocol/model tests include a frame produced by the real C encoder. One GUI integration test exercises the actual Tk application and serial read loop through a Linux PTY, all three tabs, calibration validation, replay provenance, reconnect clearing, staleness, unplug handling, missing device handling, private CSV permissions, and overwrite refusal.
- `/usr/bin/python3 tools/check_hardware.py`: passed on KiCad 9.0.2. ERC zero violations, DRC zero violations, zero unconnected items. Fresh schematic XML and PCB pads match the manifest: fourteen components, thirty connected pins, ten nets; values and full footprint identifiers also match.
- `kicad-cli sch export netlist --format kicadxml -o kicad/device.net kicad/device.kicad_sch`: passed. The committed netlist is a genuine exporter result, not a separately imagined connection list.
- Schematic SVG export succeeded and its rendered image was inspected for four input paths, pull-ups, series resistors, headers, labels, and attribution.
- README measured 3,236 whitespace-delimited words. Production firmware measured 539 nonblank lines after block-comment removal across `.c` and `.h` files; tests, linker script, Makefile, vendor files, and generated outputs are excluded.
- Independent read-only software review found no blocking correctness issue. Its stale-summary finding was reproduced with a failing GUI assertion, fixed, and included in the passing suite. A simultaneous-wheel reversal-origin defect was likewise reproduced and fixed before final testing.

## Reproduce

From this directory:

```sh
make -C firmware clean all test
xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v
/usr/bin/python3 tools/check_hardware.py
```

Debian host packages used: GCC, ARM GNU embedded GCC/binutils, KiCad 9 with libraries and Python bindings, Python 3, python3-tk, xvfb, and xauth. No Python package download is required. The CAD generator can be run with `/usr/bin/python3 tools/generate_hardware.py`; it intentionally overwrites the generated CAD files, so preserve manual modifications first. Build products and local KiCad preferences are ignored.

## Interpretation limits

All telemetry in tests is synthetic. No physical encoder, MCU, USB link, PCB, material feed, or power supply was exercised. Target linking is not a flash/programming test. Passive-board ERC cannot validate external module power or cable wiring. A clean DRC does not certify electrical safety or fabrication quality. Mechanical geometry, wheel loading, traction, encoder input levels, timing margins, EMC and accuracy still require bench validation.

Manufacturer PDF fetches failed in this environment; product search results supported the selected encoder variant, but assembly requires verification against the current authoritative datasheet. No compliance, production readiness, worldwide novelty, or measured performance claim is made.
