/* StitchScope companion application
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
'use strict';
const $ = selector => document.querySelector(selector);
const $$ = selector => [...document.querySelectorAll(selector)];
const classNames = ['OK', 'SKIP SUSPECT', 'TOP THREAD BREAK', 'BOBBIN DEPLETION', 'FEED STALL', 'HARD STRIKE', 'BIRDNEST GROWTH', 'UNTRAINED'];
const state = { device: null, server: null, control: null, live: null, sequence: 0, running: false, simulator: null, history: [], currentSession: [], selectedSession: null, calibration: 0 };

function stored(key, fallback) { try { return JSON.parse(localStorage.getItem(key)) ?? fallback; } catch (_) { return fallback; } }
function persist(key, value) { localStorage.setItem(key, JSON.stringify(value)); }
function command(type, payload = new Uint8Array()) {
  if (!state.control) return Promise.reject(new Error('Device is not connected'));
  state.sequence = (state.sequence + 1) & 0xffff;
  return state.control.writeValueWithoutResponse(StitchProtocol.encode(type, state.sequence, payload));
}

async function connect() {
  if (!navigator.bluetooth) throw new Error('Web Bluetooth is not available in this browser');
  state.device = await navigator.bluetooth.requestDevice({ filters: [{ namePrefix: 'StitchScope' }], optionalServices: [StitchProtocol.SERVICE] });
  state.device.addEventListener('gattserverdisconnected', disconnected);
  state.server = await state.device.gatt.connect();
  const service = await state.server.getPrimaryService(StitchProtocol.SERVICE);
  [state.control, state.live] = await Promise.all([service.getCharacteristic(StitchProtocol.CONTROL), service.getCharacteristic(StitchProtocol.LIVE)]);
  await state.live.startNotifications();
  state.live.addEventListener('characteristicvaluechanged', event => {
    try { renderPacket(StitchProtocol.decodeLive(new Uint8Array(event.target.value.buffer))); }
    catch (error) { showAlert(`Dropped invalid packet: ${error.message}`, false); }
  });
  $('#connect').textContent = state.device.name;
  $('#session').disabled = false;
  $('#connectionHint').textContent = 'Ready. Load the matching recipe before monitoring.';
}

function disconnected() {
  state.control = null; state.live = null; state.running = false;
  $('#connect').textContent = 'Connect'; $('#session').disabled = true;
  $('#classification').textContent = 'Disconnected';
  showAlert('Bluetooth connection ended. The local session has been retained.', false);
}

function showAlert(message, severe) {
  const alert = $('#alert'); alert.textContent = message; alert.classList.remove('hidden');
  alert.style.background = severe ? '#7a1f30' : '#594716'; $('#ack').disabled = false;
}

function renderPacket(packet) {
  const classification = classNames[packet.classification] || 'UNKNOWN';
  $('#classification').textContent = classification;
  $('#quality').textContent = `${Math.round(Math.max(0, Math.min(1, packet.quality)) * 100)}%`;
  $('#rate').textContent = packet.rate; $('#force').textContent = packet.force;
  $('#feed').textContent = packet.feed.toFixed(2); $('#impulse').textContent = packet.impulse;
  $('#battery').textContent = packet.battery;
  const severe = [2, 5].includes(packet.classification);
  $('#classification').style.color = packet.classification === 0 ? 'var(--accent)' : severe ? 'var(--danger)' : 'var(--warn)';
  if (packet.classification > 0 && packet.classification < 7) showAlert(`${classification} at cycle ${packet.cycle}. Inspect the seam and machine.`, severe);
  state.history.push(packet); if (state.history.length > 120) state.history.shift();
  if (state.running) state.currentSession.push({ ...packet, at: new Date().toISOString() });
  drawChart();
}

function drawChart() {
  const canvas = $('#chart'), ctx = canvas.getContext('2d'), dpr = devicePixelRatio || 1;
  const width = canvas.clientWidth || 900, height = canvas.clientHeight || 220;
  canvas.width = width * dpr; canvas.height = height * dpr; ctx.scale(dpr, dpr);
  ctx.clearRect(0, 0, width, height); ctx.strokeStyle = '#244058'; ctx.lineWidth = 1;
  for (let i = 1; i < 4; i += 1) { const y = i * height / 4; ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke(); }
  if (state.history.length < 2) return;
  ctx.strokeStyle = '#19d3ae'; ctx.lineWidth = 2; ctx.beginPath();
  state.history.forEach((sample, i) => { const x = i * width / 119, y = height - Math.max(0, Math.min(1, sample.quality)) * height; i ? ctx.lineTo(x, y) : ctx.moveTo(x, y); }); ctx.stroke();
  state.history.forEach((sample, i) => { if (sample.classification) { ctx.fillStyle = '#ff5d73'; ctx.fillRect(i * width / 119 - 2, 4, 4, 12); } });
}

function simulatorPacket() {
  const cycle = state.history.length ? state.history[state.history.length - 1].cycle + 1 : 1;
  const fault = cycle % 37 === 0 ? 4 : cycle % 53 === 0 ? 5 : 0;
  renderPacket({ cycle, classification: fault, quality: fault ? .18 : .88 + Math.random() * .1,
    rate: 1180 + Math.round(Math.random() * 60), force: 510 + Math.round(Math.random() * 45),
    feed: fault === 4 ? .08 : 2.45 + Math.random() * .12, impulse: fault === 5 ? 9400 : 720 + Math.round(Math.random() * 90), battery: 84, capabilities: 0x1f });
}

function toggleSimulator() {
  if (state.simulator) { clearInterval(state.simulator); state.simulator = null; $('#simulate').textContent = 'Run simulator'; return; }
  $('#connectionHint').textContent = 'SIMULATED data — no hardware measurements are being displayed.';
  state.simulator = setInterval(simulatorPacket, 250); $('#simulate').textContent = 'Stop simulator';
}

async function toggleSession() {
  if (!state.running) { if (state.control) await command(0x10); state.currentSession = []; state.running = true; $('#session').textContent = 'Stop session'; }
  else { if (state.control) await command(0x11); state.running = false; $('#session').textContent = 'Start session'; saveSession(); }
}

function saveSession() {
  if (!state.currentSession.length) return;
  const sessions = stored('stitchscope.sessions', []);
  sessions.unshift({ id: crypto.randomUUID(), started: state.currentSession[0].at, ended: state.currentSession.at(-1).at, records: state.currentSession });
  persist('stitchscope.sessions', sessions.slice(0, 30)); renderSessions();
}

function renderRecipes() {
  const recipes = stored('stitchscope.recipes', []), list = $('#recipeList'); list.textContent = '';
  recipes.forEach(recipe => { const article = document.createElement('article');
    const info = document.createElement('div'); const title = document.createElement('b'); title.textContent = recipe.name;
    const detail = document.createElement('p'); detail.textContent = `${recipe.needle} · ${recipe.length} mm · sensitivity ${recipe.sensitivity}`;
    info.append(title, detail); const use = document.createElement('button'); use.textContent = 'Use'; use.onclick = () => { persist('stitchscope.activeRecipe', recipe); use.textContent = 'Active'; };
    article.append(info, use); list.append(article); });
}

function renderSessions() {
  const sessions = stored('stitchscope.sessions', []), list = $('#sessionList'); list.textContent = '';
  sessions.forEach(session => { const faults = session.records.filter(r => r.classification > 0).length; const article = document.createElement('article');
    const info = document.createElement('div'), title = document.createElement('b'), detail = document.createElement('p');
    title.textContent = new Date(session.started).toLocaleString(); detail.textContent = `${session.records.length} stitches · ${faults} fault votes`; info.append(title, detail);
    const select = document.createElement('button'); select.className = 'secondary'; select.textContent = 'Select'; select.onclick = () => { state.selectedSession = session; $('#export').disabled = false; };
    article.append(info, select); list.append(article); });
}

function exportCSV() {
  if (!state.selectedSession) return;
  const keys = ['at', 'cycle', 'classification', 'quality', 'rate', 'force', 'feed', 'impulse', 'battery'];
  const rows = [keys.join(','), ...state.selectedSession.records.map(record => keys.map(key => JSON.stringify(record[key])).join(','))];
  const url = URL.createObjectURL(new Blob([rows.join('\n')], { type: 'text/csv' }));
  const link = document.createElement('a'); link.href = url; link.download = `stitchscope-${state.selectedSession.id}.csv`; link.click(); URL.revokeObjectURL(url);
}

$$('.tab').forEach(tab => tab.onclick = () => { $$('.tab,.view').forEach(x => x.classList.remove('active')); tab.classList.add('active'); $(`#${tab.dataset.view}`).classList.add('active'); });
$('#connect').onclick = () => connect().catch(error => showAlert(error.message, false));
$('#simulate').onclick = toggleSimulator; $('#session').onclick = () => toggleSession().catch(error => showAlert(error.message, false));
$('#ack').onclick = () => { $('#alert').classList.add('hidden'); $('#ack').disabled = true; };
$$('[data-cal]').forEach(button => button.onclick = async () => { try { await command({ zero: 0x20, phase: 0x21, seam: 0x22 }[button.dataset.cal]); state.calibration += 1; $('#calProgress').value = state.calibration; $('#calStatus').textContent = `${button.textContent} command accepted (${state.calibration}/3).`; button.disabled = true; } catch (error) { showAlert(error.message, false); } });
$('#recipeForm').onsubmit = event => { event.preventDefault(); const recipes = stored('stitchscope.recipes', []); recipes.push({ id: crypto.randomUUID(), name: $('#recipeName').value, needle: $('#needle').value, length: Number($('#stitchLength').value), sensitivity: Number($('#sensitivity').value) }); persist('stitchscope.recipes', recipes); event.target.reset(); renderRecipes(); };
$('#export').onclick = exportCSV; $('#clearSessions').onclick = () => { if (confirm('Delete all locally stored StitchScope sessions?')) { persist('stitchscope.sessions', []); state.selectedSession = null; $('#export').disabled = true; renderSessions(); } };
window.addEventListener('resize', drawChart); if ('serviceWorker' in navigator) navigator.serviceWorker.register('./sw.js');
renderRecipes(); renderSessions(); drawChart();
