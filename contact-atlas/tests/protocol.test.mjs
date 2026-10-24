// Author: jayis1. SPDX-License-Identifier: MIT
import test from 'node:test';
import assert from 'node:assert/strict';
import {crc, parse, Framer, Session} from '../app/protocol.mjs';
const frame = s => `${s}*${crc(s).toString(16).padStart(4,'0').toUpperCase()}`;
const status = (seq=0,time=100) => frame(`CA1,S,${seq},${time},1,0,1,1,0,0,0,0,0,200`);
const event = (seq=1,time=310) => frame(`CA1,E,${seq},${time},1,1,2,210,10,0,2`);
test('CRC and strict versioned schema', () => {
 assert.equal(crc('123456789'),0x29b1);
 assert.equal(parse(status()).stable,1);
 assert.equal(parse(event()).zero,10);
 for (const s of ['oops',status().replace('CA1','CA2'),status()+'x',frame('CA1,S,0'),frame('CA1,E,1,2,1,16,2,210,10,0,2'),frame('CA1,E,1,2,1,1,2,10,20,0,2')])
  assert.throws(()=>parse(s));
});
test('fragmentation and overlength recovery', () => {
 const f = new Framer();
 assert.equal(f.feed(status().slice(0,8)).length,0);
 assert.equal(f.feed(status().slice(8)+'\n').length,1);
 assert.equal(f.feed('X'.repeat(1000)+'\n'+event()+'\n').length,1);
 assert.equal(f.errors,1);
});
test('status freshness, sequence gaps, restart, bounded history', () => {
 const s = new Session();
 assert.equal(s.accept(parse(event()),0),false);
 assert(s.accept(parse(status()),10));
 assert(s.accept(parse(event()),20));
 assert(!s.accept(parse(event()),21));
 assert(s.live(500)); assert(!s.live(1011));
 assert(s.accept(parse(status(4,500)),600));
 assert.equal(s.missing,2);
 assert(!s.accept(parse(status(0,10)),700));
 assert(s.discontinuity); assert.equal(s.events.length,1);
 assert(!s.live(701,false));
});
test('partial line resynchronizes at next CA1 marker',()=>{
 const f=new Framer();
 assert.equal(f.feed('CA1,S,0,'+status()+'\n').length,1);
 assert.equal(f.errors,1);
});
test('regressions fail closed without erasing evidence',()=>{
 for (const [oldTime,newTime] of [[0x90000000,100],[100,200],[1000,900]]) {
  const s=new Session();
  assert(s.accept(parse(status(10,oldTime)),0));
  assert(s.accept(parse(event(11)),1));
  assert(!s.accept(parse(status(9,newTime)),2));
  assert(s.discontinuity); assert(!s.live(3));
  assert.equal(s.events.length,1);
 }
});
test('history bound and normal uptime/sequence rollover',()=>{
 const s=new Session();
 assert(s.accept(parse(status(0,100)),0));
 for(let i=1;i<=1005;i++) assert(s.accept(parse(event(i,310)),i));
 assert.equal(s.events.length,1000); assert.equal(s.evicted,5);
 const w=new Session();
 assert(w.accept(parse(status(0xffffffff,0xfffffffe)),0));
 assert(w.accept(parse(status(0,10)),1));
 assert(w.live(2)); assert.equal(w.missing,0);
});
