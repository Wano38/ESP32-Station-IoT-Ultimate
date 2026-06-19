#include <Arduino.h>

extern const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <title>Station IoT ESP32</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <style>
    :root{
      --bg:#f6f7fb;
      --card:#ffffff;
      --text:#1f2937;
      --muted:#667085;
      --line:#d9e2ef;
      --blue:#0b4ea2;
      --green:#198754;
      --orange:#f59e0b;
      --red:#dc3545;
      --purple:#7c3aed;
    }

    *{box-sizing:border-box}

    body{
      margin:0;
      font-family:Arial, Helvetica, sans-serif;
      background:var(--bg);
      color:var(--text);
    }

    header{
      background:white;
      border-bottom:1px solid var(--line);
      padding:16px 20px;
    }

    header h1{
      margin:0;
      font-size:24px;
      color:var(--blue);
    }

    header p{
      margin:6px 0 0;
      color:var(--muted);
      font-size:14px;
    }

    main{
      width:min(1220px,calc(100% - 22px));
      margin:14px auto;
    }

    .status{
      display:flex;
      gap:8px;
      flex-wrap:wrap;
      margin-bottom:12px;
    }

    .pill{
      background:white;
      border:1px solid var(--line);
      border-radius:999px;
      padding:8px 12px;
      font-size:13px;
      font-weight:800;
    }

    .ok{color:var(--green)!important}
    .warn{color:var(--orange)!important}
    .bad{color:var(--red)!important}
    .info{color:var(--blue)!important}

    .tabs{
      display:flex;
      gap:8px;
      flex-wrap:wrap;
      margin:12px 0 14px;
      position:sticky;
      top:0;
      z-index:5;
      background:var(--bg);
      padding:8px 0;
    }

    button{
      border:0;
      border-radius:8px;
      padding:9px 11px;
      background:var(--blue);
      color:white;
      font-weight:800;
      cursor:pointer;
    }

    button.secondary{
      background:#eef3fb;
      color:var(--text);
      border:1px solid var(--line);
    }

    button.green{background:var(--green)}
    button.orange{background:var(--orange)}
    button.red{background:var(--red)}
    button.purple{background:var(--purple)}

    .tabs button{
      background:white;
      color:var(--text);
      border:1px solid var(--line);
    }

    .tabs button.active{
      background:var(--blue);
      color:white;
      border-color:var(--blue);
    }

    section{display:none}
    section.active{display:block}

    .grid{
      display:grid;
      grid-template-columns:repeat(4,minmax(0,1fr));
      gap:12px;
    }

    .card{
      background:white;
      border:1px solid var(--line);
      border-radius:12px;
      padding:14px;
      box-shadow:0 3px 12px rgba(15,23,42,.04);
    }

    .span2{grid-column:span 2}
    .span3{grid-column:span 3}
    .span4{grid-column:span 4}

    .title{
      font-size:12px;
      text-transform:uppercase;
      color:var(--muted);
      font-weight:900;
      margin-bottom:8px;
    }

    .value{
      font-size:30px;
      font-weight:900;
      color:var(--blue);
      line-height:1.1;
    }

    .sub{
      font-size:13px;
      color:var(--muted);
      margin-top:6px;
    }

    table{
      width:100%;
      border-collapse:collapse;
    }

    td{
      padding:7px 0;
      border-bottom:1px solid #edf1f7;
      color:var(--muted);
      font-size:14px;
    }

    td:last-child{
      color:var(--text);
      text-align:right;
      font-weight:900;
    }

    .controls{
      display:flex;
      gap:8px;
      flex-wrap:wrap;
      margin-top:10px;
    }

    input{
      width:100%;
      border:1px solid var(--line);
      border-radius:8px;
      padding:9px;
      margin:5px 0 10px;
    }

    label{
      color:var(--muted);
      font-size:13px;
      font-weight:900;
    }

    .graphGrid{
      display:grid;
      grid-template-columns:repeat(2,minmax(0,1fr));
      gap:12px;
    }

    canvas.graph{
      width:100%;
      height:250px;
      border:1px solid var(--line);
      border-radius:10px;
      background:white;
      display:block;
    }

    #radarCanvas{
      width:100%;
      height:350px;
      border:1px solid var(--line);
      border-radius:10px;
      background:#f8fbff;
      display:block;
    }

    .events{
      max-height:320px;
      overflow:auto;
      padding-right:4px;
    }

    .event{
      background:#f8fbff;
      border-left:4px solid var(--blue);
      border-radius:8px;
      padding:8px 9px;
      margin-bottom:7px;
      font-size:14px;
    }

    .event.WARNING{border-left-color:var(--orange)}
    .event.CRITIQUE,.event.CRITICAL{border-left-color:var(--red)}

    .eventTop{
      display:flex;
      justify-content:space-between;
      gap:10px;
      color:var(--muted);
      font-size:12px;
      font-weight:900;
      margin-bottom:4px;
    }

    .gameCanvas{
      width:100%;
      max-width:430px;
      height:330px;
      border:2px solid var(--blue);
      border-radius:12px;
      background:#f8fbff;
      display:block;
      margin:0 auto;
    }

    footer{
      width:min(1220px,calc(100% - 22px));
      margin:20px auto;
      color:var(--muted);
      font-size:13px;
      text-align:center;
    }

    @media(max-width:900px){
      .grid,.graphGrid{grid-template-columns:1fr}
      .span2,.span3,.span4{grid-column:span 1}
      header h1{font-size:21px}
    }
  </style>
</head>

