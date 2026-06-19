#include <Arduino.h>

extern const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <title>Station IoT ESP32 - ESGI</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <style>
    :root{
      --blue:#073985;
      --blue2:#0b55d9;
      --bg:#f6f8fc;
      --card:#ffffff;
      --ink:#071d49;
      --muted:#667085;
      --line:#d9e2ef;
      --soft:#eef5ff;
      --green:#198754;
      --orange:#f59e0b;
      --red:#dc3545;
      --shadow:0 8px 24px rgba(7,29,73,.07);
      --radius:18px;
    }

    *{box-sizing:border-box}
    body{
      margin:0;
      font-family:Arial,Helvetica,sans-serif;
      background:var(--bg);
      color:var(--ink);
    }

    .top{
      position:relative;
      overflow:hidden;
      background:#fff;
      border-bottom:1px solid var(--line);
      padding:28px 18px 22px;
    }
    .top:before{
      content:"";
      position:absolute;
      left:-70px;
      top:-70px;
      width:380px;
      height:150px;
      border-radius:0 0 230px 0;
      background:var(--blue);
    }
    .top:after{
      content:"";
      position:absolute;
      right:-70px;
      top:-70px;
      width:380px;
      height:150px;
      border-radius:0 0 0 230px;
      background:var(--blue);
    }

    .topInner{
      position:relative;
      z-index:1;
      width:min(1180px,calc(100% - 10px));
      margin:auto;
      display:flex;
      align-items:center;
      justify-content:space-between;
      gap:16px;
    }

    .brand h1{
      margin:0;
      font-size:30px;
      color:var(--ink);
    }
    .brand p{
      margin:7px 0 0;
      color:var(--muted);
      font-size:14px;
    }

    .logo{
      text-align:right;
      font-weight:900;
      color:var(--ink);
      font-size:32px;
      letter-spacing:-2px;
      line-height:.8;
    }
    .logo span{
      display:block;
      font-size:11px;
      letter-spacing:0;
      color:#2575e6;
      margin-top:6px;
    }

    .wrap{
      width:min(1180px,calc(100% - 24px));
      margin:18px auto 0;
    }

    .hero{
      background:var(--card);
      border:1px solid var(--line);
      border-radius:24px;
      box-shadow:var(--shadow);
      padding:20px;
      display:grid;
      grid-template-columns:1.35fr .65fr;
      gap:16px;
      align-items:center;
    }

    .hero h2{
      margin:0 0 8px;
      font-size:24px;
    }
    .hero p{
      margin:0;
      color:var(--muted);
      line-height:1.45;
    }

    .risk{
      text-align:center;
    }
    .risk .num{
      font-size:60px;
      font-weight:900;
      color:var(--blue);
      line-height:1;
    }
    .risk .label{
      margin-top:6px;
      font-weight:900;
      color:var(--ink);
      text-transform:uppercase;
      font-size:13px;
    }

    .pills{
      display:flex;
      flex-wrap:wrap;
      gap:8px;
      margin-top:14px;
    }
    .pill{
      border:1px solid var(--line);
      background:var(--soft);
      color:var(--ink);
      padding:7px 10px;
      border-radius:999px;
      font-weight:800;
      font-size:13px;
    }
    .ok{color:var(--green)!important}
    .warn{color:var(--orange)!important}
    .bad{color:var(--red)!important}

    .tabs{
      display:flex;
      gap:8px;
      flex-wrap:wrap;
      margin:16px 0;
      position:sticky;
      top:0;
      z-index:5;
      background:var(--bg);
      padding:8px 0;
    }
    .tab{
      border:1px solid var(--line);
      background:#fff;
      color:var(--ink);
      padding:9px 12px;
      border-radius:10px;
      font-weight:900;
      cursor:pointer;
      box-shadow:0 3px 10px rgba(7,29,73,.04);
    }
    .tab.active{
      background:var(--blue);
      color:#fff;
      border-color:var(--blue);
    }

    .section{display:none}
    .section.active{display:block}

    .grid{
      display:grid;
      grid-template-columns:repeat(4,minmax(0,1fr));
      gap:12px;
    }

    .card{
      background:#fff;
      border:1px solid var(--line);
      border-radius:var(--radius);
      box-shadow:0 5px 16px rgba(7,29,73,.05);
      padding:15px;
    }

    .span2{grid-column:span 2}
    .span4{grid-column:span 4}

    .title{
      color:var(--muted);
      font-size:12px;
      font-weight:900;
      letter-spacing:.5px;
      text-transform:uppercase;
      margin-bottom:9px;
    }

    .value{
      color:var(--blue);
      font-size:31px;
      font-weight:900;
      line-height:1.1;
    }

    .sub{
      color:var(--muted);
      margin-top:6px;
      font-size:13px;
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
      color:var(--ink);
      font-weight:900;
      text-align:right;
    }

    .controls{
      display:flex;
      gap:8px;
      flex-wrap:wrap;
      margin-top:10px;
    }

    button{
      border:0;
      border-radius:10px;
      padding:9px 11px;
      background:var(--blue);
      color:#fff;
      font-weight:900;
      cursor:pointer;
    }
    button.secondary{
      background:#eef3fb;
      color:var(--ink);
      border:1px solid var(--line);
    }
    button.green{background:var(--green)}
    button.orange{background:var(--orange)}
    button.red{background:var(--red)}

    input{
      width:100%;
      padding:9px;
      border:1px solid var(--line);
      border-radius:10px;
      margin:6px 0 11px;
      background:#fff;
      color:var(--ink);
    }
    label{
      color:var(--muted);
      font-size:13px;
      font-weight:900;
    }

    canvas.chart{
      width:100%;
      height:250px;
      border:1px solid var(--line);
      border-radius:14px;
      background:#fff;
      display:block;
    }

    .radarLayout{
      display:grid;
      grid-template-columns:360px 1fr;
      gap:16px;
      align-items:center;
    }

    #radarCanvas{
      width:100%;
      height:260px;
      border:1px solid var(--line);
      border-radius:14px;
      background:#f8fbff;
      display:block;
    }

    .events{
      max-height:290px;
      overflow:auto;
      padding-right:4px;
    }
    .event{
      background:#f8fbff;
      border-left:4px solid var(--blue2);
      border-radius:10px;
      padding:9px 10px;
      margin-bottom:8px;
    }
    .event.WARNING{border-left-color:var(--orange)}
    .event.CRITIQUE,.event.CRITICAL{border-left-color:var(--red)}
    .eventTop{
      display:flex;
      justify-content:space-between;
      color:var(--muted);
      font-size:12px;
      font-weight:900;
      margin-bottom:4px;
    }

    footer{
      width:min(1180px,calc(100% - 24px));
      margin:18px auto;
      color:var(--muted);
      font-size:13px;
      display:flex;
      justify-content:space-between;
      gap:10px;
      flex-wrap:wrap;
      align-items:center;
    }

    .badgeFooter{
      border:4px solid var(--ink);
      border-radius:8px;
      background:#fff;
      color:var(--ink);
      font-weight:900;
      padding:7px 10px;
    }

    @media(max-width:900px){
      .topInner,.hero,.radarLayout{grid-template-columns:1fr;display:grid}
      .grid{grid-template-columns:1fr}
      .span2,.span4{grid-column:span 1}
      .logo{font-size:25px}
      .brand h1{font-size:24px}
      .risk .num{font-size:46px}
    }
  </style>
