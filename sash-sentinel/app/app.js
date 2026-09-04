// sash-sentinel/app/app.js
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.

import { cToF, decodeFrame, demoFrame, gradeRisk, installChecklist, simulateCommand } from './protocol.js';

const state = {
  useMetric: true,
  frame: demoFrame,
};

const input = document.querySelector('#telemetry-input');
const cards = document.querySelector('#risk-cards');
const summary = document.querySelector('#summary');
const action = document.querySelector('#action');
const checklist = document.querySelector('#checklist');
const commandInput = document.querySelector('#command-input');
const commandOutput = document.querySelector('#command-output');

function formatTemp(valueC) {
  if (state.useMetric) {
    return `${valueC.toFixed(1)} °C`;
  }
  return `${cToF(valueC).toFixed(1)} °F`;
}

function renderChecklist() {
  checklist.innerHTML = installChecklist
    .map((item) => `<li>${item}</li>`)
    .join('');
}

function renderCards(frame) {
  const cardDefs = [
    ['Condensation', frame.risk.condensation],
    ['Infiltration', frame.risk.infiltration],
    ['Mold', frame.risk.mold],
    ['Latch Fault', frame.risk.latch_fault],
  ];

  cards.innerHTML = cardDefs
    .map(([label, value]) => {
      const grade = gradeRisk(value);
      return `
        <article class="risk-card ${grade}">
          <div class="label">${label}</div>
          <div class="value">${value.toFixed(1)}</div>
          <div class="label">Glass edge ${formatTemp(frame.env.dew_point_c)} dew point reference</div>
        </article>
      `;
    })
    .join('');
}

function renderNarrative(frame) {
  summary.textContent = `${frame.risk.summary} Cavity humidity is ${frame.env.cavity_humidity_pct.toFixed(1)}% and leak velocity is ${frame.airflow.leak_velocity_mps.toFixed(2)} m/s.`;
  action.textContent = `Recommended action: ${frame.risk.action}`;
}

function renderFrame(frame) {
  state.frame = frame;
  input.value = JSON.stringify(frame, null, 2);
  renderCards(frame);
  renderNarrative(frame);
}

function parseInput() {
  try {
    const frame = decodeFrame(input.value);
    renderFrame(frame);
    commandOutput.textContent = 'Frame parsed successfully.';
  } catch (error) {
    commandOutput.textContent = error.message;
  }
}

function runCommand() {
  commandOutput.textContent = simulateCommand(commandInput.value.trim());
}

document.querySelector('#load-demo').addEventListener('click', () => {
  renderFrame(demoFrame);
  commandOutput.textContent = 'Loaded demo telemetry.';
});

document.querySelector('#parse-frame').addEventListener('click', parseInput);
document.querySelector('#run-command').addEventListener('click', runCommand);
document.querySelector('#toggle-units').addEventListener('click', () => {
  state.useMetric = !state.useMetric;
  renderFrame(state.frame);
});
document.querySelector('#reset-state').addEventListener('click', () => {
  state.useMetric = true;
  renderFrame(demoFrame);
  commandOutput.textContent = 'State reset to demo defaults.';
});
document.querySelector('#copy-command').addEventListener('click', async () => {
  const cmd = 'ping';
  try {
    if (navigator.clipboard && navigator.clipboard.writeText) {
      await navigator.clipboard.writeText(cmd);
      commandOutput.textContent = 'Copied command: ping';
    } else {
      commandOutput.textContent = 'Clipboard API unavailable in this browser.';
    }
  } catch (error) {
    commandOutput.textContent = `Copy failed: ${error.message}`;
  }
});

renderChecklist();
renderFrame(demoFrame);
commandOutput.textContent = 'Ready.';
