# Snap History validation record

Author: jayis1

## Scope and mode

This was a standalone-owner implementation, not a full collective hardware review. Initial peer reports were advisory; reports naming creative-projects or other destinations were rejected against the verified checkout and remote jayis1/hardware-design-lab. A later read-only code review identified validator gaps, which the owner corrected and exercised with failing-then-passing regressions. No peer wrote repository files. The starting checkout was clean on main and `git pull --ff-only` reported already up to date.

## Reproduced automated results

Command: `/usr/bin/python3 tools/verify.py` from this device directory.

- GNU Arm Embedded GCC 14.2.1 linked the Cortex-M4 target with warnings treated as errors. Final executable size: 2424 bytes text, 0 initialized data, 788 static BSS. Stack use is additional; no physical target was programmed.
- Host GCC 14.2.0 ran the engine, clock and serial test executables under address and undefined-behavior sanitizers.
- Engine tests enumerated 65536 four-step contact-mask words against an independent prior-vector history. Separate tests cover held-closed startup, reopenings, tied vector changes, unrelated chatter, raw-glitch settlement cancellation, exact deadline precedence, log exhaustion, counter exhaustion, timestamp wrap and encoder capacity.
- Python 3.13.5 ran 15 model/transport/Tk tests under Xvfb. The production C encoder generated the explicitly synthetic fixture. GUI tests loaded that recording, then delivered it through a Linux PTY into the real application read loop; they checked retained history after disconnect, reconnect failure, exclusive/private CSV export and empty-session metadata.
- Review regressions first reproduced acceptance of impossible settlement-credit growth and DONE at exactly 60000 ms. Both are now rejected. History-transition negative tests now use fresh sessions so an earlier latched warning cannot mask later cases.
- KiCad CLI 9.0.2 ERC: zero violations. PCB DRC: zero violations and zero unconnected items.
- Fresh schematic XML export, PCB pads and connectivity manifest agree on 14 components, 30 pins and 10 nets, including component values and full footprint-library identities.
- Schematic SVG was rendered with KiCad, converted using rsvg-convert and visually inspected. Four SNAP_A through SNAP_D networks, connector labels and the jayis1 title block are legible without apparent label collisions. CairoSVG was unavailable; rsvg-convert supplied the rendering path.
- README: 3269 whitespace-delimited words. Production C: 522 nonblank/noncomment lines, excluding tests and headers. All delivered text files include jayis1 in supported comments, title blocks or metadata.
- Python byte-compilation and `git diff --check` passed. Generated build products, bytecode and KiCad local preference files are excluded from publication.

## Remaining engineering limitations

The carrier has not been fabricated or electrically inspected. There is no physical firmware flashing, measured timing, assembled reed/magnet calibration, actual textile trial, EMC/ESD evaluation, environmental qualification or regulatory assessment. A passing passive-only ERC cannot verify the external Nucleo supply. No claim of worldwide novelty or patent clearance is made.

The magnetic contact is a proximity proxy, not proof of mechanical snap engagement. The fixture must remain tabletop and isolated from powered electronics; it is not a worn product or a clinical assessment instrument. The carrier lacks field-rated ESD, isolation and reverse-polarity protection. Manual wiring, current-limited initial power and protective enclosure design remain necessary.

The receive-only protocol has no authenticated or persistent boot identity. Host transition checks are necessary consistency checks, not a reconstruction of hidden contact activity. Trial evidence is volatile, bounded to 32 groups and 60 seconds, and deliberately frozen on completion or fault. Reset only after exporting evidence and reconnect explicitly afterward.
