/* HingeScribe protocol tests. Author: jayis1. Copyright (c) 2026 jayis1. */
'use strict';const test=require('node:test');const assert=require('node:assert/strict');const p=require('../protocol');
test('CRC-16/CCITT-FALSE standard vector',()=>assert.equal(p.crc16(Uint8Array.from(Buffer.from('123456789'))),0x29b1));
test('synthetic readings degrade deterministically',()=>{assert.equal(p.synthetic(1).health,93);assert.equal(p.synthetic(6).flags,1);assert.ok(p.synthetic(9).health<p.synthetic(2).health);});
test('flag labels are actionable',()=>{assert.deepEqual(p.flags(5),['Opening resistance is above baseline','Closer is taking too long']);});
test('short frames fail closed',()=>assert.throws(()=>p.validate(new Uint8Array([1,2,3])),/short/));
