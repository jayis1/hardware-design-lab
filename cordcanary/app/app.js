// CordCanary companion app
// Author: jayis1

(function () {
  const protocol = window.CordCanaryProtocol;
  const tabs = ['Dashboard', 'Incidents', 'Inspector', 'Placement Coach', 'Simulator', 'Settings'];
  let activeTab = 'Dashboard';
  let activeScenario = 'nominal';
  let sensitivity = 55;
  let quietHours = true;
  let wifiSync = true;

  const tabRoot = document.getElementById('tabs');
  const summaryGrid = document.getElementById('summary-grid');
  const screenTitle = document.getElementById('screen-title');
  const screenContent = document.getElementById('screen-content');
  const badgeState = document.getElementById('badge-state');
  const badgeRisk = document.getElementById('badge-risk');

  function getSnapshot() {
    const base = protocol.scenarios[activeScenario];
    const sensitivityBias = (sensitivity - 50) * 0.004;
    const adjustedRisk = Math.max(0.03, Math.min(0.99, base.risk + sensitivityBias));
    return {
      ...base,
      risk: adjustedRisk
    };
  }

  function toneToBadgeClass(risk) {
    const tone = protocol.scoreTone(risk);
    if (tone === 'danger') {
      return 'badge badge-red';
    }
    if (tone === 'warn') {
      return 'badge badge-orange';
    }
    return 'badge badge-green';
  }

  function renderTabs() {
    tabRoot.innerHTML = '';
    tabs.forEach((label) => {
      const button = document.createElement('button');
      button.className = 'tab-button' + (activeTab === label ? ' active' : '');
      button.textContent = label;
      button.addEventListener('click', function () {
        activeTab = label;
        render();
      });
      tabRoot.appendChild(button);
    });
  }

  function renderSummary(snapshot) {
    badgeState.textContent = snapshot.state;
    badgeState.className = toneToBadgeClass(snapshot.risk);
    badgeRisk.textContent = 'Risk ' + snapshot.risk.toFixed(2);
    summaryGrid.innerHTML = '';
    protocol.buildMetrics(snapshot).forEach((metric) => {
      const card = document.createElement('div');
      card.className = 'metric-card';
      const label = document.createElement('div');
      label.className = 'metric-label';
      label.textContent = metric.label;
      const value = document.createElement('div');
      value.className = 'metric-value';
      value.textContent = metric.value;
      card.appendChild(label);
      card.appendChild(value);
      summaryGrid.appendChild(card);
    });
  }

  function setScreen(title, subtitle) {
    screenTitle.innerHTML = '';
    const h2 = document.createElement('h2');
    h2.textContent = title;
    const p = document.createElement('p');
    p.className = 'muted';
    p.textContent = subtitle;
    screenTitle.appendChild(h2);
    screenTitle.appendChild(p);
  }

  function renderDashboard(snapshot) {
    setScreen('Live safety view', snapshot.label);
    const wrap = document.createElement('div');
    wrap.className = 'grid-two';

    const advisory = document.createElement('div');
    advisory.className = 'list-item';
    advisory.innerHTML = '<h3>Advisory</h3><p>' + snapshot.advisory + '</p><small>' + snapshot.cause + '</small>';

    const fleet = document.createElement('div');
    fleet.className = 'list-item';
    fleet.innerHTML = '<h3>Fleet summary</h3><p>1 active device • 0 offline • Sync ' + (wifiSync ? 'enabled' : 'BLE-only') + '</p><small>Quiet hours: ' + (quietHours ? 'On' : 'Off') + '</small>';

    wrap.appendChild(advisory);
    wrap.appendChild(fleet);
    screenContent.innerHTML = '';
    screenContent.appendChild(wrap);
  }

  function renderIncidents(snapshot) {
    setScreen('Incident timeline', 'Recent classified events from the device log');
    const list = document.createElement('div');
    list.className = 'list';
    snapshot.incidents.forEach((event) => {
      const item = document.createElement('div');
      item.className = 'list-item';
      item.innerHTML = '<h4>' + event.time + ' — ' + event.title + '</h4><p>' + event.detail + '</p>';
      list.appendChild(item);
    });
    screenContent.innerHTML = '';
    screenContent.appendChild(list);
  }

  function renderInspector(snapshot) {
    setScreen('Root-cause inspector', 'Why CordCanary classified the condition this way');
    const wrap = document.createElement('div');
    wrap.className = 'grid-two';

    const model = document.createElement('div');
    model.className = 'list-item';
    model.innerHTML = '<h3>Reasoning summary</h3>' +
      '<p><strong>State:</strong> ' + snapshot.state + '</p>' +
      '<p><strong>Primary cause:</strong> ' + snapshot.cause + '</p>' +
      '<p><strong>User action:</strong> ' + snapshot.advisory + '</p>';

    const explanation = document.createElement('div');
    explanation.className = 'list-item';
    explanation.innerHTML = '<h3>Sensor interpretation</h3>' +
      '<p>Risk increases when thermal asymmetry, waveform instability, or strain severity exceed expected load behavior.</p>' +
      '<p>Current scenario hotspot delta: <code class="inline">' + snapshot.hotspotC.toFixed(1) + ' C</code></p>' +
      '<p>Current scenario RMS load: <code class="inline">' + snapshot.currentA.toFixed(2) + ' A</code></p>';

    wrap.appendChild(model);
    wrap.appendChild(explanation);
    screenContent.innerHTML = '';
    screenContent.appendChild(wrap);
  }

  function renderPlacementCoach() {
    setScreen('Placement coach', 'Installation guidance for accurate sensing');
    const list = document.createElement('div');
    list.className = 'list';
    protocol.placementGuide().forEach((tip, index) => {
      const item = document.createElement('div');
      item.className = 'list-item';
      item.innerHTML = '<h4>Tip ' + (index + 1) + '</h4><p>' + tip + '</p>';
      list.appendChild(item);
    });
    screenContent.innerHTML = '';
    screenContent.appendChild(list);
  }

  function renderSimulator(snapshot) {
    setScreen('Scenario simulator', 'Switch between realistic failure cases');
    const wrap = document.createElement('div');
    const controls = document.createElement('div');
    controls.className = 'simulator-controls';

    Object.keys(protocol.scenarios).forEach((key) => {
      const button = document.createElement('button');
      button.className = 'action' + (key === activeScenario ? ' secondary' : '');
      button.textContent = protocol.scenarios[key].label;
      button.addEventListener('click', function () {
        activeScenario = key;
        render();
      });
      controls.appendChild(button);
    });

    const detail = document.createElement('div');
    detail.className = 'list-item';
    detail.innerHTML = '<h3>Selected scenario</h3>' +
      '<p><strong>' + snapshot.label + '</strong></p>' +
      '<p>' + snapshot.advisory + '</p>' +
      '<small>' + snapshot.cause + '</small>';

    wrap.appendChild(controls);
    wrap.appendChild(detail);
    screenContent.innerHTML = '';
    screenContent.appendChild(wrap);
  }

  function renderSettings() {
    setScreen('Settings', 'Alert behavior and sync preferences');
    screenContent.innerHTML = '';

    const settingsPanel = document.createElement('div');
    settingsPanel.className = 'list';

    const sensitivityPanel = document.createElement('div');
    sensitivityPanel.className = 'list-item';
    sensitivityPanel.innerHTML = '<h3>Alert sensitivity</h3><p>Adjust how quickly advisory states escalate.</p>';
    const rangeRow = document.createElement('div');
    rangeRow.className = 'range-row';
    const slider = document.createElement('input');
    slider.type = 'range';
    slider.min = '0';
    slider.max = '100';
    slider.value = String(sensitivity);
    const value = document.createElement('strong');
    value.textContent = sensitivity + '%';
    slider.addEventListener('input', function () {
      sensitivity = Number(slider.value);
      value.textContent = sensitivity + '%';
      render();
    });
    rangeRow.appendChild(slider);
    rangeRow.appendChild(value);
    sensitivityPanel.appendChild(rangeRow);

    const toggles = document.createElement('div');
    toggles.className = 'list-item';
    toggles.innerHTML = '<h3>Preferences</h3>';
    const quietButton = document.createElement('button');
    quietButton.className = 'action';
    quietButton.textContent = 'Quiet Hours: ' + (quietHours ? 'On' : 'Off');
    quietButton.addEventListener('click', function () {
      quietHours = !quietHours;
      render();
    });
    const wifiButton = document.createElement('button');
    wifiButton.className = 'action secondary';
    wifiButton.textContent = 'Wi-Fi Sync: ' + (wifiSync ? 'On' : 'BLE-only');
    wifiButton.addEventListener('click', function () {
      wifiSync = !wifiSync;
      render();
    });
    toggles.appendChild(quietButton);
    toggles.appendChild(document.createTextNode(' '));
    toggles.appendChild(wifiButton);

    settingsPanel.appendChild(sensitivityPanel);
    settingsPanel.appendChild(toggles);
    screenContent.appendChild(settingsPanel);
  }

  function render() {
    const snapshot = getSnapshot();
    renderTabs();
    renderSummary(snapshot);

    if (activeTab === 'Dashboard') {
      renderDashboard(snapshot);
    } else if (activeTab === 'Incidents') {
      renderIncidents(snapshot);
    } else if (activeTab === 'Inspector') {
      renderInspector(snapshot);
    } else if (activeTab === 'Placement Coach') {
      renderPlacementCoach();
    } else if (activeTab === 'Simulator') {
      renderSimulator(snapshot);
    } else {
      renderSettings();
    }
  }

  render();
})();
