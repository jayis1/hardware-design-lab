# Ratio Window validation record

Author: jayis1

## Scope and repository

Executed in degraded standalone-owner mode. Advisory peer reports were treated as recommendations, not evidence. The verified repository was jayis1/hardware-design-lab, its remote default branch was main, and `git pull --ff-only` reported already up to date before implementation. No changes were made to daily-ideas or another repository. No task branch or new automation workflow was created.

The device inventory was checked before selection. The closest purpose contracts, WebSlack Ledger and Tooth Census, were read in full. Dual-encoder gear measurement is established prior art; Ratio Window claims a differentiated manual experiment workflow, not worldwide invention of the underlying measurement principle.

## Executed software checks

- Clean ARM target build: GNU Arm Embedded GCC 14.2.1, Cortex-M4 Thumb, freestanding C11, warnings treated as errors. ELF, map and binary generated successfully. `arm-none-eabi-size` reports 2852 bytes text, 0 bytes initialized data and 788 bytes BSS. Stack and local variables are additional runtime memory; the linker reserves headroom.
- Native firmware tests: GCC 14.2.0 with AddressSanitizer and UndefinedBehaviorSanitizer; three test executables passed for the measurement engine, clock/configuration code and production UART driver.
- Quadrature verification covers all sixteen ordered phase pairs against a separate ring oracle. Ratio patterns cover twelve divisors in both output directions across two consecutive windows. Tests also cover zero output, input/output reversal, diagonal jump, rearm after rejection, a final-rearm raw glitch, timeout equality versus the preceding completion, output capacity, timer wrap, acquisition loss, terminal freeze, sample exhaustion, CRC and bounded encoding.
- Target and native compilers exposed an initial pointer-type mismatch; the direction helper now uses uint32_t consistently on both architectures. A rearm test was corrected to respect the implementation's explicit prior-clean-endpoint rule. New negative tests demonstrated missing latest-category checks before matching C and Python fixes were applied.
- Python 3.13.5: nine unittest methods passed under Xvfb, including real C-encoder fixture parsing, bytewise fragmentation, truncation recovery, malformed and semantically impossible frames, duplicate freshness, regression, sparse impossible completion, strict replay, private CSV permissions and serial attribute restoration.
- The GUI test exercises actual Tk widgets and the production POSIX serial reader via a paced Linux pseudo-terminal. It verifies both completed synthetic windows, all three tabs, notebook rows, plotted data, connected CSV export, peer hangup, disconnected evidence retention and replay. It does not simulate a physical USB controller or prove target flashing.
- Synthetic fixture outcome: two accepted windows, one same-direction count of 96, one opposite-direction count of 48. These are generated inputs, not physical measurements.

## Executed CAD checks

KiCad 9.0.2 and its system Python pcbnew bindings generated and opened the project. Native ERC returned zero violations. Native DRC returned zero violations and zero unconnected items. A freshly exported schematic XML netlist matched the manifest and actual PCB pads for fourteen components, thirty pins and ten nets; values and full library footprint identities also matched.

The schematic was exported as SVG, rasterized with rsvg-convert and visually inspected. References, values, input/output net labels, power header and author title block were legible and separated. The PCB is an actual routed passive carrier, not an MCU design with missing power circuitry. The external Nucleo, encoder breakouts and harness are outside the passive-board ERC/DRC scope.

## Reproduction and audit

Recovery verification by jayis1: the untracked directory was traced to the interrupted execution of this same authorized cron task, rather than adopted as unrelated work. After a fresh fast-forward pull, the target build was cleaned and the complete release gate rerun successfully: three native sanitizer executables, nine Python/Tk tests, zero ERC/DRC violations, zero unconnected items, and exact 14-component/30-pin/10-net agreement. The release audit counted 36 attributed files, 3246 README words and 631 nonblank firmware C/header lines excluding block comments. A newly exported schematic was rendered and visually checked again. Publication remains limited to this recovered device directory on main; no physical validation is implied.

Run `python3 tools/check_release.py` from this directory. It invokes the target/native builds, the Python/GUI tests and native CAD checks, then checks required files, jayis1 attribution, a basic literal-secret pattern scan, README word count, substantive C/header line count and whitespace. This scan is not a comprehensive security audit. `tools/import_platform.py` records exact shared-platform provenance and refuses to overwrite files.

Build products, caches and KiCad local settings are ignored. All authored source and design files carry jayis1 attribution in syntax appropriate to their formats. Installed footprint geometry retains upstream provenance rather than being relabelled as original work. The README contains 3246 whitespace-separated words. Firmware exceeds five hundred nonblank C/header lines after block comments are removed, without counting test fixtures or generated build output.

## Not physically verified

No PCB was fabricated, soldered or powered; no target was flashed; no encoder, gear train or coupling was assembled. Contact wetting current, phase plateau duration, bounce filtering, overspeed aliasing, input voltage margins, ground integrity, MCU timing jitter, watchdog recovery on real silicon, shaft loading, coupling slip, EMC, ESD and practical teaching value remain untested. No calibration, safety certification, precision transmission-error claim or production-readiness claim is made.

Manufacturer information was checked through retrieved search extracts. The direct lowercase Bourns PDF request returned HTTP 403; the manufacturer-indexed uppercase PEC11R datasheet and ST UM1724 extracts supplied the ordering-code, pin-map and VCP references. Builders must review current complete datasheets and their purchased board revision before assembly. Hardware limitations are engineering acceptance work, not passing software test results.
