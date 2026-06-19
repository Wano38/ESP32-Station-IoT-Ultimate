#include <Arduino.h>

extern const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <title>Station IoT ESGI</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <style>
    :root {
      --blue:#073985;
      --blue2:#0b55d9;
      --lightBlue:#d9e7ff;
      --sky:#eef5ff;
      --ink:#071d49;
      --muted:#64748b;
      --border:#d7e0ee;
      --card:#ffffff;
      --bg:#f5f7fb;
      --green:#198754;
      --orange:#f59e0b;
      --red:#dc3545;
      --shadow:0 12px 30px rgba(7,29,73,.08);
      --radius:26px;
    }

    *{box-sizing:border-box}
    body{
      margin:0;
      font-family:Arial,Helvetica,sans-serif;
      color:var(--ink);
      background:var(--bg);
    }

    .page{
      min-height:100vh;
      position:relative;
      overflow-x:hidden;
      padding-bottom:44px;
    }

    .shapeTop{
      position:absolute;
      top:-80px;
      left:-50px;
      width:420px;
      height:170px;
      border-radius:0 0 220px 0;
      background:var(--blue);
      z-index:0;
    }
    .shapeTop2{
      position:absolute;
      top:38px;
      left:0;
      width:260px;
      height:95px;
      border-radius:0 0 180px 0;
      background:rgba(73,132,255,.55);
      z-index:0;
    }
    .shapeRight{
      position:absolute;
      top:-75px;
      right:-60px;
      width:420px;
      height:170px;
      border-radius:0 0 0 220px;
      background:var(--blue);
      z-index:0;
    }
    .shapeRight2{
      position:absolute;
      top:42px;
      right:0;
      width:270px;
      height:88px;
      border-radius:0 0 0 180px;
      background:rgba(73,132,255,.40);
      z-index:0;
    }
    .shapeBottom{
      position:absolute;
      left:-55px;
      bottom:-40px;
      width:360px;
      height:120px;
      transform:rotate(-4deg);
      background:linear-gradient(135deg,#1300a8 0%,#0d2d69 70%);
      border-radius:14px;
      z-index:0;
    }
    .shapeBottom:after{
      content:"";
      position:absolute;
      right:22px;
      top:-16px;
      width:48px;
      height:48px;
      background:#b7bdc6;
      transform:rotate(45deg);
    }

    header{
      position:relative;
      z-index:1;
      height:150px;
      display:flex;
      justify-content:center;
      align-items:center;
      text-align:center;
    }

    .logo{
      position:absolute;
      top:18px;
      right:24px;
      text-align:right;
      font-weight:900;
      letter-spacing:-2px;
      color:var(--ink);
      font-size:34px;
      line-height:.8;
    }
    .logo span{
      display:block;
      font-size:11px;
      letter-spacing:0;
      font-weight:700;
      color:#2575e6;
      margin-top:5px;
    }

    .titleBlock h1{
      margin:0;
      font-size:36px;
      color:var(--ink);
      letter-spacing:.5px;
    }
    .titleBlock p{
      margin:8px 0 0;
      color:var(--muted);
      font-size:15px;
    }

    main{
      position:relative;
      z-index:1;
      width:min(1180px,calc(100% - 36px));
      margin:0 auto;
    }

    .heroCard{
      background:var(--card);
      border:1px solid var(--border);
      border-radius:var(--radius);
      box-shadow:var(--shadow);
      padding:26px;
      min-height:210px;
      display:grid;
      grid-template-columns:1.25fr .75fr;
      gap:18px;
      align-items:center;
      overflow:hidden;
      position:relative;
    }

    .heroCard:after{
      content:"";
      position:absolute;
      right:-60px;
      bottom:-90px;
      width:260px;
      height:260px;
      border:34px solid rgba(11,85,217,.12);
      border-radius:50%;
    }

    .kpiBig{
      font-size:72px;
      line-height:.9;
      font-weight:900;
      color:var(--blue);
      text-align:center;
    }
    .kpiLabel{
      text-align:center;
      margin-top:12px;
      font-size:14px;
      font-weight:800;
      text-transform:uppercase;
      color:var(--ink);
    }

    .statusPills{
      display:flex;
      flex-wrap:wrap;
      gap:8px;
      margin-top:18px;
    }
    .pill{
      border:1px solid var(--border);
      background:#f8fbff;
      padding:8px 12px;
      border-radius:999px;
      color:var(--ink);
      font-size:13px;
      font-weight:700;
    }
    .pill.ok{color:var(--green)}
    .pill.warn{color:var(--orange)}
    .pill.bad{color:var(--red)}

    .tabs{
      display:flex;
      flex-wrap:wrap;
      gap:8px;
      margin:18px 0;
    }
    .tab{
      border:1px solid var(--border);
      background:white;
      color:var(--ink);
      padding:10px 14px;
      border-radius:12px;
      cursor:pointer;
      font-weight:800;
      box-shadow:0 4px 12px rgba(7,29,73,.04);
    }
    .tab.active{
      background:var(--blue);
      color:white;
      border-color:var(--blue);
    }

    .section{display:none}
    .section.active{display:block}

    .grid{
      display:grid;
      grid-template-columns:repeat(4,minmax(0,1fr));
      gap:14px;
      margin-bottom:18px;
    }
    .card{
      background:white;
      border:1px solid var(--border);
      border-radius:18px;
      box-shadow:0 8px 22px rgba(7,29,73,.06);
      padding:16px;
    }
    .span2{grid-column:span 2}
    .span4{grid-column:span 4}

    .cardTitle{
      color:var(--muted);
      font-size:13px;
      text-transform:uppercase;
      letter-spacing:.7px;
      font-weight:900;
      margin-bottom:10px;
    }
    .value{
      font-size:34px;
      font-weight:900;
      color:var(--blue);
    }
    .sub{
      margin-top:6px;
      color:var(--muted);
      font-size:13px;
    }
    .okText{color:var(--green)}
    .warnText{color:var(--orange)}
    .badText{color:var(--red)}

    canvas.chart{
      width:100%;
      height:260px;
      border:1px solid var(--border);
      border-radius:16px;
      background:#fff;
    }

    .controls{
      display:flex;
      gap:8px;
      flex-wrap:wrap;
      margin-top:10px;
    }
    button{
      border:0;
      background:var(--blue);
      color:white;
      font-weight:800;
      padding:10px 12px;
      border-radius:12px;
      cursor:pointer;
    }
    button.secondary{background:#eef3fb;color:var(--ink);border:1px solid var(--border)}
    button.green{background:var(--green)}
    button.orange{background:var(--orange)}
    button.red{background:var(--red)}

    input{
      width:100%;
      border:1px solid var(--border);
      border-radius:12px;
      padding:10px;
      margin:6px 0 12px;
      color:var(--ink);
      background:#fff;
    }
    label{
      display:block;
      color:var(--muted);
      font-weight:800;
      font-size:13px;
    }

    table{
      width:100%;
      border-collapse:collapse;
    }
    td{
      padding:8px 0;
      border-bottom:1px solid #edf1f7;
      color:var(--muted);
      font-size:14px;
    }
    td:last-child{
      color:var(--ink);
      font-weight:900;
      text-align:right;
    }

    .events{
      max-height:310px;
      overflow:auto;
    }
    .event{
      padding:10px 12px;
      border-left:4px solid var(--blue2);
      background:#f8fbff;
      border-radius:10px;
      margin-bottom:8px;
    }
    .event.WARNING{border-color:var(--orange)}
    .event.CRITIQUE,.event.CRITICAL{border-color:var(--red)}
    .eventTop{
      display:flex;
      justify-content:space-between;
      gap:10px;
      color:var(--muted);
      font-size:12px;
      font-weight:800;
      margin-bottom:4px;
    }

    .gameCanvas{
      width:100%;
      max-width:460px;
      height:360px;
      border:2px solid var(--blue);
      border-radius:16px;
      background:#f8fbff;
      display:block;
      margin:auto;
    }

    footer{
      position:relative;
      z-index:1;
      width:min(1180px,calc(100% - 36px));
      margin:18px auto 0;
      display:flex;
      justify-content:space-between;
      align-items:center;
      gap:12px;
      flex-wrap:wrap;
      color:var(--ink);
      font-weight:800;
      font-size:13px;
    }
    .classification{
      border:6px solid var(--ink);
      border-radius:8px;
      padding:8px 12px;
      background:white;
    }
    .pageNum{
      border:6px solid var(--ink);
      border-radius:8px;
      padding:8px 12px;
      background:white;
    }

    @media(max-width:850px){
      header{height:130px}
      .titleBlock h1{font-size:26px}
      .logo{font-size:26px}
      .heroCard{grid-template-columns:1fr}
      .grid{grid-template-columns:1fr}
      .span2,.span4{grid-column:span 1}
      .kpiBig{font-size:54px}
    }
  </style>
</head>

<body>
<div class="page">
  <div class="shapeTop"></div>
  <div class="shapeTop2"></div>
  <div class="shapeRight"></div>
  <div class="shapeRight2"></div>
  <div class="shapeBottom"></div>

  <header>
    <div class="logo">ESGI<span>école supérieure de<br>génie informatique</span></div>
    <div class="titleBlock">
      <h1>Station IoT Industrielle</h1>
      <p>ESP32 • FreeRTOS • MQTT • Offline Replay • Supervision</p>
    </div>
  </header>

  <main>
    <section class="heroCard">
      <div>
        <h2 style="margin:0;color:var(--ink);font-size:28px;">Dashboard de supervision</h2>
        <p style="color:var(--muted);line-height:1.5;">
          Acquisition capteurs, surveillance réseau, base locale, MQTT et supervision processeur double cœur.
        </p>
        <div class="statusPills">
          <span class="pill" id="clock">--:--:--</span>
          <span class="pill" id="wifiPill">WiFi --</span>
          <span class="pill" id="mqttPill">MQTT --</span>
          <span class="pill" id="offlinePill">Offline --</span>
          <span class="pill" id="securityPill">Sécurité --</span>
        </div>
      </div>
      <div>
        <div class="kpiBig" id="riskHero">--%</div>
        <div class="kpiLabel" id="riskHeroLabel">Score de risque</div>
      </div>
    </section>

    <nav class="tabs">
      <button class="tab active" onclick="showTab('dash',this)">Dashboard</button>
      <button class="tab" onclick="showTab('graphs',this)">Graphiques</button>
      <button class="tab" onclick="showTab('cpu',this)">Supervision CPU</button>
      <button class="tab" onclick="showTab('offline',this)">BDD / Offline</button>
      <button class="tab" onclick="showTab('game',this)">Jeu joystick</button>
      <button class="tab" onclick="showTab('settings',this)">Réglages</button>
    </nav>

    <section id="dash" class="section active">
      <div class="grid">
        <div class="card">
          <div class="cardTitle">Température</div>
          <div class="value" id="tempVal">-- °C</div>
          <div class="sub" id="dhtState">DHT22 --</div>
        </div>
        <div class="card">
          <div class="cardTitle">Humidité</div>
          <div class="value" id="humVal">-- %</div>
          <div class="sub">Seuils configurables</div>
        </div>
        <div class="card">
          <div class="cardTitle">Gaz MQ</div>
          <div class="value" id="gasVal">--</div>
          <div class="sub" id="gasState">Capteur gaz --</div>
        </div>
        <div class="card">
          <div class="cardTitle">Présence</div>
          <div class="value" id="pirVal">--</div>
          <div class="sub">PIR + HW-499</div>
        </div>
        <div class="card span2">
          <div class="cardTitle">État système</div>
          <table>
            <tr><td>Adresse IP</td><td id="ipVal">--</td></tr>
            <tr><td>RSSI WiFi</td><td id="rssiVal">--</td></tr>
            <tr><td>Heap libre</td><td id="heapVal">--</td></tr>
            <tr><td>Uptime</td><td id="uptimeVal">--</td></tr>
            <tr><td>Queue capteurs</td><td id="queueVal">--</td></tr>
          </table>
        </div>
        <div class="card span2">
          <div class="cardTitle">Historique événements avec heure</div>
          <div class="events" id="eventsList">Chargement...</div>
        </div>
      </div>
    </section>

    <section id="graphs" class="section">
      <div class="grid">
        <div class="card span4">
          <div class="cardTitle">Mesures live avec ligne de temps</div>
          <canvas id="sensorChart" class="chart" width="1000" height="280"></canvas>
        </div>
        <div class="card span4">
          <div class="cardTitle">Timeline réseau / MQTT / offline / risque</div>
          <canvas id="networkChart" class="chart" width="1000" height="280"></canvas>
          <div class="controls">
            <button class="secondary" onclick="loadHistory()">Recharger historique LittleFS</button>
            <button class="orange" onclick="action('mqttFault5')">Panne MQTT 5 min</button>
            <button class="orange" onclick="action('mqttFault30')">Panne MQTT 30 min</button>
            <button class="secondary" onclick="action('mqttFaultOff')">Stop panne MQTT</button>
          </div>
        </div>
      </div>
    </section>

    <section id="cpu" class="section">
      <div class="grid">
        <div class="card span4">
          <div class="cardTitle">Supervision processeur ESP32 double cœur</div>
          <canvas id="cpuChart" class="chart" width="1000" height="280"></canvas>
        </div>
        <div class="card span2">
          <div class="cardTitle">Charge estimée</div>
          <table>
            <tr><td>Core 0 réseau/web/MQTT</td><td id="core0Val">-- %</td></tr>
            <tr><td>Core 1 capteurs/supervision</td><td id="core1Val">-- %</td></tr>
            <tr><td>Latence MQTT</td><td id="mqttLatencyVal">--</td></tr>
            <tr><td>Publications MQTT OK</td><td id="mqttOkVal">--</td></tr>
            <tr><td>Erreurs MQTT</td><td id="mqttFailVal">--</td></tr>
          </table>
        </div>
        <div class="card span2">
          <div class="cardTitle">Répartition FreeRTOS</div>
          <table>
            <tr><td>TaskSensors</td><td>Priorité 3 • Core 1</td></tr>
            <tr><td>TaskMQTT</td><td>Priorité 2 • Core 0</td></tr>
            <tr><td>TaskSupervision</td><td>Priorité 2 • Core 1</td></tr>
            <tr><td>TaskWiFi / TaskWeb</td><td>Priorité 1 • Core 0</td></tr>
            <tr><td>TaskCpuMonitor</td><td>Priorité 1 • Core 0</td></tr>
          </table>
        </div>
      </div>
    </section>

    <section id="offline" class="section">
      <div class="grid">
        <div class="card span2">
          <div class="cardTitle">Base locale / Offline</div>
          <table>
            <tr><td>Données offline</td><td id="offlineVal">--</td></tr>
            <tr><td>Données rejouées</td><td id="replayedVal">--</td></tr>
            <tr><td>Queue drops</td><td id="dropsVal">--</td></tr>
            <tr><td>Panne MQTT restante</td><td id="mqttFaultVal">--</td></tr>
            <tr><td>Coupure WiFi restante</td><td id="wifiOffVal">--</td></tr>
          </table>
          <div class="controls">
            <button class="red" onclick="action('clearDb')">Vider BDD locale</button>
            <button class="orange" onclick="action('wifiOff60')">Couper WiFi 1 min</button>
            <button class="orange" onclick="action('wifiOff30')">Couper WiFi 30 min</button>
            <button class="green" onclick="action('wifiOn')">WiFi ON</button>
          </div>
        </div>
        <div class="card span2">
          <div class="cardTitle">Statistiques min / max</div>
          <table>
            <tr><td>Température min</td><td id="tempMinVal">--</td></tr>
            <tr><td>Température max</td><td id="tempMaxVal">--</td></tr>
            <tr><td>Humidité min</td><td id="humMinVal">--</td></tr>
            <tr><td>Humidité max</td><td id="humMaxVal">--</td></tr>
            <tr><td>Gaz max</td><td id="gasMaxVal">--</td></tr>
          </table>
          <div class="controls">
            <button class="secondary" onclick="action('resetStats')">Reset statistiques</button>
          </div>
        </div>
      </div>
    </section>

    <section id="game" class="section">
      <div class="grid">
        <div class="card span2">
          <div class="cardTitle">Snake joystick HW-504</div>
          <canvas id="snake" class="gameCanvas" width="360" height="360"></canvas>
          <div class="sub">Joystick HW-504 ou flèches clavier. Bouton joystick ou espace pour recommencer.</div>
        </div>
        <div class="card span2">
          <div class="cardTitle">Joystick</div>
          <table>
            <tr><td>Direction</td><td id="joyDir">--</td></tr>
            <tr><td>X</td><td id="joyX">--</td></tr>
            <tr><td>Y</td><td id="joyY">--</td></tr>
            <tr><td>Bouton</td><td id="joyBtn">--</td></tr>
          </table>
        </div>
      </div>
    </section>

    <section id="settings" class="section">
      <div class="grid">
        <div class="card span2">
          <div class="cardTitle">MQTT</div>
          <label>Topic MQTT base</label>
          <input id="mqttBase" value="campus/groupe1/ESP32-Othmane">
          <div class="controls">
            <button class="green" onclick="saveConfig(true)">Sauver + MQTT ON</button>
            <button class="secondary" onclick="saveConfig(false)">Sauver + MQTT OFF</button>
            <button class="green" onclick="action('mqttOn')">MQTT ON</button>
            <button class="secondary" onclick="action('mqttOff')">MQTT OFF</button>
          </div>
        </div>
        <div class="card span2">
          <div class="cardTitle">Seuils</div>
          <label>Température haute</label>
          <input id="tempLimit" type="number" step="0.1" value="35">
          <label>Humidité basse</label>
          <input id="humLow" type="number" step="0.1" value="30">
          <label>Humidité haute</label>
          <input id="humHigh" type="number" step="0.1" value="75">
          <label>Seuil gaz</label>
          <input id="gasLimit" type="number" step="1" value="2500">
          <button onclick="saveConfig(false)">Sauver seuils</button>
        </div>
        <div class="card span4">
          <div class="cardTitle">Sécurité</div>
          <div class="controls">
            <button class="red" onclick="action('securityOn')">Sécurité ON</button>
            <button class="secondary" onclick="action('securityOff')">Sécurité OFF</button>
            <button class="orange" onclick="action('ack')">Acquitter alarme</button>
          </div>
        </div>
      </div>
    </section>
  </main>

  <footer>
    <div class="classification">Classification : ESGI • Projet IoT ESP32</div>
    <div>© 2026 ESGI — BALTACHE Othmane, BOUBAKER Oussema, MIVELLE Erwan</div>
    <div class="pageNum">IoT</div>
  </footer>
</div>

<script>
  const TOKEN = "1234";
  const MAX_POINTS = 80;

  let live = null;
  let livePoints = [];
  let networkPoints = [];
  let cpuPoints = [];
  let lastUptime = 0;

  function $(id){ return document.getElementById(id); }

  function showTab(id, btn){
    document.querySelectorAll('.section').forEach(s=>s.classList.remove('active'));
    document.querySelectorAll('.tab').forEach(t=>t.classList.remove('active'));
    $(id).classList.add('active');
    btn.classList.add('active');
  }

  function fmtTime(ms){
    return new Date(ms).toLocaleTimeString('fr-FR',{hour:'2-digit',minute:'2-digit',second:'2-digit'});
  }

  function uptimeToClock(uptimeSec){
    if(!live) return "—";
    const now = Date.now();
    const delta = (live.uptime - uptimeSec) * 1000;
    return fmtTime(now - delta);
  }

  function setPill(id, text, status){
    const el = $(id);
    el.textContent = text;
    el.className = 'pill ' + (status || '');
  }

  function updateClock(){
    $('clock').textContent = fmtTime(Date.now());
  }
  setInterval(updateClock,1000);
  updateClock();

  async function fetchLive(){
    try{
      const res = await fetch('/api/live?nocache=' + Date.now());
      live = await res.json();
      lastUptime = live.uptime;
      updateUI(live);
      addLivePoint(live);
      drawAllCharts();
    }catch(e){
      setPill('wifiPill','API inaccessible','bad');
    }
    setTimeout(fetchLive,2000);
  }

  async function loadEvents(){
    try{
      const res = await fetch('/api/events?nocache=' + Date.now());
      const events = await res.json();
      $('eventsList').innerHTML = events.length ? events.map(ev => {
        const levelClass = ev.level || 'INFO';
        return `<div class="event ${levelClass}">
          <div class="eventTop"><span>${uptimeToClock(ev.ts)} • ${ev.type} • ${ev.level}</span><span>t+${ev.ts}s</span></div>
          <div>${ev.message}</div>
        </div>`;
      }).join('') : '<div class="sub">Aucun événement</div>';
    }catch(e){}
  }

  async function loadHistory(){
    try{
      const res = await fetch('/api/history?limit=80&nocache=' + Date.now());
      const hist = await res.json();

      hist.forEach(p=>{
        if(!p.createdMs) return;
        const sec = Math.round(p.createdMs/1000);
        const t = Date.now() - ((lastUptime - sec) * 1000);
        livePoints.push({
          t,
          temp:p.temp,
          hum:p.humidity,
          gas:p.gasPercent,
          replayed:p.replayed || false
        });
      });

      livePoints = livePoints.slice(-MAX_POINTS);
      drawAllCharts();
    }catch(e){}
  }

  setInterval(loadEvents,5000);

  function addLivePoint(d){
    const now = Date.now();

    livePoints.push({
      t:now,
      temp:d.temp,
      hum:d.humidity,
      gas:d.gasPercent
    });

    networkPoints.push({
      t:now,
      wifi:d.wifi ? 1 : 0,
      mqtt:d.mqttConnected ? 1 : 0,
      offline:d.offlineCount > 0 ? 1 : 0,
      risk:d.riskScore
    });

    cpuPoints.push({
      t:now,
      c0:d.core0Load,
      c1:d.core1Load,
      heap:d.heap / 1000
    });

    livePoints = livePoints.slice(-MAX_POINTS);
    networkPoints = networkPoints.slice(-MAX_POINTS);
    cpuPoints = cpuPoints.slice(-MAX_POINTS);
  }

  function updateUI(d){
    $('riskHero').textContent = d.riskScore + '%';
    $('riskHeroLabel').textContent = d.riskState;

    setPill('wifiPill', d.wifi ? 'WiFi connecté' : 'WiFi déconnecté', d.wifi ? 'ok' : 'bad');
    setPill('mqttPill', d.mqttConnected ? 'MQTT connecté' : (d.mqttEnabled ? 'MQTT actif' : 'MQTT OFF'), d.mqttConnected ? 'ok' : 'warn');
    setPill('offlinePill', 'Offline ' + d.offlineCount, d.offlineCount > 0 ? 'warn' : 'ok');
    setPill('securityPill', d.security ? 'Sécurité ON' : 'Sécurité OFF', d.security ? 'bad' : 'ok');

    $('tempVal').textContent = d.temp === null ? 'Erreur' : d.temp.toFixed(1) + ' °C';
    $('humVal').textContent = d.humidity === null ? 'Erreur' : d.humidity.toFixed(1) + ' %';
    $('gasVal').textContent = d.gasRaw;
    $('pirVal').textContent = d.pir ? 'Actif' : 'RAS';

    $('dhtState').textContent = d.dhtOk ? 'DHT22 OK' : 'Panne DHT22';
    $('dhtState').className = 'sub ' + (d.dhtOk ? 'okText' : 'badText');
    $('gasState').textContent = d.gasOk ? 'Gaz OK' : 'Gaz suspect';
    $('gasState').className = 'sub ' + (d.gasOk ? 'okText' : 'warnText');

    $('ipVal').textContent = d.ip;
    $('rssiVal').textContent = d.rssi + ' dBm';
    $('heapVal').textContent = d.heap + ' bytes';
    $('uptimeVal').textContent = d.uptime + ' s';
    $('queueVal').textContent = d.queueWaiting + ' en attente / ' + d.queueSpaces + ' libres';

    $('core0Val').textContent = d.core0Load.toFixed(1) + ' %';
    $('core1Val').textContent = d.core1Load.toFixed(1) + ' %';
    $('mqttLatencyVal').textContent = d.mqttLatency + ' ms';
    $('mqttOkVal').textContent = d.mqttOkCount;
    $('mqttFailVal').textContent = d.mqttFailCount;

    $('offlineVal').textContent = d.offlineCount;
    $('replayedVal').textContent = d.replayedCount;
    $('dropsVal').textContent = d.queueDrops;
    $('mqttFaultVal').textContent = d.mqttFaultRemaining + ' s';
    $('wifiOffVal').textContent = d.wifiOffRemaining + ' s';

    $('tempMinVal').textContent = d.tempMin === null ? '--' : d.tempMin.toFixed(1) + ' °C';
    $('tempMaxVal').textContent = d.tempMax === null ? '--' : d.tempMax.toFixed(1) + ' °C';
    $('humMinVal').textContent = d.humMin === null ? '--' : d.humMin.toFixed(1) + ' %';
    $('humMaxVal').textContent = d.humMax === null ? '--' : d.humMax.toFixed(1) + ' %';
    $('gasMaxVal').textContent = d.gasMax;

    $('joyDir').textContent = d.joyDirection;
    $('joyX').textContent = d.joyX;
    $('joyY').textContent = d.joyY;
    $('joyBtn').textContent = d.joyButton ? 'Appuyé' : 'Relâché';
  }

  function drawLineChart(canvasId, data, series, title){
    const c = $(canvasId);
    const ctx = c.getContext('2d');
    const w = c.width, h = c.height;
    ctx.clearRect(0,0,w,h);

    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0,0,w,h);

    const padL = 54, padR = 18, padT = 24, padB = 42;
    const plotW = w - padL - padR;
    const plotH = h - padT - padB;

    ctx.strokeStyle = '#d7e0ee';
    ctx.lineWidth = 1;
    for(let i=0;i<=4;i++){
      const y = padT + (plotH/4)*i;
      ctx.beginPath(); ctx.moveTo(padL,y); ctx.lineTo(w-padR,y); ctx.stroke();
    }

    ctx.fillStyle = '#071d49';
    ctx.font = 'bold 14px Arial';
    ctx.fillText(title, padL, 16);

    if(data.length < 2) return;

    const tMin = data[0].t;
    const tMax = data[data.length-1].t || (tMin+1);

    let yMin = Infinity, yMax = -Infinity;
    data.forEach(p=>{
      series.forEach(s=>{
        const v = p[s.key];
        if(v !== null && v !== undefined && !isNaN(v)){
          yMin = Math.min(yMin,v);
          yMax = Math.max(yMax,v);
        }
      });
    });
    if(yMin === Infinity){ yMin = 0; yMax = 1; }
    if(yMax === yMin) yMax = yMin + 1;
    yMin = Math.max(0, yMin - (yMax-yMin)*0.1);
    yMax = yMax + (yMax-yMin)*0.12;

    function x(t){ return padL + ((t-tMin)/(tMax-tMin || 1))*plotW; }
    function y(v){ return padT + plotH - ((v-yMin)/(yMax-yMin))*plotH; }

    series.forEach(s=>{
      ctx.strokeStyle = s.color;
      ctx.lineWidth = 2.5;
      ctx.beginPath();
      let started = false;
      data.forEach(p=>{
        const v = p[s.key];
        if(v === null || v === undefined || isNaN(v)) return;
        const px = x(p.t), py = y(v);
        if(!started){ ctx.moveTo(px,py); started = true; }
        else ctx.lineTo(px,py);
      });
      ctx.stroke();

      ctx.fillStyle = s.color;
      ctx.fillText(s.label, s.legendX || (w-140), s.legendY || 18);
    });

    ctx.fillStyle = '#64748b';
    ctx.font = '12px Arial';
    for(let i=0;i<4;i++){
      const idx = Math.floor((data.length-1) * i / 3);
      const p = data[idx];
      if(!p) continue;
      ctx.fillText(fmtTime(p.t), x(p.t)-28, h-15);
    }
  }

  function drawAllCharts(){
    drawLineChart('sensorChart', livePoints, [
      {key:'temp', label:'Température °C', color:'#0b55d9', legendX:720, legendY:18},
      {key:'hum', label:'Humidité %', color:'#198754', legendX:720, legendY:36},
      {key:'gas', label:'Gaz %', color:'#f59e0b', legendX:720, legendY:54}
    ], 'Mesures live avec heure actuelle');

    drawLineChart('networkChart', networkPoints, [
      {key:'wifi', label:'WiFi 0/1', color:'#198754', legendX:735, legendY:18},
      {key:'mqtt', label:'MQTT 0/1', color:'#0b55d9', legendX:735, legendY:36},
      {key:'offline', label:'Offline 0/1', color:'#f59e0b', legendX:735, legendY:54},
      {key:'risk', label:'Risque', color:'#dc3545', legendX:735, legendY:72}
    ], 'Timeline réseau et offline');

    drawLineChart('cpuChart', cpuPoints, [
      {key:'c0', label:'Core 0 %', color:'#0b55d9', legendX:735, legendY:18},
      {key:'c1', label:'Core 1 %', color:'#198754', legendX:735, legendY:36},
      {key:'heap', label:'Heap kB', color:'#f59e0b', legendX:735, legendY:54}
    ], 'Charge CPU estimée et mémoire');
  }

  async function action(cmd){
    await fetch('/api/action?token=' + TOKEN + '&cmd=' + cmd + '&nocache=' + Date.now());
    fetchLive();
    loadEvents();
  }

  async function saveConfig(mqttOn){
    const q = new URLSearchParams();
    q.set('token',TOKEN);
    q.set('tempLimit',$('tempLimit').value);
    q.set('humLow',$('humLow').value);
    q.set('humHigh',$('humHigh').value);
    q.set('gasLimit',$('gasLimit').value);
    q.set('mqtt',mqttOn ? '1':'0');
    q.set('mqttBase',$('mqttBase').value);
    await fetch('/api/config?' + q.toString());
    fetchLive();
  }

  // Snake léger
  const snakeCanvas = $('snake');
  const sctx = snakeCanvas.getContext('2d');
  const cell = 18;
  let snake = [{x:10,y:10}], dir={x:1,y:0}, food={x:5,y:5}, gameOver=false, score=0;

  function resetSnake(){
    snake=[{x:10,y:10}]; dir={x:1,y:0}; food={x:5,y:5}; gameOver=false; score=0;
  }

  function stepSnake(){
    if(live){
      if(live.joyDirection === 'LEFT' && dir.x !== 1) dir={x:-1,y:0};
      if(live.joyDirection === 'RIGHT' && dir.x !== -1) dir={x:1,y:0};
      if(live.joyDirection === 'UP' && dir.y !== 1) dir={x:0,y:-1};
      if(live.joyDirection === 'DOWN' && dir.y !== -1) dir={x:0,y:1};
      if(live.joyButton) resetSnake();
    }

    if(!gameOver){
      const head={x:snake[0].x+dir.x,y:snake[0].y+dir.y};
      if(head.x<0||head.y<0||head.x>=20||head.y>=20||snake.some(p=>p.x===head.x&&p.y===head.y)){
        gameOver=true;
      }else{
        snake.unshift(head);
        if(head.x===food.x && head.y===food.y){
          score++;
          food={x:Math.floor(Math.random()*20),y:Math.floor(Math.random()*20)};
        }else{
          snake.pop();
        }
      }
    }

    drawSnake();
  }

  function drawSnake(){
    sctx.clearRect(0,0,360,360);
    sctx.fillStyle='#f8fbff'; sctx.fillRect(0,0,360,360);
    sctx.fillStyle='#dc3545'; sctx.fillRect(food.x*cell,food.y*cell,cell-2,cell-2);
    sctx.fillStyle='#073985';
    snake.forEach(p=>sctx.fillRect(p.x*cell,p.y*cell,cell-2,cell-2));
    sctx.fillStyle='#071d49'; sctx.font='bold 14px Arial'; sctx.fillText('Score: '+score,10,18);
    if(gameOver){sctx.fillStyle='#dc3545';sctx.font='bold 28px Arial';sctx.fillText('GAME OVER',88,180);}
  }

  document.addEventListener('keydown',e=>{
    if(e.key==='ArrowLeft' && dir.x!==1) dir={x:-1,y:0};
    if(e.key==='ArrowRight' && dir.x!==-1) dir={x:1,y:0};
    if(e.key==='ArrowUp' && dir.y!==1) dir={x:0,y:-1};
    if(e.key==='ArrowDown' && dir.y!==-1) dir={x:0,y:1};
    if(e.key===' ') resetSnake();
  });

  setInterval(stepSnake,180);

  fetchLive();
  loadEvents();
  setTimeout(loadHistory,1500);
</script>
</body>
</html>
)rawliteral";
