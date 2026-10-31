# Rail Skew validation record

Author: jayis1

## Executed checks

This is standalone-owner verification, not collective review or independent hardware certification. Advisory reports informed the gates; the implementation and final review were performed in this run.

- Verified repository origin as jayis1/hardware-design-lab and remote HEAD as main. The unrelated advisory reference to creative-projects was not used. The initial tracked working tree was clean. Ran git pull --ff-only before implementation; it reported already up to date. No daily-ideas files were touched.
- Inventoried device directories and purpose descriptions; read the closest Crossover Track, Nest Settle and Bellows Phase contracts. Rail Skew's paired maintained ordinal thresholds differ from independent-cart timing, corner seating and valve-state exposure. Worldwide novelty is not established.
- make -C firmware clean all test: ARM GNU compiler 14.2.1 linked the STM32F401 Cortex-M4 target with -Wall -Wextra -Werror. ELF text 2844 bytes, data 0 bytes, BSS 788 bytes; stack usage is additional. Binary generation passed. No physical flashing was performed.
- Native GCC AddressSanitizer and UndefinedBehaviorSanitizer tests passed for the engine, production clock configuration and production UART driver. The initial engine test failed before the engine existed; implementation then made it pass.
- Engine regression coverage includes both directions, matching and mismatched stage pairs, origin return, direct endpoint jump, raw glitch exclusion, independently qualifying rail bundles, invalid code, sampling gap, timeout/arrival precedence, ordinary timestamp wrap, counter exhaustion rollback, CRC check vector, output capacity, and invalid snapshot rejection. All legal stage pairs followed by every raw vector are checked for both origins, including serialization of each resulting state.
- xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v: eight tests passed. These exercise actual C-encoder fixtures, CRC-valid impossible records, truncation/resynchronization, sparse growth bounds, frozen results, stale/disconnected evidence, duplicate timestamps, gradual backlog, reset suspicion, bounded replay, FIFO rejection, private exclusive CSV export, POSIX serial restoration and hangup, and real Tk screens through a Linux pseudo-terminal. Initial application tests failed on absent modules before implementation.
- The Tk test connected to a real PTY, delivered seven synthetic C-encoded frames, switched all three tabs, checked all retained rows, exported CSV, exercised hangup, loaded replay, rejected invalid connection paths, and verified that a successful reconnect clears previous numeric evidence. No physical USB device or cam signal is implied by a PTY test.
- KiCad 9.0.2 schematic ERC: zero violations. PCB DRC: zero violations, zero unconnected items. Fresh schematic XML export compared exactly against actual PCB pads and the manifest: 14 components, 30 pins, 10 nets; reference, pin, net, value and full footprint identity matched. A native KiCad netlist is also delivered as kicad/device.net.
- Exported the schematic as SVG, rendered it through rsvg-convert and visually inspected labels, component separation and title attribution. The optional Python CairoSVG renderer was absent; the installed rsvg-convert successfully supplied the alternative render. Render intermediates were kept outside the repository.
- tools/check_quality.py verified 3176 README words and 552 nonblank, comment-stripped production C lines, plus 91 header lines. Tests are excluded from these source counts. Every nonignored deliverable contains jayis1; no conflicting author credit or matched hardcoded-secret assignment was found. Build products, Python caches and KiCad local preferences are ignored.

## Cross-artifact review

LEFT_1, LEFT_2, RIGHT_1 and RIGHT_2 are connected to J3 through J6 and via SENSE0 through SENSE3 to the documented PA0, PA1, PA4 and PB0 harness. Each field connector uses signal on pin one and ground on pin two. Firmware maps low electrical inputs to asserted bits and uses the same paired-bit order as the application. The RS1 field order and CRC are exercised end to end from the C encoder to the Tk receive loop. The six bins represent direction-of-origin by absolute stage difference, not signed displacement. Shared circuitry and driver provenance are explicit rather than presented as new circuitry.

No incoming commands, network services, authentication secrets, dynamic target allocations, safety outputs or dependency downloads are introduced. Replay allocation is bounded and export refuses overwrite. A connected stream may still carry a HALTED instrument; the UI displays state and flags separately from transport freshness. The retained result is a latest snapshot, not a guaranteed archive of every completion.

## Not physically verified

Fabrication, component procurement, soldering, connector fit, programming, actual GPIO voltages, HSI accuracy, watchdog timing, signal wetting reliability, cam geometry, reed hysteresis, sensor force, fixture disturbance, mechanical repeatability, cable interference, USB behavior on a physical device, ESD/EMC and regulatory compliance remain bench work. The external Nucleo supply is outside passive-carrier ERC. No supplier-stock or price verification was performed. The product value is a proposed comparative workflow, not a demonstrated workshop productivity improvement.

A valid KiCad board and passing software tests do not establish fabrication readiness or a safe drawer. Use only supervised low-energy unpowered fixtures and disconnected voltage-free contacts. No measurement authorizes machine movement or certifies dimensional squareness.