</head>

<body>
  <header class="top">
    <div class="topInner">
      <div class="brand">
        <h1>Station IoT ESP32</h1>
        <p>HC-SR04 • DHT22 • Gaz MQ • FreeRTOS • MQTT • Offline Replay</p>
      </div>
      <div class="logo">ESGI<span>école supérieure de<br>génie informatique</span></div>
    </div>
  </header>

  <div class="wrap">
    <section class="hero">
      <div>
        <h2>Supervision fonctionnelle</h2>
        <p>Interface allégée pour limiter la latence : une seule API principale, graphiques Canvas natifs, radar léger, commandes directes.</p>
        <div class="pills">
          <span class="pill" id="clock">--:--:--</span>
          <span class="pill" id="wifiPill">WiFi --</span>
          <span class="pill" id="mqttPill">MQTT --</span>
          <span class="pill" id="securityPill">Sécurité --</span>
          <span class="pill" id="radarPill">Radar --</span>
        </div>
      </div>
      <div class="risk">
        <div class="num" id="riskHero">--%</div>
        <div class="label" id="riskLabel">Risque</div>
      </div>
    </section>

    <nav class="tabs">
      <button class="tab active" data-tab="dash">Dashboard</button>
      <button class="tab" data-tab="radar">Radar</button>
      <button class="tab" data-tab="graphs">Graphiques</button>
      <button class="tab" data-tab="leds">LEDs</button>
      <button class="tab" data-tab="system">Système</button>
      <button class="tab" data-tab="settings">Réglages</button>
    </nav>

    <section id="dash" class="section active">
      <div class="grid">
        <div class="card">
          <div class="title">Température</div>
          <div class="value" id="tempVal">-- °C</div>
          <div class="sub" id="dhtState">DHT22 --</div>
        </div>
        <div class="card">
          <div class="title">Humidité</div>
          <div class="value" id="humVal">-- %</div>
          <div class="sub">Mesure DHT22</div>
        </div>
        <div class="card">
          <div class="title">Gaz MQ</div>
          <div class="value" id="gasVal">--</div>
          <div class="sub" id="gasState">Gaz --</div>
        </div>
        <div class="card">
          <div class="title">Distance</div>
          <div class="value" id="distVal">-- cm</div>
          <div class="sub" id="distState">HC-SR04 --</div>
        </div>
        <div class="card span2">
          <div class="title">État général</div>
          <table>
            <tr><td>Objet détecté</td><td id="objVal">--</td></tr>
            <tr><td>Sécurité</td><td id="secVal">--</td></tr>
            <tr><td>Intrusion</td><td id="intrVal">--</td></tr>
            <tr><td>Détections radar</td><td id="radarCountVal">--</td></tr>
            <tr><td>HW-499</td><td id="hwVal">--</td></tr>
          </table>
        </div>
        <div class="card span2">
          <div class="title">Historique événements</div>
          <div class="events" id="eventsList">Chargement...</div>
        </div>
      </div>
    </section>

    <section id="radar" class="section">
      <div class="card span4">
        <div class="title">Radar détecteur HC-SR04</div>
        <div class="radarLayout">
          <canvas id="radarCanvas" width="360" height="260"></canvas>
          <div>
            <table>
              <tr><td>Distance actuelle</td><td id="radarDist">--</td></tr>
              <tr><td>Seuil de détection</td><td id="radarLimitText">--</td></tr>
              <tr><td>Distance minimale</td><td id="distanceMinVal">--</td></tr>
              <tr><td>État radar</td><td id="radarStatus">--</td></tr>
            </table>
            <div class="controls">
              <button class="red" data-action="securityOn">Sécurité ON</button>
              <button class="secondary" data-action="securityOff">Sécurité OFF</button>
              <button class="orange" data-action="ack">Acquitter alarme</button>
            </div>
          </div>
        </div>
      </div>
    </section>

    <section id="graphs" class="section">
      <div class="grid">
        <div class="card span4">
          <div class="title">Capteurs avec heure actuelle</div>
          <canvas id="sensorChart" class="chart" width="1000" height="260"></canvas>
        </div>
        <div class="card span4">
          <div class="title">Réseau / MQTT / Offline / Risque</div>
          <canvas id="networkChart" class="chart" width="1000" height="260"></canvas>
          <div class="controls">
            <button class="secondary" id="reloadHistory">Recharger historique</button>
            <button class="orange" data-action="mqttFault5">Panne MQTT 5 min</button>
            <button class="secondary" data-action="mqttFaultOff">Stop panne MQTT</button>
          </div>
        </div>
      </div>
    </section>

    <section id="leds" class="section">
      <div class="grid">
        <div class="card span4">
          <div class="title">Commandes LEDs</div>
          <div class="controls">
            <button class="green" data-action="autoLeds">Mode automatique</button>
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
          <div class="title">Buzzer / sécurité</div>
          <div class="controls">
            <button class="green" data-action="soundOn">Son ON</button>
            <button class="secondary" data-action="soundOff">Son OFF</button>
            <button class="red" data-action="securityOn">Sécurité ON</button>
            <button class="secondary" data-action="securityOff">Sécurité OFF</button>
            <button class="orange" data-action="ack">Acquitter</button>
          </div>
        </div>
      </div>
    </section>

    <section id="system" class="section">
      <div class="grid">
        <div class="card span2">
          <div class="title">ESP32</div>
          <table>
            <tr><td>IP</td><td id="ipVal">--</td></tr>
            <tr><td>RSSI</td><td id="rssiVal">--</td></tr>
            <tr><td>Heap libre</td><td id="heapVal">--</td></tr>
            <tr><td>Uptime</td><td id="uptimeVal">--</td></tr>
            <tr><td>Queue</td><td id="queueVal">--</td></tr>
            <tr><td>Core 0</td><td id="core0Val">--</td></tr>
            <tr><td>Core 1</td><td id="core1Val">--</td></tr>
          </table>
        </div>
        <div class="card span2">
          <div class="title">MQTT / Offline</div>
          <table>
            <tr><td>MQTT</td><td id="mqttVal">--</td></tr>
            <tr><td>Latence MQTT</td><td id="latencyVal">--</td></tr>
            <tr><td>Publications OK</td><td id="mqttOkVal">--</td></tr>
            <tr><td>Erreurs MQTT</td><td id="mqttFailVal">--</td></tr>
            <tr><td>Offline</td><td id="offlineVal">--</td></tr>
            <tr><td>Rejouées</td><td id="replayedVal">--</td></tr>
          </table>
          <div class="controls">
            <button class="green" data-action="mqttOn">MQTT ON</button>
            <button class="secondary" data-action="mqttOff">MQTT OFF</button>
            <button class="red" data-action="clearDb">Vider BDD</button>
          </div>
        </div>
      </div>
    </section>

    <section id="settings" class="section">
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
          <label>Seuil radar HC-SR04 en cm</label>
          <input id="radarLimit" type="number" step="1" value="80">
          <button id="saveThresholds">Sauver seuils</button>
        </div>
        <div class="card span2">
          <div class="title">MQTT</div>
          <label>Topic MQTT base</label>
          <input id="mqttBase" value="campus/groupe1/ESP32-Othmane">
          <div class="controls">
            <button class="green" id="saveMqttOn">Sauver + MQTT ON</button>
            <button class="secondary" id="saveMqttOff">Sauver + MQTT OFF</button>
          </div>
        </div>
      </div>
    </section>
  </div>

  <footer>
    <div class="badgeFooter">Classification : ESGI • Projet IoT</div>
    <div>© 2026 ESGI — Gérard De Viala — BALTACHE Othmane, BOUBAKER Oussema, MIVELLE Erwan</div>
    <div class="badgeFooter">HC-SR04</div>
  </footer>

