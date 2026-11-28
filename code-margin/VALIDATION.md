# Code Margin verification record

Author: jayis1

## Scope and result

Revision A is a source-complete, build-tested engineering prototype, not a physically validated device. Verification was performed in standalone-owner mode. A separate read-only review found no blocking code findings; its missing-validation-report observation is addressed by this file. Publication is owned by the repository author. No physical measurements are represented by generated fixtures.

## Executed checks

- `make -C code-margin/firmware clean all test fixture`: passed. GNU Arm Embedded GCC 14.2.1 linked the STM32F401 target and produced the binary. ELF size: 2540 bytes text, zero initialized data, 1044 bytes BSS. Stack is additional to BSS.
- Four host C executables built with warnings as errors and AddressSanitizer/UndefinedBehaviorSanitizer: passed. Analysis checked all 65,536 retained codebooks, every four-bit reading and closest-pair witnesses against a bit-by-bit oracle. Acquisition checked all 256 ordered raw-word pairs plus qualification, gap, wrap, CRC and faults. Clock and production serial driver checks passed.
- `xvfb-run -a python3 -m unittest discover -s code-margin/tests -p 'test_app.py' -v`: all 11 tests passed under Python 3.13.5. The model's exhaustive oracle populates error spheres independently. Real C encoding generated 960 dense and 48 sparse synthetic frames; both ended in the same accepted state. Every ordered sparse fixture pair passed transition checks.
- The actual Tk event loop exercised replay, table rendering, ambiguous/outside-radius/unique-candidate inspection, private CSV export and overwrite refusal. PTYs exercised real serial transport, reconnect, failed-open evidence preservation, terminal restoration, stale acquisition, hangup and partial-tail accounting. These are software transport tests, not USB hardware tests.
- `/usr/bin/python3 code-margin/tools/generate_hardware.py`: produced the schematic, PCB, project, local symbol library and connectivity manifest using KiCad 9.0.2.
- `/usr/bin/python3 code-margin/tools/check_hardware.py`: native ERC reported zero violations; DRC reported zero violations and zero unconnected items. Fresh schematic XML and PCB pad mapping matched the manifest: 14 components, 30 pins, 10 nets, including reference values and footprint identities.
- Schematic exported to SVG with `kicad-cli sch export svg`, rendered using `rsvg-convert`, and visually inspected. Four pull-up/series/contact channels, separate supply/GPIO connectors, legible signal labels and author/title block were visible without obvious label collisions. The schematic is label-connected; the external processor is explicitly outside its boundary.
- Downloaded UM1724 Rev 16 from the university-hosted manufacturer-manual copy linked in README. Table 16 confirmed CN6.4 3V3, CN6.6 ground, CN8.1–4 PA0/PA1/PA4/PB0 and PA2 USART2 TX. The virtual-COM routing paragraph identifies SB13/SB14 on and SB62/SB63 off for the onboard bridge. Actual purchased-board configuration remains a commissioning check.
- README contains 3,044 whitespace-delimited words. Production firmware contains 553 nonblank, noncomment `.c` lines plus 92 `.h` lines, excluding tests and generated build files. Existing platform code is reused explicitly rather than represented as newly invented firmware.
- Every deliverable has format-compatible jayis1 attribution. Build outputs, Python caches and KiCad local preferences are ignored. No third-party package source is copied into the device; installed KiCad footprint geometry retains its upstream licensing.

## Failures encountered and resolved

The new C test initially failed to compile because the device-specific analysis API did not exist; the new Python test initially failed to import its analysis functions. These were intentional test-first checks. An initial GUI test incorrectly expected reading seven to have only code three as a candidate; code fifteen is also one bit away. The test was corrected to assert actual ambiguity and to exercise unique-candidate behavior on the earlier complementary-code fixture. Production code was not changed to satisfy that incorrect expectation.

The web extraction backend could not extract PDFs. Direct HTTPS download and `pdftotext` provided the manual check instead. The optional Python SVG renderer was absent; installed `rsvg-convert` rendered the schematic successfully. Neither limitation blocks the build or CAD verification.

## Requirements to evidence

| Requirement | Evidence |
|---|---|
| Distinct useful device | README comparison with Insert Code, Mask Key and Step Islands; bounded prior-art discussion |
| Detailed README | Word count above; purpose, hardware, diagram, firmware, application, use cases and limits |
| Substantial compile-ready C | ARM target ELF/binary plus four sanitized host suites; line counts above |
| Real electrical CAD | Generated native KiCad files, ERC/DRC and exact netlist/PCB comparison |
| Functional companion | Actual Tk screens, live POSIX transport, replay, inspector, CSV and tests |
| Consistent interpretation | Same CM1 schema; independent C/Python summary checks and real-encoder fixtures |
| Authorship | jayis1 attribution in original sources, documents, CAD metadata and exports |

## Not verified

Fabrication, target programming, assembled supply current, input levels, electrical timing, watchdog behavior on silicon, ESD, EMC, USB interoperability, physical code fixture construction, magnetic/mechanical calibration, measurement repeatability, enclosure fit and actual user value remain unverified. Native ERC on a passive carrier cannot establish the correctness of an external board or harness. Exhaustive mathematical tests establish behavior over retained bitsets, not completeness of a physical codebook. There is no object-identity authentication or guarantee that at most one contact can fail. Publication does not constitute a manufacturing release.
