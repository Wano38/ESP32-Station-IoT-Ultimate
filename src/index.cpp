#include <Arduino.h>

extern const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <title>Station IoT ESP32</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>

  <style>
    :root {
      --bg: #f4f7fb;
      --card: #ffffff;
      --text: #1f2937;
      --muted: #6b7280;
      --blue: #2563eb;
      --green: #16a34a;
      --orange: #f59e0b;
      --red: #dc2626;
      --purple: #7c3aed;
      --line: #e5e7eb;
      --shadow: 0 8px 24px rgba(31, 41, 55, 0.08);
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      background: var(--bg);
      color: var(--text);
      font-family: Arial, Helvetica, sans-serif;
    }

    header {
      background: linear-gradient(135deg, #2563eb, #60a5fa);
      color: white;
      padding: 22px 18px;
    }

    .wrap {
      max-width: 1320px;
      margin: auto;
    }

    header h1 {
      margin: 0;
      font-size: 30px;
    }

    header p {
      margin: 6px 0 0 0;
      opacity: 0.94;
    }

    nav {
      background: white;
      border-bottom: 1px solid var(--line);
      position: sticky;
      top: 0;
      z-index: 10;
    }

    .tabs {
      max-width: 1320px;
      margin: auto;
      display: flex;
      gap: 8px;
      padding: 10px;
      overflow-x: auto;
    }

    .tabBtn {
      border: 1px solid var(--line);
      background: #f9fafb;
      color: var(--text);
      padding: 10px 14px;
      border-radius: 12px;
      cursor: pointer;
      font-weight: 700;
      white-space: nowrap;
    }

    .tabBtn.active {
      background: var(--blue);
      color: white;
      border-color: var(--blue);
    }

    main {
      max-width: 1320px;
      margin: auto;
      padding: 18px;
    }

    .page { display: none; }
    .page.active { display: block; }

    .grid {
      display: grid;
      grid-template-columns: repeat(4, minmax(0, 1fr));
      gap: 16px;
    }

    .card {
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: 18px;
      box-shadow: var(--shadow);
      padding: 18px;
    }

    .span2 { grid-column: span 2; }
    .span4 { grid-column: span 4; }

    .card h2, .card h3 {
      margin: 0 0 12px 0;
    }

    .label {
      color: var(--muted);
      font-size: 13px;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: .04em;
    }

    .value {
      font-size: 34px;
      font-weight: 800;
      margin-top: 6px;
    }

    .small {
      color: var(--muted);
      font-size: 14px;
      line-height: 1.5;
    }

    .ok { color: var(--green); }
    .warn { color: var(--orange); }
    .danger { color: var(--red); }
    .info { color: var(--blue); }
    .purple { color: var(--purple); }

    .badge {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      border-radius: 999px;
      padding: 7px 10px;
      background: #eef2ff;
      color: #3730a3;
      font-weight: 700;
      margin: 3px;
      font-size: 13px;
    }

    .statusRow {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      margin-top: 12px;
    }

    canvas.chartCanvas {
      width: 100%;
      height: 310px !important;
    }

    button {
      border: none;
      border-radius: 10px;
      padding: 10px 12px;
      margin: 4px;
      font-weight: 700;
      cursor: pointer;
      background: var(--blue);
      color: white;
    }

    button.green { background: var(--green); }
    button.orange { background: var(--orange); color: #111827; }
    button.red { background: var(--red); }
    button.purple { background: var(--purple); }
    button.gray { background: #6b7280; }

    input {
      width: 100%;
      padding: 10px;
      border: 1px solid #d1d5db;
      border-radius: 10px;
      margin: 6px 0 12px 0;
      font-size: 15px;
    }

    table {
      width: 100%;
      border-collapse: collapse;
    }

    td, th {
      text-align: left;
      border-bottom: 1px solid var(--line);
      padding: 9px 6px;
      font-size: 14px;
    }

    td:last-child, th:last-child { text-align: right; }

    .events {
      max-height: 360px;
      overflow-y: auto;
    }

    .event {
      border-left: 4px solid var(--blue);
      background: #f9fafb;
      margin-bottom: 8px;
      padding: 10px;
      border-radius: 10px;
    }

    .event.CRITICAL, .event.DANGER { border-left-color: var(--red); }
    .event.WARNING { border-left-color: var(--orange); }

    .gameLayout {
      display: grid;
      grid-template-columns: 400px 1fr;
      gap: 18px;
      align-items: start;
    }

    #gameCanvas {
      width: 360px;
      height: 360px;
      border: 2px solid #cbd5e1;
      border-radius: 14px;
      background: #f8fafc;
      display: block;
    }

    .gamePad {
      display: grid;
      grid-template-columns: repeat(3, 64px);
      gap: 8px;
      width: 210px;
      margin-top: 12px;
    }

    .gamePad button { margin: 0; }

    .legendDot {
      display: inline-block;
      width: 12px;
      height: 12px;
      border-radius: 50%;
      margin-right: 6px;
    }

    @media (max-width: 920px) {
      .grid { grid-template-columns: 1fr; }
      .span2, .span4 { grid-column: span 1; }
      .gameLayout { grid-template-columns: 1fr; }
      #gameCanvas { width: 100%; max-width: 360px; height: auto; aspect-ratio: 1 / 1; }
    }
  </style>
</head>

<body>
  <header>
    <div class="wrap">
      <h1>Station IoT ESP32</h1>
      <p>Dashboard simple : capteurs, MQTT, base locale, supervision processeur et jeux au joystick HW-504.</p>
      <div class="statusRow">
        <span class="badge">Heure : <span id="clock">--:--:--</span></span>
        <span class="badge">WiFi : <span id="topWifi">--</span></span>
        <span class="badge">MQTT : <span id="topMqtt">--</span></span>
        <span class="badge">Risque : <span id="topRisk">--</span></span>
        <span class="badge">Offline : <span id="topOffline">--</span></span>
      </div>
    </div>
  </header>

  <nav>
    <div class="tabs">
      <button class="tabBtn active" onclick="showPage('dashboard', this)">Dashboard</button>
      <button class="tabBtn" onclick="showPage('graphs', this)">Graphiques</button>
      <button class="tabBtn" onclick="showPage('supervision', this)">Supervision CPU</button>
      <button class="tabBtn" onclick="showPage('offline', this)">BDD / Offline</button>
      <button class="tabBtn" onclick="showPage('games', this)">Jeux joystick</button>
      <button class="tabBtn" onclick="showPage('settings', this)">Réglages</button>
    </div>
  </nav>

  <main>
    <section id="dashboard" class="page active">
      <div class="grid">
        <div class="card"><div class="label">Température</div><div class="value" id="tempValue">-- °C</div><p class="small" id="tempState">--</p></div>
        <div class="card"><div class="label">Humidité</div><div class="value" id="humValue">-- %</div><p class="small" id="humState">--</p></div>
        <div class="card"><div class="label">Gaz MQ</div><div class="value" id="gasValue">--</div><p class="small" id="gasState">--</p></div>
        <div class="card"><div class="label">Risque</div><div class="value" id="riskValue">--</div><p class="small" id="riskState">--</p></div>

        <div class="card span2">
          <h2>État des capteurs</h2>
          <table>
            <tr><td>DHT22</td><td id="dhtState">--</td></tr>
            <tr><td>Capteur gaz</td><td id="gasSensorState">--</td></tr>
            <tr><td>PIR</td><td id="pirState">--</td></tr>
            <tr><td>HW-499</td><td id="hwState">--</td></tr>
            <tr><td>Bouton GPIO25</td><td id="buttonState">--</td></tr>
            <tr><td>Joystick HW-504</td><td id="joyState">--</td></tr>
          </table>
        </div>

        <div class="card span2">
          <h2>Commandes rapides</h2>
          <button class="green" onclick="cmd('auto')">Auto</button>
          <button class="orange" onclick="cmd('manual')">Manuel</button>
          <button class="red" onclick="cmd('alloff')">Tout OFF</button>
          <button class="purple" onclick="cmd('securityOn')">Sécurité ON</button>
          <button class="gray" onclick="cmd('securityOff')">Sécurité OFF</button>
          <button class="orange" onclick="cmd('ack')">Acquitter alarme</button>
          <button onclick="cmd('soundOn')">Son ON</button>
          <button class="gray" onclick="cmd('soundOff')">Son OFF</button>
          <button class="green" onclick="cmd('lightOn')">Lumière ON</button>
          <button class="gray" onclick="cmd('lightOff')">Lumière OFF</button>
        </div>

        <div class="card span4">
          <h2>Mesures en temps réel</h2>
          <canvas id="liveChart" class="chartCanvas"></canvas>
        </div>
      </div>
    </section>

    <section id="graphs" class="page">
      <div class="grid">
        <div class="card span4">
          <h2>Historique local avec ligne de temps</h2>
          <p class="small">Quand le WiFi ou MQTT tombe, les mesures continuent d’être stockées dans LittleFS. Au retour réseau, les points sont recalés sur l’heure du navigateur.</p>
          <button onclick="loadHistory()">Recharger historique mesures</button>
          <button class="gray" onclick="clearHistory()">Vider historique</button>
          <canvas id="historyChart" class="chartCanvas"></canvas>
        </div>
        <div class="card span4">
          <h2>Timeline réseau / offline / risque</h2>
          <canvas id="networkChart" class="chartCanvas"></canvas>
        </div>
      </div>
    </section>

    <section id="supervision" class="page">
      <div class="grid">
        <div class="card span2">
          <h2>Charge estimée des deux cœurs</h2>
          <p class="small">Core 0 : WiFi, MQTT, Web, logs. Core 1 : capteurs, supervision, LEDs et buzzers. La charge est estimée avec le temps actif des tâches FreeRTOS.</p>
          <canvas id="cpuChart" class="chartCanvas"></canvas>
        </div>
        <div class="card span2">
          <h2>Mémoire / Queue / MQTT</h2>
          <canvas id="systemChart" class="chartCanvas"></canvas>
        </div>
        <div class="card span4">
          <h2>Répartition des tâches FreeRTOS</h2>
          <table>
            <thead><tr><th>Tâche</th><th>Cœur</th><th>Priorité</th><th>Boucles</th><th>Temps actif</th></tr></thead>
            <tbody id="taskTable"><tr><td colspan="5">Chargement...</td></tr></tbody>
          </table>
        </div>
      </div>
    </section>

    <section id="offline" class="page">
      <div class="grid">
        <div class="card span2">
          <h2>Base locale / Offline</h2>
          <table>
            <tr><td>LittleFS</td><td id="fsState">--</td></tr>
            <tr><td>Mesures offline</td><td id="offlineCount">--</td></tr>
            <tr><td>Taille offline</td><td id="offlineBytes">--</td></tr>
            <tr><td>Historique mesures</td><td id="historyCount">--</td></tr>
            <tr><td>Latence MQTT</td><td id="latencyValue">--</td></tr>
            <tr><td>Publications MQTT</td><td id="mqttPubValue">--</td></tr>
          </table>
        </div>
        <div class="card span2">
          <h2>Tests panne réseau</h2>
          <p class="small">Pour la démo, préfère “Panne MQTT” : le site reste accessible et tu vois les graphes continuer. “Couper WiFi” rendra le site inaccessible pendant la coupure.</p>
          <button class="orange" onclick="mqttBlackout(5)">Panne MQTT 5 min</button>
          <button class="orange" onclick="mqttBlackout(30)">Panne MQTT 30 min</button>
          <button class="red" onclick="mqttBlackout(60)">Panne MQTT 1 h</button>
          <button class="gray" onclick="cmd('mqttBlackoutOff')">Stop panne MQTT</button><br>
          <button class="orange" onclick="wifiOff(1)">Couper WiFi 1 min</button>
          <button class="red" onclick="wifiOff(30)">Couper WiFi 30 min</button>
          <button class="red" onclick="wifiOff(60)">Couper WiFi 1 h</button>
          <button class="green" onclick="cmd('wifiOn')">WiFi ON</button>
        </div>
        <div class="card span4">
          <h2>Historique événements</h2>
          <button onclick="loadEvents()">Rafraîchir</button>
          <button class="gray" onclick="cmd('clearEvents')">Vider historique</button>
          <div id="events" class="events">Chargement...</div>
        </div>
      </div>
    </section>

    <section id="games" class="page">
      <div class="card">
        <h2>Jeux joystick HW-504</h2>
        <p class="small">Contrôle avec le joystick physique. Clavier possible aussi : flèches + espace. SW du joystick sert à démarrer/rejouer.</p>
        <div class="gameLayout">
          <div>
            <canvas id="gameCanvas" width="360" height="360"></canvas>
            <div class="gamePad">
              <span></span><button onclick="setGameDir('UP')">▲</button><span></span>
              <button onclick="setGameDir('LEFT')">◀</button><button onclick="gameButton()">●</button><button onclick="setGameDir('RIGHT')">▶</button>
              <span></span><button onclick="setGameDir('DOWN')">▼</button><span></span>
            </div>
          </div>
          <div>
            <h3 id="gameTitle">Snake</h3>
            <p class="small" id="gameHelp">Mange les pommes, évite les murs et ton corps.</p>
            <p><b>Score :</b> <span id="gameScore">0</span> — <b>Joystick :</b> <span id="gameJoy">CENTER</span></p>
            <button class="green" onclick="selectGame('snake')">Snake</button>
            <button onclick="selectGame('collect')">Collecteur</button>
            <button class="purple" onclick="selectGame('dodge')">Dodge</button>
            <button class="orange" onclick="restartGame()">Rejouer</button>
            <hr>
            <p class="small"><b>Branchement HW-504 :</b> VCC→3V3, GND→GND, VRX→GPIO32, VRY→GPIO35, SW→GPIO23.</p>
          </div>
        </div>
      </div>
    </section>

    <section id="settings" class="page">
      <div class="grid">
        <div class="card span2">
          <h2>Seuils</h2>
          <label>Seuil température haute (°C)</label><input id="tempLimit" type="number" step="0.1" value="35">
          <label>Humidité basse (%)</label><input id="humLow" type="number" step="0.1" value="30">
          <label>Humidité haute (%)</label><input id="humHigh" type="number" step="0.1" value="75">
          <label>Seuil gaz brut ADC</label><input id="gasLimit" type="number" step="1" value="2500">
          <button class="green" onclick="saveConfig(false)">Sauver seuils</button>
          <button class="orange" onclick="cmd('calibrateGas')">Calibrer gaz</button>
        </div>
        <div class="card span2">
          <h2>MQTT</h2>
          <label>Topic MQTT base</label><input id="mqttBase" type="text" value="campus/groupe1/ESP32-Othmane">
          <button class="green" onclick="saveConfig(true)">Sauver + MQTT ON</button>
          <button class="gray" onclick="saveConfig(false)">MQTT OFF</button>
          <p class="small">Dans Node-RED, écoute le topic <b>campus/groupe1/ESP32-Othmane/data</b>.</p>
        </div>
      </div>
    </section>
  </main>

<script>
const TOKEN = '1234';
const MAX_POINTS = 45;
let lastData = null;
let prevPerf = null;
let game = 'snake';
let gameDir = 'RIGHT';
let nextGameDir = 'RIGHT';
let joystickDirection = 'CENTER';
let joystickButton = false;
let previousJoystickButton = false;
let gameScore = 0;
let gameRunning = true;
let snake, apple, collectPlayer, collectTarget, dodgePlayer, enemies;

function api(path, token=false) {
  if (!token) return path + (path.includes('?') ? '&' : '?') + 'nocache=' + Date.now();
  return path + (path.includes('?') ? '&' : '?') + 'token=' + TOKEN + '&nocache=' + Date.now();
}

function showPage(id, btn) {
  document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
  document.querySelectorAll('.tabBtn').forEach(b => b.classList.remove('active'));
  document.getElementById(id).classList.add('active');
  btn.classList.add('active');
}

function setText(id, value, cls) {
  const el = document.getElementById(id);
  if (!el) return;
  el.textContent = value;
  if (cls !== undefined) el.className = cls;
}

function timeLabel(date = new Date()) {
  return date.toLocaleTimeString('fr-FR', {hour:'2-digit', minute:'2-digit', second:'2-digit'});
}

function estimateTimeLabel(createdMs, currentUptimeSec) {
  if (!lastData) return '';
  const currentUptimeMs = currentUptimeSec * 1000;
  const age = Math.max(0, currentUptimeMs - createdMs);
  return timeLabel(new Date(Date.now() - age));
}

function pushLimited(arr, value) {
  arr.push(value);
  if (arr.length > MAX_POINTS) arr.shift();
}

let liveLabels=[], tempData=[], humData=[], gasData=[];
let networkLabels=[], wifiData=[], mqttData=[], offlineData=[], riskData=[];
let cpuLabels=[], core0Data=[], core1Data=[];
let sysLabels=[], heapData=[], queueData=[], latencyData=[];
let liveChart, networkChart, historyChart, cpuChart, systemChart;

function makeLineChart(id, labels, datasets) {
  return new Chart(document.getElementById(id), {
    type: 'line',
    data: { labels, datasets },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      animation: false,
      plugins: { legend: { position: 'bottom' } },
      scales: { y: { beginAtZero: true }, x: { ticks: { maxRotation: 0, autoSkip: true, maxTicksLimit: 8 } } }
    }
  });
}

function initCharts() {
  if (typeof Chart === 'undefined') return;
  liveChart = makeLineChart('liveChart', liveLabels, [
    {label:'Température °C', data:tempData, borderColor:'#2563eb', backgroundColor:'rgba(37,99,235,.08)', tension:.25},
    {label:'Humidité %', data:humData, borderColor:'#16a34a', backgroundColor:'rgba(22,163,74,.08)', tension:.25},
    {label:'Gaz %', data:gasData, borderColor:'#f59e0b', backgroundColor:'rgba(245,158,11,.08)', tension:.25}
  ]);
  networkChart = makeLineChart('networkChart', networkLabels, [
    {label:'WiFi OK', data:wifiData, borderColor:'#16a34a', tension:.15, stepped:true},
    {label:'MQTT OK', data:mqttData, borderColor:'#2563eb', tension:.15, stepped:true},
    {label:'Offline actif', data:offlineData, borderColor:'#dc2626', tension:.15, stepped:true},
    {label:'Risque /100', data:riskData, borderColor:'#7c3aed', tension:.25}
  ]);
  historyChart = makeLineChart('historyChart', [], [
    {label:'Température °C', data:[], borderColor:'#2563eb', tension:.25},
    {label:'Humidité %', data:[], borderColor:'#16a34a', tension:.25},
    {label:'Gaz %', data:[], borderColor:'#f59e0b', tension:.25}
  ]);
  cpuChart = makeLineChart('cpuChart', cpuLabels, [
    {label:'Core 0 %', data:core0Data, borderColor:'#2563eb', backgroundColor:'rgba(37,99,235,.08)', tension:.25},
    {label:'Core 1 %', data:core1Data, borderColor:'#dc2626', backgroundColor:'rgba(220,38,38,.08)', tension:.25}
  ]);
  systemChart = makeLineChart('systemChart', sysLabels, [
    {label:'Heap libre KB', data:heapData, borderColor:'#16a34a', tension:.25},
    {label:'Queue', data:queueData, borderColor:'#f59e0b', tension:.25, stepped:true},
    {label:'Latence MQTT ms', data:latencyData, borderColor:'#7c3aed', tension:.25}
  ]);
}

async function updateData() {
  try {
    const res = await fetch(api('/api/sensors'));
    const d = await res.json();
    lastData = d;
    const now = timeLabel();

    setText('clock', now);
    setText('topWifi', d.wifi ? 'OK' : 'OFF', d.wifi ? 'ok' : 'danger');
    setText('topMqtt', d.mqttConnected ? 'OK' : (d.mqttEnabled ? 'ACTIF' : 'OFF'), d.mqttConnected ? 'ok' : 'warn');
    setText('topRisk', d.riskScore + ' / ' + d.riskState, d.riskScore >= 55 ? 'danger' : d.riskScore >= 30 ? 'warn' : 'ok');
    setText('topOffline', d.offlineFileCount, d.offlineFileCount > 0 ? 'warn' : 'ok');

    setText('tempValue', d.temp === null ? 'Erreur' : d.temp.toFixed(1) + ' °C', d.tempAlert ? 'value danger' : 'value');
    setText('humValue', d.humidity === null ? 'Erreur' : d.humidity.toFixed(1) + ' %', d.humidityAlert ? 'value warn' : 'value');
    setText('gasValue', d.gasRaw, d.gasAlert ? 'value danger' : 'value');
    setText('riskValue', d.riskScore + '/100', d.riskScore >= 55 ? 'value danger' : d.riskScore >= 30 ? 'value warn' : 'value ok');
    setText('riskState', d.riskState);
    setText('tempState', d.tempAlert ? 'Alerte température' : 'Température normale', d.tempAlert ? 'small danger' : 'small ok');
    setText('humState', d.humidityAlert ? 'Humidité hors limites' : 'Humidité normale', d.humidityAlert ? 'small warn' : 'small ok');
    setText('gasState', d.gasAlert ? 'Alerte gaz' : 'Gaz normal - ' + d.gasPercent.toFixed(1) + '%', d.gasAlert ? 'small danger' : 'small ok');

    setText('dhtState', d.dhtOk ? 'OK' : 'PANNE', d.dhtOk ? 'ok' : 'danger');
    setText('gasSensorState', d.gasSensorOk ? 'OK' : 'SUSPECT', d.gasSensorOk ? 'ok' : 'danger');
    setText('pirState', d.pir ? 'MOUVEMENT' : 'RAS', d.pir ? 'danger' : 'ok');
    setText('hwState', d.hw499 ? 'ACTIF' : 'RAS', d.hw499 ? 'warn' : 'ok');
    setText('buttonState', d.button ? 'APPUYÉ' : 'RAS', d.button ? 'warn' : 'ok');
    setText('joyState', d.joyDirection + ' X=' + d.joyX + ' Y=' + d.joyY, d.joyButton ? 'warn' : 'info');

    setText('fsState', d.littleFsOk ? 'OK' : 'ERREUR', d.littleFsOk ? 'ok' : 'danger');
    setText('offlineCount', d.offlineFileCount);
    setText('offlineBytes', Math.round(d.offlineFileBytes / 1024) + ' KB');
    setText('historyCount', d.historyFileCount);
    setText('latencyValue', d.lastPublishLatency + ' ms');
    setText('mqttPubValue', d.mqttPublished + ' OK / ' + d.mqttFailed + ' erreurs');

    if (liveChart && d.temp !== null && d.humidity !== null) {
      pushLimited(liveLabels, now);
      pushLimited(tempData, d.temp);
      pushLimited(humData, d.humidity);
      pushLimited(gasData, d.gasPercent);
      liveChart.update();

      pushLimited(networkLabels, now);
      pushLimited(wifiData, d.wifi ? 100 : 0);
      pushLimited(mqttData, d.mqttConnected ? 100 : 0);
      pushLimited(offlineData, (d.offlineFileCount > 0 || d.networkForcedOff || d.mqttBlackoutActive) ? 100 : 0);
      pushLimited(riskData, d.riskScore);
      networkChart.update();
    }
  } catch(e) {
    setText('topWifi', 'API erreur', 'danger');
  }
  setTimeout(updateData, 1000);
}

async function updatePerformance() {
  try {
    const res = await fetch(api('/api/performance'));
    const p = await res.json();
    const nowClient = Date.now();
    const label = timeLabel();

    let core0 = 0, core1 = 0;
    if (prevPerf) {
      const elapsedUs = Math.max(1, (nowClient - prevPerf.clientMs) * 1000);
      p.tasks.forEach((t, i) => {
        const previous = prevPerf.tasks[i] || {runtimeUs:t.runtimeUs};
        let delta = t.runtimeUs - previous.runtimeUs;
        if (delta < 0) delta = 0;
        if (t.core === 0) core0 += delta;
        else core1 += delta;
      });
      core0 = Math.min(100, Math.round(core0 * 100 / elapsedUs));
      core1 = Math.min(100, Math.round(core1 * 100 / elapsedUs));

      pushLimited(cpuLabels, label);
      pushLimited(core0Data, core0);
      pushLimited(core1Data, core1);
      if (cpuChart) cpuChart.update();

      pushLimited(sysLabels, label);
      pushLimited(heapData, Math.round(p.heap / 1024));
      pushLimited(queueData, p.queueWaiting);
      pushLimited(latencyData, p.lastPublishLatency);
      if (systemChart) systemChart.update();
    }

    prevPerf = Object.assign({clientMs: nowClient}, p);
    const body = document.getElementById('taskTable');
    body.innerHTML = p.tasks.map(t => `<tr><td>${t.name}</td><td>Core ${t.core}</td><td>${t.priority}</td><td>${t.loops}</td><td>${Math.round(t.runtimeUs/1000)} ms</td></tr>`).join('');
  } catch(e) {}
  setTimeout(updatePerformance, 1000);
}

async function loadHistory() {
  try {
    const res = await fetch(api('/api/history?limit=120'));
    const rows = await res.json();
    const labels = rows.map(r => estimateTimeLabel(r.createdMs, lastData ? lastData.uptime : 0));
    historyChart.data.labels = labels;
    historyChart.data.datasets[0].data = rows.map(r => r.temp);
    historyChart.data.datasets[1].data = rows.map(r => r.humidity);
    historyChart.data.datasets[2].data = rows.map(r => r.gasPercent);
    historyChart.update();
  } catch(e) {}
}

async function loadEvents() {
  try {
    const res = await fetch(api('/api/events'));
    const rows = await res.json();
    const box = document.getElementById('events');
    box.innerHTML = rows.length ? rows.map(e => `<div class="event ${e.severity}"><b>${e.type}</b> <span class="small">t+${e.ts}s — ${e.severity}</span><br>${e.message}</div>`).join('') : '<p class="small">Aucun événement.</p>';
  } catch(e) {}
}

async function loadConfig() {
  try {
    const r = await fetch(api('/api/config'));
    const c = await r.json();
    document.getElementById('tempLimit').value = c.tempLimit;
    document.getElementById('humLow').value = c.humLow;
    document.getElementById('humHigh').value = c.humHigh;
    document.getElementById('gasLimit').value = c.gasLimit;
    document.getElementById('mqttBase').value = c.mqttBase;
  } catch(e) {}
}

async function saveConfig(enableMqtt) {
  const url = '/api/config?tempLimit=' + document.getElementById('tempLimit').value +
    '&humLow=' + document.getElementById('humLow').value +
    '&humHigh=' + document.getElementById('humHigh').value +
    '&gasLimit=' + document.getElementById('gasLimit').value +
    '&mqtt=' + (enableMqtt ? 1 : 0) +
    '&mqttBase=' + encodeURIComponent(document.getElementById('mqttBase').value);
  await fetch(api(url, true));
  loadConfig();
}

async function cmd(name) {
  await fetch(api('/api/control?cmd=' + name, true));
  updateData();
  loadEvents();
}

async function mqttBlackout(minutes) {
  await fetch(api('/api/control?cmd=mqttBlackout&minutes=' + minutes, true));
}

async function wifiOff(minutes) {
  await fetch(api('/api/control?cmd=wifiOff&minutes=' + minutes, true));
}

async function clearHistory() {
  await fetch(api('/api/history/clear', true));
  loadHistory();
}

function speakData() {
  if (!lastData) return;
  const msg = new SpeechSynthesisUtterance(`Station IoT. Température ${Math.round(lastData.temp)} degrés. Humidité ${Math.round(lastData.humidity)} pour cent. Risque ${lastData.riskScore} sur 100.`);
  msg.lang = 'fr-FR';
  speechSynthesis.speak(msg);
}

async function pollJoystick() {
  try {
    const r = await fetch(api('/api/joystick'));
    const j = await r.json();
    joystickDirection = j.direction;
    joystickButton = j.button;
    setText('gameJoy', joystickDirection + (joystickButton ? ' + SW' : ''));
    if (joystickDirection !== 'CENTER') setGameDir(joystickDirection);
    if (joystickButton && !previousJoystickButton) gameButton();
    previousJoystickButton = joystickButton;
  } catch(e) {}
  setTimeout(pollJoystick, 90);
}

const canvas = () => document.getElementById('gameCanvas');
const ctx = () => canvas().getContext('2d');
const cell = 18;

function selectGame(g) {
  game = g;
  restartGame();
}

function setGameDir(d) {
  if (game === 'snake') {
    if ((gameDir === 'LEFT' && d === 'RIGHT') || (gameDir === 'RIGHT' && d === 'LEFT') || (gameDir === 'UP' && d === 'DOWN') || (gameDir === 'DOWN' && d === 'UP')) return;
  }
  nextGameDir = d;
}

function gameButton() { restartGame(); }

function restartGame() {
  gameScore = 0;
  gameRunning = true;
  gameDir = 'RIGHT';
  nextGameDir = 'RIGHT';
  snake = [{x:5,y:10},{x:4,y:10},{x:3,y:10}];
  apple = randomCell();
  collectPlayer = {x:10,y:10};
  collectTarget = randomCell();
  dodgePlayer = {x:10,y:17};
  enemies = [{x:3,y:0},{x:10,y:5},{x:17,y:2}];
  document.getElementById('gameScore').textContent = gameScore;
  document.getElementById('gameTitle').textContent = game === 'snake' ? 'Snake' : game === 'collect' ? 'Collecteur' : 'Dodge';
  document.getElementById('gameHelp').textContent = game === 'snake' ? 'Mange les pommes, évite les murs et ton corps.' : game === 'collect' ? 'Attrape le carré vert le plus vite possible.' : 'Évite les blocs rouges qui descendent.';
}

function randomCell() { return {x:Math.floor(Math.random()*20), y:Math.floor(Math.random()*20)}; }

function moveByDir(pos) {
  if (nextGameDir === 'LEFT') pos.x--;
  if (nextGameDir === 'RIGHT') pos.x++;
  if (nextGameDir === 'UP') pos.y--;
  if (nextGameDir === 'DOWN') pos.y++;
}

function drawGrid() {
  const c = ctx();
  c.clearRect(0,0,360,360);
  c.fillStyle = '#f8fafc'; c.fillRect(0,0,360,360);
  c.strokeStyle = '#e5e7eb';
  for (let i=0;i<=20;i++){ c.beginPath(); c.moveTo(i*cell,0); c.lineTo(i*cell,360); c.stroke(); c.beginPath(); c.moveTo(0,i*cell); c.lineTo(360,i*cell); c.stroke(); }
}

function drawCell(p, color) {
  const c = ctx(); c.fillStyle = color; c.fillRect(p.x*cell+2, p.y*cell+2, cell-4, cell-4);
}

function gameLoop() {
  drawGrid();
  if (!gameRunning) {
    ctx().fillStyle = '#111827'; ctx().font='22px Arial'; ctx().fillText('Game over - appuie SW', 70, 180);
    setTimeout(gameLoop, 140); return;
  }
  if (game === 'snake') updateSnake();
  else if (game === 'collect') updateCollect();
  else updateDodge();
  document.getElementById('gameScore').textContent = gameScore;
  setTimeout(gameLoop, game === 'snake' ? 140 : 110);
}

function updateSnake() {
  gameDir = nextGameDir;
  const head = {x:snake[0].x, y:snake[0].y};
  moveByDir(head);
  if (head.x<0 || head.y<0 || head.x>=20 || head.y>=20 || snake.some(s=>s.x===head.x && s.y===head.y)) { gameRunning=false; return; }
  snake.unshift(head);
  if (head.x===apple.x && head.y===apple.y) { gameScore++; apple=randomCell(); }
  else snake.pop();
  snake.forEach((s,i)=>drawCell(s, i===0 ? '#2563eb' : '#60a5fa'));
  drawCell(apple, '#dc2626');
}

function updateCollect() {
  moveByDir(collectPlayer);
  collectPlayer.x = Math.max(0, Math.min(19, collectPlayer.x));
  collectPlayer.y = Math.max(0, Math.min(19, collectPlayer.y));
  if (collectPlayer.x===collectTarget.x && collectPlayer.y===collectTarget.y) { gameScore++; collectTarget=randomCell(); }
  drawCell(collectPlayer, '#2563eb');
  drawCell(collectTarget, '#16a34a');
}

function updateDodge() {
  if (nextGameDir === 'LEFT') dodgePlayer.x--;
  if (nextGameDir === 'RIGHT') dodgePlayer.x++;
  dodgePlayer.x = Math.max(0, Math.min(19, dodgePlayer.x));
  enemies.forEach(e => { e.y++; if (e.y>19) { e.y=0; e.x=Math.floor(Math.random()*20); gameScore++; } if (e.x===dodgePlayer.x && e.y===dodgePlayer.y) gameRunning=false; });
  drawCell(dodgePlayer, '#2563eb');
  enemies.forEach(e=>drawCell(e, '#dc2626'));
}

document.addEventListener('keydown', e => {
  if (e.key === 'ArrowUp') setGameDir('UP');
  if (e.key === 'ArrowDown') setGameDir('DOWN');
  if (e.key === 'ArrowLeft') setGameDir('LEFT');
  if (e.key === 'ArrowRight') setGameDir('RIGHT');
  if (e.key === ' ') gameButton();
});

window.addEventListener('load', () => {
  initCharts();
  restartGame();
  gameLoop();
  updateData();
  updatePerformance();
  pollJoystick();
  loadConfig();
  loadEvents();
  setTimeout(loadHistory, 2000);
  setInterval(loadEvents, 5000);
  setInterval(()=>setText('clock', timeLabel()), 1000);
});
</script>
</body>
</html>
)rawliteral";
