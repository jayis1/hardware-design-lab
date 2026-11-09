# Author: jayis1. SPDX-License-Identifier: MIT
"""Cross-check physical cam center samples against the firmware Gray contract."""
import math
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

class MechanicsTests(unittest.TestCase):
    def test_all_sector_centers(self):
        root=ET.parse(Path(__file__).resolve().parents[1]/'mechanical/cam-plates.svg').getroot()
        ns={'s':'http://www.w3.org/2000/svg'}
        profiles=root.findall('s:polygon',ns)
        self.assertEqual(len(profiles),4)
        for sector in range(16):
            code=0
            for bit,profile in enumerate(profiles):
                points=[tuple(map(float,p.split(','))) for p in profile.attrib['points'].split()]
                self.assertEqual(len(points),272)
                x,y=points[17*sector+8]
                cx=65+(bit%2)*130; cy=65+(bit//2)*130
                radius=math.hypot(x-cx,y-cy)
                self.assertTrue(abs(radius-50)<.001 or abs(radius-54)<.001)
                if radius>52: code|=1<<bit
            self.assertEqual(code,sector^(sector>>1))
        self.assertEqual(root.attrib['width'],'260mm')
        self.assertEqual(root.attrib['height'],'290mm')
        metadata=root.find('s:metadata',ns)
        assert metadata is not None and metadata.text is not None
        self.assertIn('jayis1',metadata.text)
if __name__=='__main__': unittest.main()
