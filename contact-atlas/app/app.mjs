// Author: jayis1. SPDX-License-Identifier: MIT
import {Framer, Session} from './protocol.mjs';
const $ = id => document.getElementById(id);
let session = new Session(), framer = new Framer();
let generation=0, busy=false, reader=null, mode='DISCONNECTED';
const mask = n => n.toString(2).padStart(4,'0');
function reset(source) { session=new Session(); framer=new Framer(); mode=source; $('error').textContent=''; }
function ingest(text) { for (const row of framer.feed(text)) session.accept(row,performance.now()); }
function render() {
 const live = mode==='SERIAL' && session.live(performance.now());
 $('mode').textContent = `${mode} — ${live ? 'LIVE' : 'evidence is not live'}`;
 if (session.discontinuity) $('error').textContent='Session discontinuity: export evidence, then disconnect and reconnect.';
 const s=session.status;
 $('status').textContent = s ? `Raw mask: ${mask(s.raw)}\nQualified mask: ${s.known ? mask(s.stable) : 'UNKNOWN'}\nMoving: ${Boolean(s.travelling)}\nCandidate age: ${s.age} ms\nCompleted transitions: ${s.transitions}\nSample gaps: ${s.gaps}; invalid samples: ${s.invalid}\nDevice queue drops: ${s.dropped}; TX losses: ${s.loss}\nTransport missing: ${session.missing}; malformed: ${framer.errors}; rejected: ${session.rejected}\nEpoch: ${session.epoch}; status age: ${Math.max(0,Math.floor(performance.now()-session.received))} ms` : 'No evidence received';
 const edges=new Map();
 for (const e of session.events) {
  const key=`${e.from}:${e.to}`;
  const a=edges.get(key) || [mask(e.from),mask(e.to),0,Infinity,0,0,0];
  a[2]++; a[3]=Math.min(a[3],e.duration); a[4]=Math.max(a[4],e.duration); a[5]+=e.zero; a[6]+=e.multi; edges.set(key,a);
 }
 const rows=[];
 for (const values of edges.values()) {
  const tr=document.createElement('tr');
  for (const value of values) { const td=document.createElement('td'); td.textContent=String(value); tr.append(td); }
  rows.push(tr);
 }
 $('edges').replaceChildren(...rows);
 $('counts').textContent=`${session.events.length} retained events; ${session.evicted} evicted. Map covers retained events only.`;
 $('connect').disabled=busy;
 $('replay').disabled=busy;
}
for (const button of document.querySelectorAll('[data-screen]')) button.onclick=()=>{
 for (const id of ['live','map','connection']) $(id).hidden=id!==button.dataset.screen;
};
$('connect').onclick=async()=>{
 if (busy) return;
 if (!navigator.serial) { $('error').textContent='Web Serial is unavailable. Use supported desktop Chromium on localhost.'; return; }
 busy=true; const token=++generation; let port=null, opened=false, localReader=null;
 reset('CONNECTING'); render();
 try {
  port=await navigator.serial.requestPort();
  if (token!==generation) return;
  await port.open({baudRate:115200,dataBits:8,stopBits:1,parity:'none',flowControl:'none'});
  opened=true;
  if (token!==generation) return;
  mode='SERIAL'; localReader=port.readable.getReader(); reader=localReader;
  const decoder=new TextDecoder('ascii');
  while (token===generation) {
   const {value,done}=await localReader.read();
   if (done || token!==generation) break;
   ingest(decoder.decode(value,{stream:true})); render();
  }
 } catch(e) { if (token===generation) $('error').textContent=String(e.message || e); }
 finally {
  if (localReader) localReader.releaseLock();
  reader=null;
  if (opened) { try { await port.close(); } catch(e) { $('error').textContent=`Close failed: ${e.message}`; } }
  mode='DISCONNECTED'; busy=false; render();
 }
};
$('disconnect').onclick=async()=>{
 ++generation; mode=busy && !reader ? 'CANCELLING — dismiss the port chooser or wait for pending operation' : 'DISCONNECTED'; render();
 if (reader) { try { await reader.cancel(); } catch(e) { $('error').textContent=e.message; } }
};
$('replay').onchange=async()=>{
 const file=$('replay').files[0];
 if (!file || busy) return;
 if (file.size>1048576) { $('error').textContent='Replay exceeds 1 MiB'; return; }
 busy=true; const token=++generation; reset('LOADING REPLAY'); render();
 try {
  const text=await file.text();
  if (token!==generation) { mode='DISCONNECTED'; return; }
  mode='SYNTHETIC REPLAY'; ingest(text);
 } catch(e) { $('error').textContent=e.message; }
 finally { busy=false; render(); }
};
$('export').onclick=()=>{
 const header='author,source,epoch,status_age_ms,live,seq,event,uptime_ms,from,to,duration_ms,zero_ms,multi_ms,changes';
 const age=session.status ? Math.max(0,Math.floor(performance.now()-session.received)) : '';
 const live=mode==='SERIAL' && session.live(performance.now());
 const rows=session.events.map(e=>['jayis1',mode,e.epoch,age,live,e.seq,e.number,e.time,e.from,e.to,e.duration,e.zero,e.multi,e.changes].join(','));
 const blob=new Blob([header+'\n'+rows.join('\n')+'\n'],{type:'text/csv'});
 const url=URL.createObjectURL(blob); const a=document.createElement('a');
 a.href=url; a.download='contact-atlas-events.csv'; a.click(); setTimeout(()=>URL.revokeObjectURL(url),1000);
};
setInterval(render,250);
render();