<script>
(function(){
  "use strict";

  const TOKEN = "1234";
  const MAX_POINTS = 70;
  const POLL_MS = 1800;

  const el = {};
  const ids = [
    "clock","wifiPill","mqttPill","securityPill","radarPill","riskHero","riskLabel",
    "tempVal","humVal","gasVal","distVal","dhtState","gasState","distState",
    "objVal","secVal","intrVal","radarCountVal","hwVal","eventsList",
    "radarDist","radarLimitText","distanceMinVal","radarStatus",
    "ledModeVal","ledRedVal","ledBlueVal","ledGreenVal","ledYellowVal","ledWhiteVal",
    "ipVal","rssiVal","heapVal","uptimeVal","queueVal","core0Val","core1Val",
    "mqttVal","latencyVal","mqttOkVal","mqttFailVal","offlineVal","replayedVal",
    "tempLimit","humLow","humHigh","gasLimit","radarLimit","mqttBase",
    "sensorChart","networkChart","radarCanvas"
  ];

  ids.forEach(id => el[id] = document.getElementById(id));

  let live = null;
  let activeTab = "dash";
  let livePoints = [];
  let networkPoints = [];
  let lastEventLoad = 0;
  let requestBusy = false;

  function fmtTime(ms){
    return new Date(ms).toLocaleTimeString("fr-FR",{hour:"2-digit",minute:"2-digit",second:"2-digit"});
  }

  function uptimeToClock(uptimeSec){
    if(!live) return "--";
    const delta = (live.uptime - uptimeSec) * 1000;
    return fmtTime(Date.now() - delta);
  }

  function text(node, value){
    if(node && node.textContent !== String(value)) node.textContent = value;
  }

  function setClass(node, base, state){
    if(node) node.className = base + (state ? " " + state : "");
  }

  function setPill(node, label, state){
    text(node, label);
    setClass(node, "pill", state);
  }

  function showTab(id, btn){
    activeTab = id;
    document.querySelectorAll(".section").forEach(s => s.classList.remove("active"));
    document.querySelectorAll(".tab").forEach(t => t.classList.remove("active"));
    document.getElementById(id).classList.add("active");
    btn.classList.add("active");
    drawVisible();
  }

  document.querySelectorAll(".tab").forEach(btn => {
    btn.addEventListener("click", () => showTab(btn.dataset.tab, btn));
  });

  document.body.addEventListener("click", async ev => {
    const b = ev.target.closest("button");
    if(!b) return;

    if(b.dataset.action){
      await action(b.dataset.action);
    }

    if(b.dataset.led){
      await led(b.dataset.led, b.dataset.on);
    }
  });

  document.getElementById("reloadHistory").addEventListener("click", loadHistory);
  document.getElementById("saveThresholds").addEventListener("click", () => saveConfig(false));
  document.getElementById("saveMqttOn").addEventListener("click", () => saveConfig(true));
  document.getElementById("saveMqttOff").addEventListener("click", () => saveConfig(false));

  setInterval(() => {
    text(el.clock, fmtTime(Date.now()));
    if(activeTab === "radar") drawRadar();
  }, 1000);

  async function poll(){
    if(requestBusy){
      setTimeout(poll, POLL_MS);
      return;
    }

    requestBusy = true;

    try{
      const res = await fetch("/api/live?nocache=" + Date.now());
      live = await res.json();

      updateUI(live);
      addPoint(live);
      drawVisible();

      const now = Date.now();
      if(now - lastEventLoad > 5000){
        lastEventLoad = now;
        loadEvents();
      }
    }catch(e){
      setPill(el.wifiPill, "API inaccessible", "bad");
    }

    requestBusy = false;
    setTimeout(poll, POLL_MS);
  }

  async function loadEvents(){
    try{
      const res = await fetch("/api/events?nocache=" + Date.now());
      const events = await res.json();

      if(!events.length){
        el.eventsList.innerHTML = "<div class='sub'>Aucun événement</div>";
        return;
      }

      el.eventsList.innerHTML = events.map(ev => (
        "<div class='event " + ev.level + "'>" +
          "<div class='eventTop'><span>" + uptimeToClock(ev.ts) + " • " + ev.type + " • " + ev.level + "</span><span>t+" + ev.ts + "s</span></div>" +
          "<div>" + ev.message + "</div>" +
        "</div>"
      )).join("");
    }catch(e){}
  }

  async function loadHistory(){
    try{
      if(!live) return;
      const res = await fetch("/api/history?limit=70&nocache=" + Date.now());
      const hist = await res.json();

      hist.forEach(p => {
        if(!p.createdMs) return;
        const sec = Math.round(p.createdMs / 1000);
        const t = Date.now() - ((live.uptime - sec) * 1000);
        livePoints.push({
          t,
          temp:p.temp,
          hum:p.humidity,
          gas:p.gasPercent,
          distance:p.distanceCm
        });
      });

      livePoints = livePoints.slice(-MAX_POINTS);
      drawVisible();
    }catch(e){}
  }

  function addPoint(d){
    const now = Date.now();

    livePoints.push({
      t:now,
      temp:d.temp,
      hum:d.humidity,
      gas:d.gasPercent,
      distance:d.distanceCm
    });

    networkPoints.push({
      t:now,
      wifi:d.wifi ? 1 : 0,
      mqtt:d.mqttConnected ? 1 : 0,
      offline:d.offlineCount > 0 ? 1 : 0,
      risk:d.riskScore
    });

    if(livePoints.length > MAX_POINTS) livePoints.shift();
    if(networkPoints.length > MAX_POINTS) networkPoints.shift();
  }

  function updateUI(d){
    text(el.riskHero, d.riskScore + "%");
    text(el.riskLabel, d.riskState);

    setPill(el.wifiPill, d.wifi ? "WiFi connecté" : "WiFi déconnecté", d.wifi ? "ok" : "bad");
    setPill(el.mqttPill, d.mqttConnected ? "MQTT connecté" : (d.mqttEnabled ? "MQTT actif" : "MQTT OFF"), d.mqttConnected ? "ok" : "warn");
    setPill(el.securityPill, d.security ? "Sécurité ON" : "Sécurité OFF", d.security ? "bad" : "ok");
    setPill(el.radarPill, d.radarObject ? "Objet détecté" : "Radar libre", d.radarObject ? "bad" : "ok");

    text(el.tempVal, d.temp === null ? "Erreur" : d.temp.toFixed(1) + " °C");
    text(el.humVal, d.humidity === null ? "Erreur" : d.humidity.toFixed(1) + " %");
    text(el.gasVal, d.gasRaw);
    text(el.distVal, d.distanceCm === null || d.distanceCm < 0 ? "--" : d.distanceCm.toFixed(1) + " cm");

    text(el.dhtState, d.dhtOk ? "DHT22 OK" : "Panne DHT22");
    setClass(el.dhtState, "sub", d.dhtOk ? "ok" : "bad");

    text(el.gasState, d.gasOk ? "Gaz OK" : "Gaz suspect");
    setClass(el.gasState, "sub", d.gasOk ? "ok" : "warn");

    text(el.distState, d.radarOk ? (d.radarObject ? "Objet détecté" : "Zone libre") : "Radar sans écho");
    setClass(el.distState, "sub", d.radarObject ? "bad" : (d.radarOk ? "ok" : "warn"));

    text(el.objVal, d.radarObject ? "OUI" : "NON");
    text(el.secVal, d.security ? "ON" : "OFF");
    text(el.intrVal, d.intrusion ? "OUI" : "NON");
    text(el.radarCountVal, d.radarDetectCount);
    text(el.hwVal, d.hw499 ? "Actif" : "RAS");

    text(el.radarDist, d.distanceCm < 0 ? "--" : d.distanceCm.toFixed(1) + " cm");
    text(el.radarLimitText, d.radarLimit.toFixed(1) + " cm");
    text(el.distanceMinVal, d.distanceMin === null ? "--" : d.distanceMin.toFixed(1) + " cm");
    text(el.radarStatus, d.radarObject ? "Objet dans la zone" : "Zone libre");

    text(el.ledModeVal, d.manualLedMode ? "Manuel" : "Auto");
    text(el.ledRedVal, d.ledRed ? "ON" : "OFF");
    text(el.ledBlueVal, d.ledBlue ? "ON" : "OFF");
    text(el.ledGreenVal, d.ledGreen ? "ON" : "OFF");
    text(el.ledYellowVal, d.ledYellow ? "ON" : "OFF");
    text(el.ledWhiteVal, d.ledWhite ? "ON" : "OFF");

    text(el.ipVal, d.ip);
    text(el.rssiVal, d.rssi + " dBm");
    text(el.heapVal, d.heap + " bytes");
    text(el.uptimeVal, d.uptime + " s");
    text(el.queueVal, d.queueWaiting + " attente / " + d.queueSpaces + " libres");
    text(el.core0Val, d.core0Load.toFixed(1) + " %");
    text(el.core1Val, d.core1Load.toFixed(1) + " %");

    text(el.mqttVal, d.mqttConnected ? "Connecté" : (d.mqttEnabled ? "Actif" : "OFF"));
    text(el.latencyVal, d.mqttLatency + " ms");
    text(el.mqttOkVal, d.mqttOkCount);
    text(el.mqttFailVal, d.mqttFailCount);
    text(el.offlineVal, d.offlineCount);
    text(el.replayedVal, d.replayedCount);
  }

  function drawVisible(){
    if(activeTab === "radar") drawRadar();
    if(activeTab === "graphs"){
      drawLineChart(el.sensorChart, livePoints, [
        {key:"temp", label:"Temp °C", color:"#0b55d9", y:18},
        {key:"hum", label:"Hum %", color:"#198754", y:36},
        {key:"gas", label:"Gaz %", color:"#f59e0b", y:54},
        {key:"distance", label:"Distance cm", color:"#dc3545", y:72}
      ], "Capteurs avec heure actuelle");

      drawLineChart(el.networkChart, networkPoints, [
        {key:"wifi", label:"WiFi 0/1", color:"#198754", y:18},
        {key:"mqtt", label:"MQTT 0/1", color:"#0b55d9", y:36},
        {key:"offline", label:"Offline", color:"#f59e0b", y:54},
        {key:"risk", label:"Risque", color:"#dc3545", y:72}
      ], "Timeline réseau");
    }
  }

  function drawLineChart(c, data, series, title){
    if(!c || data.length < 2) return;

    const ctx = c.getContext("2d");
    const w = c.width, h = c.height;
    ctx.clearRect(0,0,w,h);
    ctx.fillStyle = "#fff";
    ctx.fillRect(0,0,w,h);

    const L=54, R=18, T=24, B=42, PW=w-L-R, PH=h-T-B;

    ctx.strokeStyle="#d9e2ef";
    ctx.lineWidth=1;
    for(let i=0;i<=4;i++){
      const y=T+(PH/4)*i;
      ctx.beginPath();
      ctx.moveTo(L,y);
      ctx.lineTo(w-R,y);
      ctx.stroke();
    }

    ctx.fillStyle="#071d49";
    ctx.font="bold 14px Arial";
    ctx.fillText(title,L,16);

    const t0=data[0].t, t1=data[data.length-1].t || t0+1;
    let ymin=Infinity, ymax=-Infinity;

    data.forEach(p => series.forEach(s => {
      const v=p[s.key];
      if(v!==null && v!==undefined && !isNaN(v)){
        ymin=Math.min(ymin,v);
        ymax=Math.max(ymax,v);
      }
    }));

    if(ymin===Infinity){ymin=0;ymax=1;}
    if(ymax===ymin)ymax=ymin+1;
    ymin=Math.max(0, ymin-(ymax-ymin)*0.1);
    ymax=ymax+(ymax-ymin)*0.12;

    const X=t => L + ((t-t0)/(t1-t0 || 1))*PW;
    const Y=v => T + PH - ((v-ymin)/(ymax-ymin))*PH;

    series.forEach(s => {
      ctx.strokeStyle=s.color;
      ctx.lineWidth=2.3;
      ctx.beginPath();
      let started=false;

      data.forEach(p => {
        const v=p[s.key];
        if(v===null || v===undefined || isNaN(v)) return;
        const x=X(p.t), y=Y(v);
        if(!started){ctx.moveTo(x,y);started=true;}
        else ctx.lineTo(x,y);
      });

      ctx.stroke();
      ctx.fillStyle=s.color;
      ctx.fillText(s.label, 730, s.y);
    });

    ctx.fillStyle="#667085";
    ctx.font="12px Arial";
    for(let i=0;i<4;i++){
      const idx=Math.floor((data.length-1)*i/3);
      const p=data[idx];
      if(!p) continue;
      ctx.fillText(fmtTime(p.t), X(p.t)-28, h-15);
    }
  }

  function drawRadar(){
    if(!live || !el.radarCanvas) return;

    const c = el.radarCanvas;
    const ctx = c.getContext("2d");
    const w = c.width, h = c.height;
    const cx=w/2, cy=h-18, maxRange=live.radarMax || 250;
    const maxRadius=h-42;

    ctx.clearRect(0,0,w,h);
    ctx.fillStyle="#f8fbff";
    ctx.fillRect(0,0,w,h);

    ctx.strokeStyle="#d9e2ef";
    ctx.lineWidth=1;

    for(let r=50;r<=maxRange;r+=50){
      const rr=(r/maxRange)*maxRadius;
      ctx.beginPath();
      ctx.arc(cx,cy,rr,Math.PI,0);
      ctx.stroke();
      ctx.fillStyle="#667085";
      ctx.font="11px Arial";
      ctx.fillText(r+"cm",cx+rr-24,cy-5);
    }

    for(let a=0;a<=180;a+=30){
      const rad=(Math.PI*a)/180;
      const x=cx+Math.cos(Math.PI-rad)*maxRadius;
      const y=cy-Math.sin(rad)*maxRadius;
      ctx.beginPath();
      ctx.moveTo(cx,cy);
      ctx.lineTo(x,y);
      ctx.stroke();
    }

    ctx.strokeStyle="rgba(11,85,217,.75)";
    ctx.lineWidth=3;
    const sweep=(Date.now()/20)%180;
    const rad=(Math.PI*sweep)/180;
    ctx.beginPath();
    ctx.moveTo(cx,cy);
    ctx.lineTo(cx+Math.cos(Math.PI-rad)*maxRadius, cy-Math.sin(rad)*maxRadius);
    ctx.stroke();

    if(live.distanceCm > 0){
      const rr=(Math.min(live.distanceCm,maxRange)/maxRange)*maxRadius;
      ctx.fillStyle=live.radarObject ? "#dc3545" : "#198754";
      ctx.beginPath();
      ctx.arc(cx,cy-rr,8,0,Math.PI*2);
      ctx.fill();
    }

    ctx.fillStyle="#071d49";
    ctx.font="bold 13px Arial";
    ctx.fillText("Radar HC-SR04",14,20);
  }

  async function action(cmd){
    await fetch("/api/action?token="+TOKEN+"&cmd="+cmd+"&nocache="+Date.now());
    await pollOnce();
  }

  async function led(name,on){
    await fetch("/api/action?token="+TOKEN+"&cmd=led&led="+name+"&on="+on+"&nocache="+Date.now());
    await pollOnce();
  }

  async function saveConfig(mqttOn){
    const q = new URLSearchParams();
    q.set("token", TOKEN);
    q.set("tempLimit", el.tempLimit.value);
    q.set("humLow", el.humLow.value);
    q.set("humHigh", el.humHigh.value);
    q.set("gasLimit", el.gasLimit.value);
    q.set("radarLimit", el.radarLimit.value);
    q.set("mqtt", mqttOn ? "1" : "0");
    q.set("mqttBase", el.mqttBase.value);
    await fetch("/api/config?" + q.toString());
    await pollOnce();
  }

  async function pollOnce(){
    try{
      const res = await fetch("/api/live?nocache=" + Date.now());
      live = await res.json();
      updateUI(live);
      drawVisible();
    }catch(e){}
  }

  text(el.clock, fmtTime(Date.now()));
  setInterval(() => text(el.clock, fmtTime(Date.now())), 1000);
  poll();
  loadEvents();
  setTimeout(loadHistory, 1500);
})();
</script>
</body>
</html>
)rawliteral";