<body>
<header>
  <h1>Station IoT ESP32</h1>
  <p>Version finale simple — DHT22, Gaz MQ, Radar HC-SR04, HW-499, joystick, MQTT, FreeRTOS, offline/replay</p>
</header>

<main>
  <div class="status">
    <span class="pill" id="clock">--:--:--</span>
    <span class="pill" id="wifiPill">WiFi --</span>
    <span class="pill" id="mqttPill">MQTT --</span>
    <span class="pill" id="securityPill">Sécurité --</span>
    <span class="pill" id="radarPill">Radar --</span>
    <span class="pill" id="riskPill">Risque --</span>
  </div>

  <div class="tabs">
    <button class="active" data-tab="dash">Dashboard</button>
    <button data-tab="radar">Radar</button>
    <button data-tab="graphs">Graphiques</button>
    <button data-tab="commands">Commandes</button>
    <button data-tab="games">Jeux</button>
    <button data-tab="system">Système</button>
    <button data-tab="settings">Réglages</button>
  </div>

  <section id="dash" class="active">
    <div class="grid">
      <div class="card">
        <div class="title">Température</div>
        <div class="value" id="tempVal">--</div>
        <div class="sub" id="dhtState">--</div>
      </div>

      <div class="card">
        <div class="title">Humidité</div>
        <div class="value" id="humVal">--</div>
        <div class="sub">DHT22</div>
      </div>

      <div class="card">
        <div class="title">Gaz MQ</div>
        <div class="value" id="gasVal">--</div>
        <div class="sub" id="gasState">--</div>
      </div>

      <div class="card">
        <div class="title">Distance radar</div>
        <div class="value" id="distVal">--</div>
        <div class="sub" id="distState">--</div>
      </div>

      <div class="card span2">
        <div class="title">État sécurité</div>
        <table>
          <tr><td>Objet radar</td><td id="objVal">--</td></tr>
          <tr><td>Sécurité</td><td id="secVal">--</td></tr>
          <tr><td>Intrusion</td><td id="intrVal">--</td></tr>
          <tr><td>Détections radar</td><td id="radarCountVal">--</td></tr>
          <tr><td>HW-499</td><td id="hwVal">--</td></tr>
          <tr><td>Joystick</td><td id="joyVal">--</td></tr>
          <tr><td>Mélodie</td><td id="melodyDemoVal">--</td></tr>
          <tr><td>Interruption critique</td><td id="criticalVal">--</td></tr>
        </table>
      </div>

      <div class="card span2">
        <div class="title">Historique événements</div>
        <div class="events" id="eventsList">Chargement...</div>
      </div>
    </div>
  </section>

  <section id="radar">
    <div class="grid">
      <div class="card span3">
        <div class="title">Radar HC-SR04 — échos récents</div>
        <canvas id="radarCanvas" width="800" height="380"></canvas>
        <div class="sub">
          Le HC-SR04 fixe renvoie une distance principale. Le mode multi-objets ci-dessous affiche les derniers échos détectés pour visualiser plusieurs obstacles récents.
        </div>
      </div>

      <div class="card">
        <div class="title">Détails radar</div>
        <table>
          <tr><td>Distance actuelle</td><td id="radarDist">--</td></tr>
          <tr><td>Seuil détection</td><td id="radarLimitText">--</td></tr>
          <tr><td>Distance minimale</td><td id="distanceMinVal">--</td></tr>
          <tr><td>Échos récents</td><td id="radarEchoCount">0</td></tr>
          <tr><td>État</td><td id="radarStatus">--</td></tr>
        </table>
        <div class="controls">
          <button class="secondary" id="clearEchoesBtn">Effacer échos</button>
          <button class="red" data-action="securityOn">Sécurité ON</button>
          <button class="secondary" data-action="securityOff">Sécurité OFF</button>
          <button class="orange" data-action="ack">Acquitter</button>
        </div>
      </div>
    </div>
  </section>

  <section id="graphs">
    <div class="graphGrid">
      <div class="card"><div class="title">Température / heure</div><canvas class="graph" id="chartTemp" width="560" height="280"></canvas></div>
      <div class="card"><div class="title">Humidité / heure</div><canvas class="graph" id="chartHum" width="560" height="280"></canvas></div>
      <div class="card"><div class="title">Gaz / heure</div><canvas class="graph" id="chartGas" width="560" height="280"></canvas></div>
      <div class="card"><div class="title">Distance / heure</div><canvas class="graph" id="chartDistance" width="560" height="280"></canvas></div>
      <div class="card"><div class="title">Risque / heure</div><canvas class="graph" id="chartRisk" width="560" height="280"></canvas></div>
      <div class="card"><div class="title">CPU / heure</div><canvas class="graph" id="chartCpu" width="560" height="280"></canvas></div>
    </div>
    <div class="controls">
      <button class="secondary" id="loadHistoryBtn">Recharger historique LittleFS</button>
      <button class="orange" data-action="mqttFault5">Démo panne MQTT 5 min</button>
      <button class="secondary" data-action="mqttFaultOff">Stop panne MQTT</button>
    </div>
  </section>

  <section id="commands">
    <div class="grid">
      <div class="card span2">
        <div class="title">Lumières</div>
        <div class="controls">
          <button class="green" data-action="autoLeds">Mode auto</button>
          <button class="secondary" data-action="manualLeds">Mode manuel</button>
          <button class="red" data-action="allLedsOff">Toutes OFF</button>
          <button data-led="red" data-on="1">Rouge ON</button>
          <button class="secondary" data-led="red" data-on="0">Rouge OFF</button>
          <button data-led="blue" data-on="1">Bleue ON</button>
          <button class="secondary" data-led="blue" data-on="0">Bleue OFF</button>
          <button data-led="green" data-on="1">Verte ON</button>
          <button class="secondary" data-led="green" data-on="0">Verte OFF</button>
          <button data-led="yellow" data-on="1">Jaune ON</button>
          <button class="secondary" data-led="yellow" data-on="0">Jaune OFF</button>
          <button data-led="white" data-on="1">Blanche ON</button>
          <button class="secondary" data-led="white" data-on="0">Blanche OFF</button>
        </div>
      </div>

      <div class="card span2">
        <div class="title">État LEDs</div>
        <table>
          <tr><td>Mode manuel</td><td id="ledModeVal">--</td></tr>
          <tr><td>Rouge</td><td id="ledRedVal">--</td></tr>
          <tr><td>Bleue</td><td id="ledBlueVal">--</td></tr>
          <tr><td>Verte</td><td id="ledGreenVal">--</td></tr>
          <tr><td>Jaune</td><td id="ledYellowVal">--</td></tr>
          <tr><td>Blanche</td><td id="ledWhiteVal">--</td></tr>
        </table>
      </div>

      <div class="card span2">
        <div class="title">Sécurité / alarme</div>
        <div class="controls">
          <button class="red" data-action="securityOn">Sécurité ON</button>
          <button class="secondary" data-action="securityOff">Sécurité OFF</button>
          <button class="orange" data-action="ack">Acquitter</button>
          <button class="green" data-action="soundOn">Son ON</button>
          <button class="secondary" data-action="soundOff">Son OFF</button>
          <button class="purple" data-action="melodyDemo">Mélodie 2 buzzers</button>
          <button class="secondary" data-action="melodyStop">Stop mélodie</button>
          <button class="red" data-action="criticalOn">Interruption critique ON</button>
          <button class="secondary" data-action="criticalOff">Interruption critique OFF</button>
        </div>
      </div>

      <div class="card span2">
        <div class="title">MQTT / réseau / démos</div>
        <div class="controls">
          <button class="green" data-action="mqttOn">MQTT ON</button>
          <button class="secondary" data-action="mqttOff">MQTT OFF</button>
          <button class="orange" data-action="mqttFault5">Panne MQTT 5 min</button>
          <button class="secondary" data-action="mqttFaultOff">Stop panne MQTT</button>
          <button class="orange" data-action="wifiOff60">Couper WiFi 1 min</button>
          <button class="orange" data-action="wifiOff300">Couper WiFi 5 min</button>
          <button class="green" data-action="wifiOn">WiFi ON</button>
          <button class="purple" id="demoLightsBtn">Démo lumières</button>
          <button class="purple" id="demoSecurityBtn">Démo sécurité</button>
          <button class="purple" id="demoResetBtn">Reset démo</button>
        </div>
      </div>
    </div>
  </section>

  <section id="games">
    <div class="grid">
      <div class="card span2">
        <div class="title">Jeux joystick</div>
        <canvas class="gameCanvas" id="gameCanvas" width="430" height="330"></canvas>
        <div class="controls">
          <button class="green" data-game="snake">Snake</button>
          <button data-game="collect">Collecteur</button>
          <button data-game="dodge">Dodge</button>
          <button class="secondary" id="restartGameBtn">Recommencer</button>
        </div>
        <div class="sub">Joystick HW-504 ou clavier flèches. Bouton joystick / espace = recommencer.</div>
      </div>

      <div class="card span2">
        <div class="title">Joystick / score</div>
        <table>
          <tr><td>Direction</td><td id="joyDirVal">--</td></tr>
          <tr><td>X</td><td id="joyXVal">--</td></tr>
          <tr><td>Y</td><td id="joyYVal">--</td></tr>
          <tr><td>Bouton</td><td id="joyBtnVal">--</td></tr>
          <tr><td>Jeu</td><td id="gameNameVal">Snake</td></tr>
          <tr><td>Score</td><td id="gameScoreVal">0</td></tr>
        </table>
      </div>
    </div>
  </section>

  <section id="system">
    <div class="grid">
      <div class="card span2">
        <div class="title">ESP32 / FreeRTOS</div>
        <table>
          <tr><td>Core 0 réseau/MQTT/Web</td><td id="core0Val">--</td></tr>
          <tr><td>Core 1 capteurs/actionneurs</td><td id="core1Val">--</td></tr>
          <tr><td>Loops sensors</td><td id="loopsSensorsVal">--</td></tr>
          <tr><td>Loops actuators</td><td id="loopsActuatorsVal">--</td></tr>
          <tr><td>Loops MQTT</td><td id="loopsMqttVal">--</td></tr>
          <tr><td>Loops WiFi</td><td id="loopsWiFiVal">--</td></tr>
          <tr><td>Loops interruption</td><td id="loopsCriticalVal">--</td></tr>
        </table>
      </div>

      <div class="card span2">
        <div class="title">Réseau / mémoire / offline</div>
        <table>
          <tr><td>IP</td><td id="ipVal">--</td></tr>
          <tr><td>RSSI</td><td id="rssiVal">--</td></tr>
          <tr><td>Heap</td><td id="heapVal">--</td></tr>
          <tr><td>Uptime</td><td id="uptimeVal">--</td></tr>
          <tr><td>Queue</td><td id="queueVal">--</td></tr>
          <tr><td>MQTT</td><td id="mqttVal">--</td></tr>
          <tr><td>Latence MQTT</td><td id="latencyVal">--</td></tr>
          <tr><td>Offline</td><td id="offlineVal">--</td></tr>
          <tr><td>Replayed</td><td id="replayedVal">--</td></tr>
          <tr><td>Mélodie buzzers</td><td id="melodySystemVal">--</td></tr>
          <tr><td>Interruption critique</td><td id="criticalSystemVal">--</td></tr>
        </table>
        <div class="controls">
          <button class="red" data-action="clearDb">Vider BDD locale</button>
          <button class="secondary" data-action="resetStats">Reset stats</button>
        </div>
      </div>
    </div>
  </section>

  <section id="settings">
    <div class="grid">
      <div class="card span2">
        <div class="title">Seuils</div>
        <label>Température haute</label>
        <input id="tempLimit" type="number" step="0.1" value="35">
        <label>Humidité basse</label>
        <input id="humLow" type="number" step="0.1" value="30">
        <label>Humidité haute</label>
        <input id="humHigh" type="number" step="0.1" value="75">
        <label>Seuil gaz</label>
        <input id="gasLimit" type="number" step="1" value="2500">
        <label>Seuil radar en cm</label>
        <input id="radarLimit" type="number" step="1" value="80">
        <button id="saveThresholdsBtn">Sauver seuils</button>
      </div>

      <div class="card span2">
        <div class="title">MQTT</div>
        <label>Topic base MQTT</label>
        <input id="mqttBase" value="campus/groupe1/ESP32-Othmane">
        <div class="controls">
          <button class="green" id="saveMqttOnBtn">Sauver + MQTT ON</button>
          <button class="secondary" id="saveMqttOffBtn">Sauver + MQTT OFF</button>
        </div>
      </div>
    </div>
  </section>
