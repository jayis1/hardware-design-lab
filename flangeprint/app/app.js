/* FlangePrint application controller.
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
'use strict';
const byId = id => document.getElementById(id);
let step = 0;
let frame = FlangeProtocol.simulatedStatus(step);
const stored = () => JSON.parse(localStorage.getItem('flangeprint.jobs') || '[]');
const saveJobs = jobs => localStorage.setItem('flangeprint.jobs', JSON.stringify(jobs));
function escapeText(text) {
  const node = document.createElement('span');
  node.textContent = String(text);
  return node.innerHTML;
}
function renderLive() {
  byId('mean').textContent = frame.meanGap.toFixed(3);
  byId('tilt').textContent = frame.tilt.toFixed(3);
  byId('next').textContent = String(frame.suggestedSector + 1);
  byId('direction').textContent = frame.direction.toFixed(1) + '°';
  byId('residual').textContent = frame.residual.toFixed(3) + ' mm';
  byId('risk').textContent = frame.acousticRisk + '/100';
  byId('battery').textContent = (frame.batteryMv / 1000).toFixed(2) + ' V';
  byId('valid').textContent = frame.gaps.filter((_, i) => frame.validMask & (1 << i)).length + '/8';
  byId('sectors').innerHTML = frame.gaps.map((gap, i) =>
    `<div class="sector ${i === frame.suggestedSector ? 'suggested' : ''}"><small>Sector ${i + 1}</small><div><b>${gap.toFixed(3)}</b> mm</div><div class="bar" style="width:${Math.min(100, gap / 3 * 100)}%"></div></div>`).join('');
  byId('headHealth').innerHTML = frame.gaps.map((_, i) =>
    `<span title="Head ${i + 1}">${frame.validMask & (1 << i) ? i + 1 : '!'}</span>`).join('');
  const ok = frame.tilt < 0.7 && frame.residual < 0.4;
  byId('verdict').textContent = ok
    ? 'Instrument criteria met — complete prescribed verification'
    : 'Continue approved cross-pattern; inspect highlighted sector';
  byId('verdict').style.color = ok ? 'var(--accent)' : 'var(--warn)';
}
function renderJobs() {
  const jobs = stored();
  byId('jobList').innerHTML = jobs.length ? jobs.map(j =>
    `<div class="job"><b>${escapeText(j.id)}</b> · ${new Date(j.savedAt).toLocaleString()}<br><small>${escapeText(j.standard)} ${escapeText(j.size)} · ${j.bolts} bolts · mean ${j.frame.meanGap.toFixed(3)} mm · tilt ${j.frame.tilt.toFixed(3)} mm</small></div>`).join('') : '<p>No local records yet.</p>';
}
document.querySelectorAll('nav button').forEach(button => button.addEventListener('click', () => {
  document.querySelectorAll('nav button,.view').forEach(x => x.classList.remove('active'));
  button.classList.add('active');
  byId(button.dataset.view).classList.add('active');
  if (button.dataset.view === 'jobs') renderJobs();
}));
byId('step').addEventListener('click', () => {
  step++;
  frame = FlangeProtocol.simulatedStatus(step);
  renderLive();
});
byId('capture').addEventListener('click', () => {
  const setup = JSON.parse(localStorage.getItem('flangeprint.setup') || '{}');
  const jobs = stored();
  jobs.unshift({...setup, id: setup.id || 'UNASSIGNED', savedAt: new Date().toISOString(), frame});
  saveJobs(jobs.slice(0, 100));
  renderJobs();
  alert('Snapshot stored locally. Instrument criteria are not a safety certification.');
});
byId('setupForm').addEventListener('submit', event => {
  event.preventDefault();
  const setup = {
    id: byId('jobId').value.trim(), operator: byId('operator').value.trim(),
    standard: byId('standard').value, size: byId('size').value.trim(),
    bolts: Number(byId('bolts').value), service: byId('service').value
  };
  localStorage.setItem('flangeprint.setup', JSON.stringify(setup));
  byId('setupResult').textContent = 'Setup saved on this device.';
});
byId('export').addEventListener('click', () => {
  const blob = new Blob([JSON.stringify({author: 'jayis1', product: 'FlangePrint', exportedAt: new Date().toISOString(), jobs: stored()}, null, 2)], {type: 'application/json'});
  const anchor = document.createElement('a');
  anchor.href = URL.createObjectURL(blob);
  anchor.download = 'flangeprint-jobs.json';
  anchor.click();
  URL.revokeObjectURL(anchor.href);
});
byId('clear').addEventListener('click', () => {
  const expected = (JSON.parse(localStorage.getItem('flangeprint.setup') || '{}').id || 'CLEAR');
  if (prompt(`Type ${expected} to clear local records`) === expected) {
    saveJobs([]);
    renderJobs();
  }
});
byId('connect').addEventListener('click', async () => {
  if (!navigator.bluetooth) {
    byId('bleMessage').textContent = 'Web Bluetooth is unavailable. Use USB JSON import or the simulator.';
    return;
  }
  try {
    const device = await navigator.bluetooth.requestDevice({filters: [{namePrefix: 'FlangePrint'}], optionalServices: ['8f2d0001-3a27-4fb8-8c3a-2ebfdf1e0001']});
    byId('connection').textContent = device.name || 'Connected';
    byId('bleMessage').textContent = 'Device selected. Pair using the code under the collar latch.';
  } catch (error) {
    byId('bleMessage').textContent = 'Connection not completed: ' + error.message;
  }
});
renderLive();
