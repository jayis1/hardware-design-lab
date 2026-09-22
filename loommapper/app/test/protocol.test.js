/* LoomMapper protocol tests. Author: jayis1. Copyright (c) 2026 jayis1. */
'use strict';const test=require('node:test');const assert=require('node:assert/strict');const p=require('../protocol');
test('CRC-16 standard vector',()=>assert.equal(p.crc16(Buffer.from('123456789')),0x29b1));
test('command round trips through validator',()=>{const frame=p.command(0x82,[1]);const parsed=p.validate(frame);assert.equal(parsed.type,0x82);assert.deepEqual([...parsed.payload],[1]);});
test('corruption fails closed',()=>{const frame=p.command(0x80);frame[6]^=1;assert.throws(()=>p.validate(frame),/crc/);});
test('synthetic topology has one mapped path per source',()=>{const model=p.synthetic(1,false);assert.equal(model.edges.filter(e=>e.kind===1).length,8);assert.equal(model.edges.length,64);});
test('flex mode isolates a movement-correlated fault',()=>{const model=p.synthetic(2,true);const edge=model.edges.find(e=>e.source===4&&e.sink===0);assert.equal(edge.kind,4);assert.ok(edge.variation>500);});