</main>

<footer>© 2026 ESGI — DE VIALA ROSALES Gérard, BALTACHE Othmane, BOUBAKER Oussema, MIVELLE Erwan</footer>

<script>
(() => {
  const TOKEN = "1234";
  const POLL_MS = 1600;
  const MAX_POINTS = 100;
  const MAX_ECHOES = 8;

  const $ = id => document.getElementById(id);
  const set = (id, value) => {
    const e = $(id);
    if(e && e.textContent !== String(value)) e.textContent = value;
  };

  const isNum = v => v !== null && v !== undefined && !isNaN(v);
  const fmt = (v, unit="", dec=1) => isNum(v) ? Number(v).toFixed(dec) + unit : "--";

  let live = null;
  let points = [];
  let echoes = [];
  let activeTab = "dash";
  let polling = false;
  let eventsLast = 0;
  let sweepAngle = 0;
  let lastEchoDistance = null;
  let lastEchoMs = 0;

  function timeLabel(ms){
    return new Date(ms).toLocaleTimeString("fr-FR",{hour:"2-digit",minute:"2-digit",second:"2-digit"});
  }

  function eventTime(ts){
    if(!live) return "--";
    return timeLabel(Date.now() - ((live.uptime - ts) * 1000));
  }

  function pill(id, label, cls){
    const e = $(id);
    if(!e) return;
    e.textContent = label;
    e.className = "pill " + (cls || "");
  }

  document.querySelectorAll(".tabs button").forEach(btn => {
    btn.addEventListener("click", () => {
      activeTab = btn.dataset.tab;
      document.querySelectorAll("section").forEach(s => s.classList.remove("active"));
      document.querySelectorAll(".tabs button").forEach(b => b.classList.remove("active"));
      $(activeTab).classList.add("active");
      btn.classList.add("active");
      drawVisible();
    });
  });

  document.body.addEventListener("click", async ev => {
    const b = ev.target.closest("button");
    if(!b) return;

    if(b.dataset.action) await action(b.dataset.action);
    if(b.dataset.led) await led(b.dataset.led, b.dataset.on);
    if(b.dataset.game) selectGame(b.dataset.game);
  });

  $("saveThresholdsBtn").onclick = () => saveConfig(false);
  $("saveMqttOnBtn").onclick = () => saveConfig(true);
  $("saveMqttOffBtn").onclick = () => saveConfig(false);
  $("loadHistoryBtn").onclick = loadHistory;
  $("restartGameBtn").onclick = resetGame;
  $("clearEchoesBtn").onclick = () => { echoes = []; set("radarEchoCount", 0); };
  $("demoLightsBtn").onclick = demoLights;
  $("demoSecurityBtn").onclick = demoSecurity;
  $("demoResetBtn").onclick = demoReset;

  async function poll(){
    if(polling){
      setTimeout(poll, POLL_MS);
      return;
    }

    polling = true;

    try{
      const r = await fetch("/api/live?nocache=" + Date.now());
      live = await r.json();
      updateUI(live);
      addPoint(live);
      updateEchoes(live);
      drawVisible();

      if(Date.now() - eventsLast > 5000){
        eventsLast = Date.now();
        loadEvents();
      }
    }catch(e){
      pill("wifiPill", "API inaccessible", "bad");
    }

    polling = false;
    setTimeout(poll, POLL_MS);
  }

  async function loadEvents(){
    try{
      const r = await fetch("/api/events?nocache=" + Date.now());
      const events = await r.json();

      $("eventsList").innerHTML = events.length ? events.map(ev => `
        <div class="event ${ev.level}">
          <div class="eventTop"><span>${eventTime(ev.ts)} • ${ev.type} • ${ev.level}</span><span>t+${ev.ts}s</span></div>
          <div>${ev.message}</div>
        </div>
      `).join("") : "<div class='sub'>Aucun événement</div>";
    }catch(e){}
  }

  async function loadHistory(){
    try{
      if(!live) return;

      const r = await fetch("/api/history?limit=60&nocache=" + Date.now());
      const hist = await r.json();

      const loaded = hist.map(p => {
        const sec = Math.round((p.createdMs || 0) / 1000);
        return {
          t: Date.now() - ((live.uptime - sec) * 1000),
          temp:p.temp,
          hum:p.humidity,
          gas:p.gasPercent,
          distance:p.distanceCm,
          risk:p.riskScore,
          core0:live.core0Load,
          core1:live.core1Load
        };
      });

      points = loaded.concat(points).slice(-MAX_POINTS);
      drawVisible();
    }catch(e){}
  }

  function addPoint(d){
    const p = {
      t:Date.now(),
      temp:d.temp,
      hum:d.humidity,
      gas:d.gasPercent,
      distance:d.distanceCm,
      risk:d.riskScore,
      core0:d.core0Load,
      core1:d.core1Load
    };

    points.push(p);
    if(points.length > MAX_POINTS) points.shift();
  }

  function updateEchoes(d){
    if(!d.radarObject || d.distanceCm <= 0) return;

    const now = Date.now();
    const distanceChanged = lastEchoDistance === null || Math.abs(d.distanceCm - lastEchoDistance) > 5;
    const enoughTime = now - lastEchoMs > 1200;

    if(distanceChanged || enoughTime){
      const angle = 18 + ((now / 37) % 144);

      echoes.push({
        distance:d.distanceCm,
        angle,
        t:now
      });

      if(echoes.length > MAX_ECHOES) echoes.shift();

      lastEchoDistance = d.distanceCm;
      lastEchoMs = now;
    }

    set("radarEchoCount", echoes.length);
  }

  function updateUI(d){
    set("clock", timeLabel(Date.now()));

    pill("wifiPill", d.wifi ? "WiFi connecté" : "WiFi off", d.wifi ? "ok" : "bad");
    pill("mqttPill", d.mqttConnected ? "MQTT connecté" : (d.mqttEnabled ? "MQTT actif" : "MQTT OFF"), d.mqttConnected ? "ok" : "warn");
    pill("securityPill", d.security ? "Sécurité ON" : "Sécurité OFF", d.security ? "bad" : "ok");
    pill("radarPill", d.radarObject ? "Objet détecté" : "Radar libre", d.radarObject ? "bad" : "ok");
    pill("riskPill", "Risque " + d.riskScore + "% " + d.riskState, d.riskScore >= 55 ? "bad" : d.riskScore >= 30 ? "warn" : "ok");

    set("tempVal", fmt(d.temp," °C"));
    set("humVal", fmt(d.humidity," %"));
    set("gasVal", d.gasRaw);
    set("distVal", d.distanceCm < 0 ? "--" : d.distanceCm.toFixed(1) + " cm");

    set("dhtState", d.dhtOk ? "DHT22 OK" : "Panne DHT22");
    set("gasState", d.gasOk ? "Gaz OK" : "Gaz suspect");
    set("distState", d.radarOk ? (d.radarObject ? "Objet détecté" : "Zone libre") : "Sans écho");

    set("objVal", d.radarObject ? "OUI" : "NON");
    set("secVal", d.security ? "ON" : "OFF");
    set("intrVal", d.intrusion ? "OUI" : "NON");
    set("radarCountVal", d.radarDetectCount);
    set("hwVal", d.hw499 ? "Actif" : "RAS");
    set("joyVal", d.joyDirection || "--");
    set("melodyDemoVal", d.melodyDemo ? "Active" : "OFF");
    set("criticalVal", d.criticalInterrupt ? "ACTIVE" : "OFF");

    set("radarDist", d.distanceCm < 0 ? "--" : d.distanceCm.toFixed(1) + " cm");
    set("radarLimitText", d.radarLimit.toFixed(1) + " cm");
    set("distanceMinVal", d.distanceMin === null ? "--" : d.distanceMin.toFixed(1) + " cm");
    set("radarStatus", d.radarObject ? "Objet dans la zone" : "Zone libre");

    set("ledModeVal", d.manualLedMode ? "Manuel" : "Auto");
    set("ledRedVal", d.ledRed ? "ON" : "OFF");
    set("ledBlueVal", d.ledBlue ? "ON" : "OFF");
    set("ledGreenVal", d.ledGreen ? "ON" : "OFF");
    set("ledYellowVal", d.ledYellow ? "ON" : "OFF");
    set("ledWhiteVal", d.ledWhite ? "ON" : "OFF");

    set("core0Val", d.core0Load.toFixed(1) + " %");
    set("core1Val", d.core1Load.toFixed(1) + " %");
    set("loopsSensorsVal", d.loopsSensors);
    set("loopsActuatorsVal", d.loopsActuators);
    set("loopsMqttVal", d.loopsMqtt);
    set("loopsWiFiVal", d.loopsWiFi);
    set("loopsCriticalVal", d.loopsCritical || 0);

    set("ipVal", d.ip);
    set("rssiVal", d.rssi + " dBm");
    set("heapVal", d.heap + " bytes");
    set("uptimeVal", d.uptime + " s");
    set("queueVal", d.queueWaiting + " attente / " + d.queueSpaces + " libres");
    set("mqttVal", d.mqttConnected ? "Connecté" : (d.mqttEnabled ? "Actif" : "OFF"));
    set("latencyVal", d.mqttLatency + " ms");
    set("offlineVal", d.offlineCount);
    set("replayedVal", d.replayedCount);
    set("melodySystemVal", d.melodyDemo ? "Active" : "OFF");
    set("criticalSystemVal", d.criticalInterrupt ? "ACTIVE" : "OFF");

    set("joyDirVal", d.joyDirection || "--");
    set("joyXVal", d.joyX);
    set("joyYVal", d.joyY);
    set("joyBtnVal", d.joyButton ? "Appuyé" : "Relâché");
  }

  function drawVisible(){
    if(activeTab === "graphs"){
      drawChart("chartTemp", [{key:"temp", label:"Température", unit:"°C", color:"#0b4ea2"}], "Température (°C)");
      drawChart("chartHum", [{key:"hum", label:"Humidité", unit:"%", color:"#198754"}], "Humidité (%)");
      drawChart("chartGas", [{key:"gas", label:"Gaz", unit:"%", color:"#f59e0b"}], "Gaz (%)");
      drawChart("chartDistance", [{key:"distance", label:"Distance", unit:"cm", color:"#dc3545"}], "Distance (cm)");
      drawChart("chartRisk", [{key:"risk", label:"Risque", unit:"%", color:"#7c3aed"}], "Risque (%)");
      drawChart("chartCpu", [
        {key:"core0", label:"Core 0", unit:"%", color:"#0b4ea2"},
        {key:"core1", label:"Core 1", unit:"%", color:"#198754"}
      ], "Charge CPU (%)");
    }
  }

  function drawChart(id, series, yLabel){
    const c = $(id);
    if(!c || points.length < 2) return;

    const ctx = c.getContext("2d");
    const w = c.width, h = c.height;
    ctx.clearRect(0,0,w,h);
    ctx.fillStyle="#fff";
    ctx.fillRect(0,0,w,h);

    const L=62, R=18, T=26, B=46, PW=w-L-R, PH=h-T-B;

    ctx.strokeStyle="#e4e8f0";
    ctx.lineWidth=1;
    for(let i=0;i<=4;i++){
      const y = T + PH*i/4;
      ctx.beginPath();
      ctx.moveTo(L,y);
      ctx.lineTo(w-R,y);
      ctx.stroke();
    }

    let mn=Infinity, mx=-Infinity;
    points.forEach(p => series.forEach(s => {
      const v = p[s.key];
      if(isNum(v)){
        mn = Math.min(mn,v);
        mx = Math.max(mx,v);
      }
    }));

    if(mn === Infinity){ mn=0; mx=1; }
    if(mx === mn) mx = mn + 1;

    const pad = (mx-mn)*0.12;
    mn = Math.max(0, mn-pad);
    mx += pad;

    const t0 = points[0].t;
    const t1 = points[points.length-1].t || t0+1;
    const X = t => L + ((t-t0)/(t1-t0 || 1))*PW;
    const Y = v => T + PH - ((v-mn)/(mx-mn))*PH;

    ctx.fillStyle="#667085";
    ctx.font="11px Arial";
    for(let i=0;i<=4;i++){
      const v = mx - (mx-mn)*i/4;
      const y = T + PH*i/4;
      ctx.fillText(v.toFixed(v>=100 ? 0 : 1), 8, y+4);
    }

    ctx.fillStyle="#1f2937";
    ctx.font="bold 12px Arial";
    ctx.fillText(yLabel, L, 15);
    ctx.fillText("Heure", w-60, h-10);

    ctx.fillStyle="#667085";
    ctx.font="11px Arial";
    for(let i=0;i<4;i++){
      const p = points[Math.floor((points.length-1)*i/3)];
      if(p) ctx.fillText(timeLabel(p.t), X(p.t)-27, h-27);
    }

    series.forEach((s, idx) => {
      ctx.strokeStyle=s.color;
      ctx.lineWidth=2.4;
      ctx.beginPath();

      let started=false;
      points.forEach(p => {
        const v = p[s.key];
        if(!isNum(v)) return;
        const x = X(p.t);
        const y = Y(v);

        if(!started){
          ctx.moveTo(x,y);
          started=true;
        } else {
          ctx.lineTo(x,y);
        }
      });

      ctx.stroke();

      const last = [...points].reverse().find(p => isNum(p[s.key]));
      const legend = s.label + (last ? " : " + Number(last[s.key]).toFixed(1) + " " + s.unit : "");

      ctx.fillStyle=s.color;
      ctx.font="bold 12px Arial";
      ctx.fillText(legend, L+8, T+16+idx*18);
    });
  }

  function radarLoop(){
    if(activeTab === "radar") drawRadar();
    requestAnimationFrame(radarLoop);
  }

  function drawRadar(){
    if(!live) return;

    const c = $("radarCanvas");
    const ctx = c.getContext("2d");
    const w=c.width, h=c.height;
    const cx=w/2;
    const cy=h-26;
    const max=live.radarMax || 250;
    const maxR=h-64;

    sweepAngle = (sweepAngle + 1.3) % 180;

    ctx.clearRect(0,0,w,h);
    ctx.fillStyle="#f8fbff";
    ctx.fillRect(0,0,w,h);

    ctx.strokeStyle="#d9e2ef";
    ctx.lineWidth=1;
    ctx.fillStyle="#667085";
    ctx.font="11px Arial";

    for(let r=25;r<=max;r+=25){
      const rr=(r/max)*maxR;

      ctx.beginPath();
      ctx.arc(cx,cy,rr,Math.PI,0);
      ctx.stroke();

      if(r % 50 === 0){
        ctx.fillText(r+" cm", cx+rr-35, cy-6);
      }
    }

    for(let a=0;a<=180;a+=15){
      const rad = Math.PI*a/180;
      ctx.beginPath();
      ctx.moveTo(cx,cy);
      ctx.lineTo(cx+Math.cos(Math.PI-rad)*maxR, cy-Math.sin(rad)*maxR);
      ctx.stroke();
    }

    const limitR = Math.min(live.radarLimit || 80, max)/max*maxR;
    ctx.strokeStyle="rgba(245,158,11,.9)";
    ctx.lineWidth=2;
    ctx.beginPath();
    ctx.arc(cx,cy,limitR,Math.PI,0);
    ctx.stroke();

    // Anciennes détections : plusieurs échos récents
    const now = Date.now();
    echoes = echoes.filter(e => now - e.t < 20000);

    echoes.forEach((e, i) => {
      const age = (now - e.t) / 20000;
      const alpha = Math.max(0.20, 1 - age);
      const rr = Math.min(e.distance,max)/max*maxR;
      const rad = Math.PI*e.angle/180;
      const x = cx + Math.cos(Math.PI-rad)*rr;
      const y = cy - Math.sin(rad)*rr;

      ctx.fillStyle = "rgba(11,78,162,"+alpha+")";
      ctx.beginPath();
      ctx.arc(x,y,7,0,Math.PI*2);
      ctx.fill();

      ctx.fillStyle = "rgba(31,41,55,"+alpha+")";
      ctx.font="bold 11px Arial";
      ctx.fillText((i+1)+" • "+e.distance.toFixed(0)+"cm", x+9, y+4);
    });

    // Distance courante au centre
    if(live.distanceCm > 0){
      const rr = Math.min(live.distanceCm,max)/max*maxR;
      const x = cx;
      const y = cy - rr;

      ctx.fillStyle = live.radarObject ? "#dc3545" : "#198754";
      ctx.beginPath();
      ctx.arc(x,y,10,0,Math.PI*2);
      ctx.fill();

      ctx.fillStyle="#1f2937";
      ctx.font="bold 12px Arial";
      ctx.fillText(live.distanceCm.toFixed(1)+" cm", x+14, y+4);
    }

    const sweep = Math.PI*sweepAngle/180;
    ctx.strokeStyle="rgba(25,135,84,.75)";
    ctx.lineWidth=3;
    ctx.beginPath();
    ctx.moveTo(cx,cy);
    ctx.lineTo(cx+Math.cos(Math.PI-sweep)*maxR, cy-Math.sin(sweep)*maxR);
    ctx.stroke();

    ctx.fillStyle="#1f2937";
    ctx.font="bold 15px Arial";
    ctx.fillText("Radar HC-SR04", 14, 22);

    ctx.font="12px Arial";
    ctx.fillText("Seuil : " + (live.radarLimit || 80).toFixed(0) + " cm", 14, 42);
    ctx.fillText("Échos récents : " + echoes.length, 14, 60);

    set("radarEchoCount", echoes.length);
  }

  // Jeux
  const gameCanvas = $("gameCanvas");
  const g = gameCanvas.getContext("2d");

  let game = "snake";
  let score = 0;
  let keyDir = "RIGHT";
  let snake, food, dir, player, target, dodgeItems, gameOver;

  function selectGame(name){
    game = name;
    set("gameNameVal", game === "snake" ? "Snake" : game === "collect" ? "Collecteur" : "Dodge");
    resetGame();
  }

  function resetGame(){
    score = 0;
    gameOver = false;

    snake = [{x:10,y:9}];
    food = {x:15,y:10};
    dir = {x:1,y:0};

    player = {x:200,y:160,size:16};
    target = {x:80+Math.random()*260,y:50+Math.random()*220,size:14};

    dodgeItems = [];
    for(let i=0;i<6;i++){
      dodgeItems.push({
        x:Math.random()*410,
        y:Math.random()*250+30,
        vx:(Math.random()-.5)*3,
        vy:(Math.random()-.5)*3,
        r:8
      });
    }

    set("gameScoreVal", score);
  }

  function joystickDirection(){
    if(live && live.joyDirection && live.joyDirection !== "CENTER") return live.joyDirection;
    return keyDir;
  }

  document.addEventListener("keydown", e => {
    if(e.key === "ArrowLeft") keyDir = "LEFT";
    if(e.key === "ArrowRight") keyDir = "RIGHT";
    if(e.key === "ArrowUp") keyDir = "UP";
    if(e.key === "ArrowDown") keyDir = "DOWN";
    if(e.key === " ") resetGame();
  });

  function gameLoop(){
    if(activeTab === "games"){
      if(live && live.joyButton) resetGame();

      if(game === "snake") drawSnake();
      else if(game === "collect") drawCollect();
      else drawDodge();
    }

    requestAnimationFrame(gameLoop);
  }

  let snakeTick = 0;

  function drawSnake(){
    snakeTick++;

    if(snakeTick % 9 === 0 && !gameOver){
      const d = joystickDirection();

      if(d==="LEFT" && dir.x!==1) dir={x:-1,y:0};
      if(d==="RIGHT" && dir.x!==-1) dir={x:1,y:0};
      if(d==="UP" && dir.y!==1) dir={x:0,y:-1};
      if(d==="DOWN" && dir.y!==-1) dir={x:0,y:1};

      const head = {x:snake[0].x+dir.x,y:snake[0].y+dir.y};

      if(head.x<0 || head.y<0 || head.x>=24 || head.y>=18 || snake.some(p=>p.x===head.x && p.y===head.y)){
        gameOver = true;
      }else{
        snake.unshift(head);

        if(head.x===food.x && head.y===food.y){
          score++;
          food={x:Math.floor(Math.random()*24),y:Math.floor(Math.random()*18)};
        }else{
          snake.pop();
        }
      }

      set("gameScoreVal", score);
    }

    g.clearRect(0,0,430,330);
    g.fillStyle="#f8fbff"; g.fillRect(0,0,430,330);
    g.fillStyle="#dc3545"; g.fillRect(food.x*18,food.y*18,16,16);
    g.fillStyle="#0b4ea2"; snake.forEach(p=>g.fillRect(p.x*18,p.y*18,16,16));
    g.fillStyle="#1f2937"; g.font="bold 14px Arial"; g.fillText("Snake - Score : "+score,12,20);

    if(gameOver){
      g.fillStyle="#dc3545";
      g.font="bold 28px Arial";
      g.fillText("GAME OVER",120,170);
    }
  }

  function movePlayer(speed){
    const d = joystickDirection();

    if(d==="LEFT") player.x -= speed;
    if(d==="RIGHT") player.x += speed;
    if(d==="UP") player.y -= speed;
    if(d==="DOWN") player.y += speed;

    player.x = Math.max(10, Math.min(410, player.x));
    player.y = Math.max(25, Math.min(310, player.y));
  }

  function drawCollect(){
    movePlayer(3.2);

    const dx=player.x-target.x;
    const dy=player.y-target.y;

    if(Math.sqrt(dx*dx+dy*dy)<24){
      score++;
      target={x:25+Math.random()*380,y:40+Math.random()*250,size:14};
      set("gameScoreVal", score);
    }

    g.clearRect(0,0,430,330);
    g.fillStyle="#f8fbff"; g.fillRect(0,0,430,330);
    g.fillStyle="#198754"; g.beginPath(); g.arc(target.x,target.y,target.size,0,Math.PI*2); g.fill();
    g.fillStyle="#0b4ea2"; g.fillRect(player.x-10,player.y-10,20,20);
    g.fillStyle="#1f2937"; g.font="bold 14px Arial"; g.fillText("Collecteur - Score : "+score,12,20);
  }

  function drawDodge(){
    if(!gameOver){
      movePlayer(3.0);

      dodgeItems.forEach(o=>{
        o.x+=o.vx; o.y+=o.vy;

        if(o.x<10||o.x>420) o.vx*=-1;
        if(o.y<30||o.y>320) o.vy*=-1;

        const dx=player.x-o.x;
        const dy=player.y-o.y;

        if(Math.sqrt(dx*dx+dy*dy)<22) gameOver=true;
      });

      score++;
      set("gameScoreVal", score);
    }

    g.clearRect(0,0,430,330);
    g.fillStyle="#f8fbff"; g.fillRect(0,0,430,330);
    g.fillStyle="#0b4ea2"; g.fillRect(player.x-10,player.y-10,20,20);
    g.fillStyle="#dc3545";

    dodgeItems.forEach(o=>{
      g.beginPath();
      g.arc(o.x,o.y,o.r,0,Math.PI*2);
      g.fill();
    });

    g.fillStyle="#1f2937"; g.font="bold 14px Arial"; g.fillText("Dodge - Score : "+score,12,20);

    if(gameOver){
      g.fillStyle="#dc3545";
      g.font="bold 28px Arial";
      g.fillText("TOUCHÉ",145,170);
    }
  }

  async function action(cmd){
    await fetch("/api/action?token="+TOKEN+"&cmd="+cmd+"&nocache="+Date.now());
  }

  async function led(name,on){
    await fetch("/api/action?token="+TOKEN+"&cmd=led&led="+name+"&on="+on+"&nocache="+Date.now());
  }

  async function saveConfig(mqtt){
    const q = new URLSearchParams();
    q.set("token", TOKEN);
    q.set("tempLimit", $("tempLimit").value);
    q.set("humLow", $("humLow").value);
    q.set("humHigh", $("humHigh").value);
    q.set("gasLimit", $("gasLimit").value);
    q.set("radarLimit", $("radarLimit").value);
    q.set("mqtt", mqtt ? "1" : "0");
    q.set("mqttBase", $("mqttBase").value);
    await fetch("/api/config?" + q.toString());
  }

  async function demoLights(){
    await action("manualLeds");

    const seq = [
      ["red",1],["red",0],["blue",1],["blue",0],
      ["green",1],["green",0],["yellow",1],["yellow",0],
      ["white",1],["white",0]
    ];

    let delay = 0;

    seq.forEach(item => {
      setTimeout(() => led(item[0], item[1]), delay);
      delay += 320;
    });
  }

  async function demoSecurity(){
    await action("securityOn");
    alert("Démo sécurité activée : approche ta main devant le HC-SR04 à moins du seuil radar.");
  }

  async function demoReset(){
    await action("criticalOff");
    await action("melodyStop");
    await action("ack");
    await action("mqttFaultOff");
    await action("autoLeds");
    await action("soundOn");
  }

  set("clock", timeLabel(Date.now()));
  setInterval(() => set("clock", timeLabel(Date.now())), 1000);

  resetGame();
  poll();
  radarLoop();
  gameLoop();
})();
</script>
</body>
</html>
)rawliteral";
