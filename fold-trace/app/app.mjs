// Author: jayis1. SPDX-License-Identifier: MIT
import {Stream, Session, states, reasons} from './protocol.mjs';
const $ = id => document.getElementById(id);
let port = null, reader = null, running = false;
let busy = false, generation = 0;
let stream = new Stream(), session = new Session(), history = [], mode = 'Disconnected';
let prefix = 'Fold';
try { prefix = localStorage.getItem('fold-label') || prefix; } catch { /* Storage is optional. */ }
$('label').value = prefix;
for (const button of document.querySelectorAll('[data-screen]')) {
  button.onclick = () => {
    for (const id of ['live', 'history', 'setup']) $(id).hidden = id !== button.dataset.screen;
  };
}
function render() {
  const r = session.last;
  const fresh = mode === 'Live' && running && session.fresh(performance.now());
  $('connection').textContent = `${mode} — ${fresh ? 'fresh telemetry' : 'NOT LIVE / stale evidence'}`;
  if (!r) return;
  $('summary').textContent = `${states[r.state]} / ${reasons[r.reason]} | accepted ${r.good}, rejected ${r.bad} | last cycle ${r.duration} ms`;
  $('quality').textContent = `Sampling gaps ${r.gaps}; device TX loss ${r.loss}; missing records ${session.missing}; invalid frames ${stream.errors}; inferred epoch ${session.epoch}`;
  $('stations').replaceChildren(...r.dwell.map((dwell, i) => {
    const card = document.createElement('div');
    card.className = 'station' + ((r.mask & (1 << i)) ? ' closed' : '');
    card.textContent = `${prefix} ${i+1}: ${(r.mask & (1 << i)) ? 'closed' : 'open'}; last completed hold ${dwell} ms`;
    return card;
  }));
  $('count').textContent = `${history.length} snapshots retained`;
  $('recent').textContent = history.slice(-15).reverse().map(row =>
    `${row.mode} epoch ${row.epoch} #${row.seq} ${row.uptime} ms ${states[row.state]} ${reasons[row.reason]}`).join('\n');
}
function ingest(chunk) {
  for (const r of stream.push(chunk)) {
    if (!session.accept(r, performance.now())) continue;
    history.push({...r, epoch: session.epoch, mode, received: new Date().toISOString()});
    if (history.length > 1000) history.shift();
  }
  render();
}
function resetSession() {
  session = new Session(); stream = new Stream(); history = [];
  $('summary').textContent = 'Waiting for telemetry';
  $('stations').replaceChildren(); $('quality').textContent = '';
  $('count').textContent = '0 snapshots'; $('recent').textContent = '';
}
async function disconnect() {
  generation++;
  running = false;
  mode = 'Disconnected';
  render();
  if (reader) await reader.cancel().catch(() => {});
  // The read loop releases its lock before closing the port.
}
$('disconnect').onclick = disconnect;
$('connect').onclick = async () => {
  if (busy) return;
  if (!navigator.serial) { $('connection').textContent = 'Web Serial unavailable: use Chromium on localhost or HTTPS'; return; }
  busy = true;
  const token = ++generation;
  try {
    port = await navigator.serial.requestPort();
    if (token !== generation) return;
    await port.open({baudRate: 115200});
    if (token !== generation) return;
    resetSession(); mode = 'Live'; running = true;
    reader = port.readable.getReader();
    const decoder = new TextDecoder();
    render();
    while (running) {
      const {value, done} = await reader.read();
      if (done || token !== generation) break;
      ingest(decoder.decode(value, {stream: true}));
    }
  } catch (error) {
    mode = `Disconnected (${error.message})`;
  } finally {
    running = false;
    if (reader) { reader.releaseLock(); reader = null; }
    if (port) { await port.close().catch(() => {}); port = null; }
    busy = false;
    if (mode === 'Live') mode = 'Disconnected';
    render();
  }
};
$('save').onclick = () => {
  prefix = $('label').value.slice(0,24).trim() || 'Fold';
  try { localStorage.setItem('fold-label', prefix); } catch { /* Keep in RAM. */ }
  render();
};
$('replay').onchange = async event => {
  if (busy) { $('connection').textContent = 'Wait for current operation or disconnect serial before replay'; event.target.value = ''; return; }
  const file = event.target.files[0];
  if (!file) return;
  if (file.size > 1000000) { $('connection').textContent = 'Replay exceeds 1 MB limit'; return; }
  busy = true;
  const token = ++generation;
  resetSession(); mode = 'SYNTHETIC REPLAY';
  try {
    const text = await file.text();
    if (token === generation) ingest(text);
  } catch (error) {
    mode = `Replay failed (${error.message})`;
  } finally {
    busy = false;
    render();
  }
};
$('export').onclick = () => {
  const columns = ['mode','epoch','received','seq','uptime','state','reason','step','mask','good','bad','gaps','duration','loss'];
  const rows = ['# Author: jayis1', '# Exported status snapshots; not a lossless event log', columns.join(',')];
  for (const r of history) rows.push(columns.map(k => r[k]).join(','));
  const url = URL.createObjectURL(new Blob([rows.join('\n')], {type: 'text/csv'}));
  const a = document.createElement('a'); a.href = url; a.download = 'fold-trace.csv'; a.click();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
};
setInterval(render, 250);
render();
