// Pantry Warden companion controller
// Author: jayis1

import { buildFrame, classifyZones, deriveAction, telemetryInterpretation } from './protocol.js';

const screens = Array.from(document.querySelectorAll('.screen'));
const tabs = Array.from(document.querySelectorAll('.tab'));
const metricGrid = document.getElementById('metricGrid');
const actionText = document.getElementById('actionText');
const healthBar = document.getElementById('healthBar');
const zoneGrid = document.getElementById('zoneGrid');
const interpretationList = document.getElementById('interpretationList');
const historyList = document.getElementById('historyList');
const frameDump = document.getElementById('frameDump');
const refreshButton = document.getElementById('refreshButton');
const applyScenarioButton = document.getElementById('applyScenarioButton');

const history = [];
let tick = 0;
let currentFrame = buildFrame();

function metricTemplate(label, value, suffix = '') {
  return `<article class="metric"><h3>${label}</h3><strong>${value}${suffix}</strong></article>`;
}

function renderMetrics(frame) {
  metricGrid.innerHTML = [
    metricTemplate('Shelf state', frame.state),
    metricTemplate('Shelf mass', frame.mass.toFixed(2), ' kg'),
    metricTemplate('VOC index', frame.voc.toFixed(1)),
    metricTemplate('Moisture strip', frame.moist.toFixed(1), '%'),
    metricTemplate('Wingbeat', frame.wing.toFixed(1)),
    metricTemplate('Battery', frame.bat.toFixed(1), '%')
  ].join('');
}

function renderZones(frame) {
  zoneGrid.innerHTML = classifyZones(frame).map((zone) => `
    <article class="zone ${zone.severity}">
      <h3>${zone.name}</h3>
      <p>Mass: ${zone.mass} kg</p>
      <p>Freshness proxy: ${zone.freshness}%</p>
      <p>Status: ${zone.severity}</p>
    </article>
  `).join('');
}

function renderInterpretations(frame) {
  interpretationList.innerHTML = telemetryInterpretation(frame)
    .map((line) => `<li>${line}</li>`)
    .join('');
}

function renderHistory() {
  historyList.innerHTML = history.slice().reverse().map((item) => `<li><strong>Tick ${item.tick}</strong> · ${item.state}<br>${item.action}</li>`).join('');
}

function setFrame(frame) {
  currentFrame = frame;
  renderMetrics(frame);
  renderZones(frame);
  renderInterpretations(frame);
  actionText.textContent = deriveAction(frame);
  healthBar.style.width = `${frame.health}%`;
  frameDump.textContent = JSON.stringify(frame, null, 2);
  history.push({ tick: frame.tick, state: frame.state, action: deriveAction(frame) });
  if (history.length > 12) {
    history.shift();
  }
  renderHistory();
}

function nextFrame() {
  tick += 1;
  const frame = buildFrame({
    tick,
    voc: 14 + (tick % 6) * 4 + (tick > 5 ? 4 : 0),
    moist: 16 + (tick % 5) * 6,
    wing: tick > 7 ? 18 + (tick - 7) * 5 : 10 + tick,
    chew: tick > 8 ? 14 + (tick - 8) * 4 : 8 + tick,
    mass: 6.1 + (tick > 3 ? 0.8 : 0) + tick * 0.04,
    gap: 68 - (tick > 4 ? tick * 1.4 : tick * 0.5),
    bat: Math.max(70, 94 - tick * 1.1),
    health: Math.max(35, 91 - tick * 4.2)
  });
  setFrame(frame);
}

tabs.forEach((tab) => {
  tab.addEventListener('click', () => {
    tabs.forEach((node) => node.classList.remove('active'));
    screens.forEach((screen) => screen.classList.remove('active'));
    tab.classList.add('active');
    document.getElementById(tab.dataset.screen).classList.add('active');
  });
});

refreshButton.addEventListener('click', nextFrame);

applyScenarioButton.addEventListener('click', () => {
  const voc = Number(document.getElementById('vocInput').value);
  const moist = Number(document.getElementById('moistInput').value);
  const wing = Number(document.getElementById('wingInput').value);
  const mass = Number(document.getElementById('massInput').value);
  const frame = buildFrame({
    tick: tick + 1,
    voc,
    moist,
    wing,
    chew: Math.max(8, wing * 0.76),
    mass,
    gap: 70 - (voc * 0.32),
    health: 92 - (voc * 0.7) - (moist * 0.35) - (wing * 0.25)
  });
  setFrame(frame);
});

setFrame(currentFrame);
