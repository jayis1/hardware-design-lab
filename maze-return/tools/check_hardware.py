#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""ERC/DRC and exact schematic/PCB/BOM pin connectivity comparison."""
import json
import pathlib
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import pcbnew
root = pathlib.Path(__file__).resolve().parents[1]
kicad = root / 'kicad'
expected = {c['ref']: c for c in json.loads((kicad / 'connectivity.json').read_text())['components']}
with tempfile.TemporaryDirectory(prefix='ledger-eda-') as temporary:
    temporary = pathlib.Path(temporary)
    for kind, command, extension in [('sch', 'erc', 'kicad_sch'), ('pcb', 'drc', 'kicad_pcb')]:
        subprocess.run(['kicad-cli', kind, command, '--exit-code-violations',
                        '-o', str(temporary / (command + '.rpt')),
                        str(kicad / ('device.' + extension))], check=True)
    netlist = temporary / 'device.xml'
    subprocess.run(['kicad-cli', 'sch', 'export', 'netlist', '--format', 'kicadxml',
                    '-o', str(netlist), str(kicad / 'device.kicad_sch')], check=True)
    xml = ET.parse(netlist).getroot()
    schematic = {}
    for net in xml.findall('./nets/net'):
        for node in net.findall('node'):
            schematic[node.attrib['ref'], node.attrib['pin']] = net.attrib['name'].lstrip('/')
    board = pcbnew.LoadBoard(str(kicad / 'device.kicad_pcb'))
    actual = {}
    for fp in board.GetFootprints():
        ref = fp.GetReference()
        assert ref in expected, ref
        assert fp.GetValue() == expected[ref]['value'], ref
        identity = fp.GetFPID()
        full_name = str(identity.GetLibNickname()) + ':' + str(identity.GetLibItemName())
        assert full_name == expected[ref]['footprint'], ref
        for pad in fp.Pads():
            actual[ref, pad.GetNumber()] = pad.GetNetname()
    target = {(ref, str(i + 1)): net for ref, c in expected.items() for i, net in enumerate(c['nets'])}
    assert actual == target
    assert schematic == target
    for c in xml.findall('./components/comp'):
        ref = c.attrib['ref']
        assert c.findtext('value') == expected[ref]['value']
        assert c.findtext('footprint') == expected[ref]['footprint']
    print(f'PASS: {len(expected)} components, {len(target)} pins, {len(set(target.values()))} nets')
