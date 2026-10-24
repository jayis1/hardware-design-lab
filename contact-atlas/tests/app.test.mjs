// Author: jayis1. SPDX-License-Identifier: MIT
// Exercise the real UI handlers with explicit DOM/serial mocks.
import test from 'node:test';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {mkdtempSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
test('UI: C replay, screens, export, cancel races and serial freshness', async()=>{
 const temp=mkdtempSync(join(tmpdir(),'atlas-ui-'));
 let fixture;
 try {
  execFileSync('gcc',['-std=c11','-Wall','-Wextra','-Werror','-Ifirmware','tests/fixture.c','firmware/engine.c','firmware/wire.c','-o',join(temp,'fixture')],{cwd:new URL('..',import.meta.url)});
  fixture=execFileSync(join(temp,'fixture'),{encoding:'utf8'});
 } finally { rmSync(temp,{recursive:true}); }
 class Element {
  constructor() { this.textContent=''; this.hidden=false; this.files=[]; this.children=[]; }
  append(child) { this.children.push(child); }
  replaceChildren(...children) { this.children=children; }
  click() { this.clicked=true; }
 }
 const elements=new Map();
 const get=id=>{if(!elements.has(id))elements.set(id,new Element());return elements.get(id);};
 const buttons=['live','map','connection'].map(id=>Object.assign(new Element(),{dataset:{screen:id}}));
 globalThis.document={getElementById:get,querySelectorAll:()=>buttons,createElement:()=>new Element()};
 const callbacks=[];
 const realInterval=globalThis.setInterval;
 globalThis.setInterval=cb=>{callbacks.push(cb);return 1;};
 let now=0;
 Object.defineProperty(globalThis,'performance',{value:{now:()=>now},configurable:true});
 Object.defineProperty(globalThis,'navigator',{value:{},configurable:true});
 let exported;
 const create=URL.createObjectURL;
 URL.createObjectURL=blob=>{exported=blob;return 'blob:test';};
 try {
  await import('../app/app.mjs');
  buttons[1].onclick(); assert.equal(get('map').hidden,false); assert.equal(get('live').hidden,true);
  await get('connect').onclick(); assert.match(get('error').textContent,/unavailable/);
  get('replay').files=[{size:fixture.length,text:async()=>fixture}];
  await get('replay').onchange();
  assert.match(get('mode').textContent,/SYNTHETIC REPLAY/);
  assert.equal(get('edges').children.length,2);
  get('export').onclick();
  const csv=await exported.text();
  assert.match(csv,/jayis1,SYNTHETIC REPLAY/);
  assert.match(csv,/,1,2,210,10,0,2/);
  let finish;
  get('replay').files=[{size:1,text:()=>new Promise(resolve=>finish=resolve)}];
  const pending=get('replay').onchange();
  await get('disconnect').onclick(); finish(fixture); await pending;
  assert.match(get('mode').textContent,/DISCONNECTED/);
  assert.equal(get('edges').children.length,0);
  let resolveOpen,closed=0;
  navigator.serial={requestPort:async()=>({open:()=>new Promise(resolve=>resolveOpen=resolve),close:async()=>closed++})};
  const opening=get('connect').onclick();
  await Promise.resolve(); await get('disconnect').onclick();
  assert.match(get('mode').textContent,/CANCELLING/);
  assert.equal(get('connect').disabled,true);
  resolveOpen(); await opening;
  assert.equal(closed,1); assert.match(get('mode').textContent,/DISCONNECTED/);
  let releaseRead;
  const r={read:()=>new Promise(resolve=>releaseRead=resolve),cancel:async()=>releaseRead({done:true}),releaseLock:()=>{}};
  navigator.serial={requestPort:async()=>({open:async()=>{},close:async()=>closed++,readable:{getReader:()=>r}})};
  const connection=get('connect').onclick();
  await Promise.resolve(); await Promise.resolve();
  releaseRead({value:new TextEncoder().encode(fixture),done:false});
  await Promise.resolve();
  assert.match(get('mode').textContent,/SERIAL — LIVE/);
  now=1001; callbacks[0](); assert.match(get('mode').textContent,/not live/);
  await get('disconnect').onclick(); await connection;
  assert.equal(closed,2);
 } finally { globalThis.setInterval=realInterval; URL.createObjectURL=create; }
});
