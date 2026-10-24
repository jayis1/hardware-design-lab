// Author: jayis1. SPDX-License-Identifier: MIT
import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import fs from 'node:fs';
import {Stream, Session, states, reasons} from '../app/protocol.mjs';
const source = fs.readFileSync(new URL('../app/app.mjs', import.meta.url), 'utf8').replace(/^import .*;$/m, '');
const frame = 'FT1,42,1084,1,0,0,0,0,0,0,0,0,0,0,0,0*BE6F\n';
function deferred() { let resolve; const promise = new Promise(r => resolve=r); return {promise,resolve}; }
function harness(serial) {
  const elements = new Map();
  const element = () => ({textContent:'',value:'',replaceChildren(){},click(){}});
  const get = id => { if (!elements.has(id)) elements.set(id, element()); return elements.get(id); };
  vm.runInNewContext(source, {Stream,Session,states,reasons,navigator:{serial},
    document:{getElementById:get,querySelectorAll:()=>[],createElement:element},
    localStorage:{getItem:()=>null,setItem(){}}, performance:{now:()=>100},
    setInterval(){},setTimeout(){},TextDecoder,Blob,URL,Date});
  return get;
}
test('replay and serial connection cannot overlap', async () => {
  const file = deferred(); let requested=0;
  const get = harness({requestPort:async()=>{requested++; throw Error('unexpected request');}});
  const replay = get('replay').onchange({target:{files:[{size:100,text:()=>file.promise}]}});
  await get('connect').onclick();
  file.resolve(frame); await replay;
  assert.equal(requested,0);
  assert.match(get('connection').textContent,/SYNTHETIC REPLAY/);
  assert.match(get('recent').textContent,/SYNTHETIC REPLAY/);
});
test('disconnect cancels delayed port opening', async () => {
  const open = deferred(); let reads=0,closed=0;
  const get = harness({requestPort:async()=>({open:()=>open.promise,close:async()=>{closed++;},
    readable:{getReader:()=>({read:async()=>{reads++; return {done:true};},releaseLock(){}})}})});
  const connect = get('connect').onclick();
  await Promise.resolve();
  await get('disconnect').onclick();
  open.resolve(); await connect;
  assert.equal(reads,0); assert.equal(closed,1);
  assert.match(get('connection').textContent,/Disconnected/);
});
