#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <LittleFS.h>

// Le HTML est compilé séparément dans src/index.cpp
extern const char index_html[] PROGMEM;

// =====================================================
// STATION IoT ESP32 - VERSION OPTIMISÉE ESGI
// Objectif : moins de latence, code plus lisible, supervision CPU,
// graphiques plus légers côté navigateur.
// =====================================================
//
// Points importants pour le sujet :
// - loop() vide
// - aucune logique métier dans loop()
// - FreeRTOS avec tâches séparées
// - Queue entre TaskSensors et TaskMQTT
// - Mutex seulement pour l'état partagé
// - MQTT + mode offline/replay
// - supervision heap, uptime, WiFi, MQTT, latence, charge Core 0/Core 1
// =====================================================

// =======================
// Identifiants WiFi
// =======================
const char* ssid = "iPhone de Othmane";
const char* password = "othmane59";

// =======================
// Sécurité Web
// =======================
// Login de l'interface Web.
const char* WEB_USER = "admin";
const char* WEB_PASS = "esp32";

// Token utilisé pour les commandes sensibles côté API.
const char* API_TOKEN = "1234";

// =======================
// MQTT
// =======================
const char* mqttServer = "test.mosquitto.org";
const int mqttPort = 1883;
String mqttBaseTopic = "campus/groupe1/ESP32-Othmane";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool mqttEnabled = false;
bool mqttConnected = false;

unsigned long lastMqttReconnectAttemptMs = 0;
unsigned long lastMqttPublishMs = 0;
unsigned long lastMqttLatencyMs = 0;

uint32_t mqttOkCount = 0;
uint32_t mqttFailCount = 0;
uint32_t offlineStoredCount = 0;
uint32_t replayedCount = 0;

// Permet de simuler une panne MQTT sans couper le WiFi.
unsigned long mqttFaultUntilMs = 0;

// Permet de couper volontairement le WiFi depuis le site.
unsigned long wifiDisabledUntilMs = 0;

// =======================
// Capteurs
// =======================
#define DHT_TYPE DHT22
const int DHT_PIN = 4;
DHT dht(DHT_PIN, DHT_TYPE);

const int PIR_PIN = 26;
const int HW499_PIN = 33;
const int GAS_AO_PIN = 34;
const bool HW499_ACTIVE_LOW = true;

// Bouton physique : une patte GPIO25, autre patte GND.
const int BUTTON_PIN = 25;

// Joystick HW-504
const int JOY_X_PIN = 32;
const int JOY_Y_PIN = 35;
const int JOY_SW_PIN = 23;

// =======================
// Actionneurs
// =======================
const int RED_LED = 5;
const int BLUE_LED = 18;
const int GREEN_LED = 19;
const int YELLOW_LED = 21;
const int WHITE_LED = 22;

const int BUZZER_ALARM_PIN = 12;
const int BUZZER_MUSIC_PIN = 27;

const int LED_PINS[] = { RED_LED, BLUE_LED, GREEN_LED, YELLOW_LED, WHITE_LED };
const int LED_COUNT = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

// =======================
// Seuils
// =======================
float tempLimit = 35.0;
float humLowLimit = 30.0;
float humHighLimit = 75.0;
int gasLimitRaw = 2500;

// =======================
// Mesure partagée pour Web/Supervision
// =======================
struct SharedState {
  float temp = NAN;
  float humidity = NAN;
  int gasRaw = 0;
  float gasFiltered = -1;
  float gasPercent = 0;

  bool pir = false;
  bool hw499 = false;
  bool buttonPressed = false;

  int joyXRaw = 2048;
  int joyYRaw = 2048;
  int joyX = 0;
  int joyY = 0;
  bool joyButton = false;
  uint32_t joyButtonCount = 0;
  String joyDirection = "CENTER";

  bool dhtOk = false;
  bool gasOk = true;

  bool securityEnabled = false;
  bool intrusionLatched = false;

  int riskScore = 0;
  String riskState = "NOMINAL";

  uint32_t pirCount = 0;
  uint32_t hw499Count = 0;
  uint32_t intrusionCount = 0;

  uint32_t sequence = 0;

  float tempMin = NAN;
  float tempMax = NAN;
  float humMin = NAN;
  float humMax = NAN;
  int gasMax = 0;
};

SharedState state;
SemaphoreHandle_t stateMutex;

// =======================
// Queue capteurs -> MQTT
// =======================
struct SensorPacket {
  uint32_t seq;
  uint32_t uptimeMs;
  float temp;
  float humidity;
  int gasRaw;
  float gasPercent;
  bool pir;
  bool hw499;
  bool dhtOk;
  bool wifiOk;
  int riskScore;
  char riskState[16];
};

QueueHandle_t sensorQueue;

// =======================
// Historique événement RAM
// =======================
const int EVENT_COUNT = 16;

struct EventItem {
  uint32_t uptimeSec;
  char type[16];
  char level[16];
  char message[72];
};

EventItem events[EVENT_COUNT];
int eventWriteIndex = 0;
int eventTotal = 0;

// =======================
// Supervision CPU estimée
// =======================
// On mesure le temps actif passé dans les tâches.
// Cela donne une estimation suffisante pour la soutenance.
portMUX_TYPE metricsMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t core0BusyUs = 0;
volatile uint32_t core1BusyUs = 0;

float core0Load = 0.0;
float core1Load = 0.0;

uint32_t taskSensorsLoops = 0;
uint32_t taskMqttLoops = 0;
uint32_t taskSupervisionLoops = 0;
uint32_t taskWebLoops = 0;
uint32_t taskLightsLoops = 0;

uint32_t queueDrops = 0;

const char* OFFLINE_FILE = "/offline.jsonl";
const char* HISTORY_FILE = "/history.jsonl";
const size_t MAX_HISTORY_BYTES = 70000;
const size_t MAX_OFFLINE_BYTES = 70000;

// =====================================================
// Utilitaires basiques
// =====================================================
String boolJson(bool value) {
  return value ? "true" : "false";
}

String floatJson(float value, int decimals = 1) {
  if (isnan(value)) return "null";
  return String(value, decimals);
}

String escapeJson(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", " ");
  s.replace("\r", " ");
  return s;
}

bool validToken(AsyncWebServerRequest* request) {
  return request->hasParam("token") && request->getParam("token")->value() == API_TOKEN;
}

bool authenticated(AsyncWebServerRequest* request) {
  if (request->authenticate(WEB_USER, WEB_PASS)) return true;

  request->requestAuthentication();
  return false;
}

void recordBusy(uint8_t core, uint32_t startUs) {
  uint32_t elapsed = micros() - startUs;

  portENTER_CRITICAL(&metricsMux);

  if (core == 0) {
    core0BusyUs += elapsed;
  } else {
    core1BusyUs += elapsed;
  }

  portEXIT_CRITICAL(&metricsMux);
}

void addEvent(const char* type, const char* level, const char* message) {
  EventItem& ev = events[eventWriteIndex];

  ev.uptimeSec = millis() / 1000;

  strncpy(ev.type, type, sizeof(ev.type) - 1);
  ev.type[sizeof(ev.type) - 1] = 0;

  strncpy(ev.level, level, sizeof(ev.level) - 1);
  ev.level[sizeof(ev.level) - 1] = 0;

  strncpy(ev.message, message, sizeof(ev.message) - 1);
  ev.message[sizeof(ev.message) - 1] = 0;

  eventWriteIndex = (eventWriteIndex + 1) % EVENT_COUNT;
  if (eventTotal < EVENT_COUNT) eventTotal++;
}

// =====================================================
// LittleFS : base locale
// =====================================================
void ensureFileLimit(const char* path, size_t maxBytes) {
  if (!LittleFS.exists(path)) return;

  File f = LittleFS.open(path, "r");
  if (!f) return;

  size_t size = f.size();
  f.close();

  // Version simple et robuste : si le fichier devient trop gros,
  // on le purge. Pour un projet plus avancé, on ferait une rotation.
  if (size > maxBytes) {
    LittleFS.remove(path);
    addEvent("BDD", "WARNING", "Fichier local purgé car trop volumineux");
  }
}

void appendLine(const char* path, const String& line, size_t maxBytes) {
  ensureFileLimit(path, maxBytes);

  File f = LittleFS.open(path, "a");
  if (!f) return;

  f.println(line);
  f.close();
}

String packetToJson(const SensorPacket& p, bool replayed = false) {
  String json = "{";
  json += "\"device\":\"ESP32-Othmane\",";
  json += "\"seq\":" + String(p.seq) + ",";
  json += "\"createdMs\":" + String(p.uptimeMs) + ",";
  json += "\"temp\":" + floatJson(p.temp, 1) + ",";
  json += "\"humidity\":" + floatJson(p.humidity, 1) + ",";
  json += "\"gasRaw\":" + String(p.gasRaw) + ",";
  json += "\"gasPercent\":" + String(p.gasPercent, 1) + ",";
  json += "\"pir\":" + boolJson(p.pir) + ",";
  json += "\"hw499\":" + boolJson(p.hw499) + ",";
  json += "\"dhtOk\":" + boolJson(p.dhtOk) + ",";
  json += "\"wifi\":" + boolJson(p.wifiOk) + ",";
  json += "\"riskScore\":" + String(p.riskScore) + ",";
  json += "\"riskState\":\"" + String(p.riskState) + "\",";
  json += "\"replayed\":" + boolJson(replayed);
  json += "}";

  return json;
}

String readTailJsonlAsArray(const char* path, int maxLines) {
  if (!LittleFS.exists(path)) return "[]";

  File f = LittleFS.open(path, "r");
  if (!f) return "[]";

  String buffer[80];
  int count = 0;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) continue;

    buffer[count % maxLines] = line;
    count++;
  }

  f.close();

  int available = min(count, maxLines);
  int start = count > maxLines ? count % maxLines : 0;

  String json = "[";

  for (int i = 0; i < available; i++) {
    int index = (start + i) % maxLines;
    if (i > 0) json += ",";
    json += buffer[index];
  }

  json += "]";
  return json;
}

// =====================================================
// Alertes et risque
// =====================================================
int computeRisk(float temp, float hum, int gasRaw, bool pir, bool hw499, bool dhtOk) {
  int risk = 0;

  if (!dhtOk) risk += 25;
  if (dhtOk && temp >= tempLimit) risk += 25;
  if (dhtOk && (hum < humLowLimit || hum > humHighLimit)) risk += 15;
  if (gasRaw >= gasLimitRaw) risk += 35;
  if (pir && state.securityEnabled) risk += 30;
  else if (pir) risk += 10;
  if (hw499) risk += 10;
  if (WiFi.status() != WL_CONNECTED) risk += 10;
  if (state.intrusionLatched) risk += 25;

  if (risk > 100) risk = 100;
  return risk;
}

const char* riskName(int score, bool dhtOk) {
  if (!dhtOk) return "CAPTEUR";
  if (score >= 80) return "CRITIQUE";
  if (score >= 55) return "DANGER";
  if (score >= 30) return "ATTENTION";
  if (score >= 10) return "INFO";
  return "NORMAL";
}

void updateMinMax(float temp, float hum, int gasRaw) {
  if (!isnan(temp)) {
    if (isnan(state.tempMin) || temp < state.tempMin) state.tempMin = temp;
    if (isnan(state.tempMax) || temp > state.tempMax) state.tempMax = temp;
  }

  if (!isnan(hum)) {
    if (isnan(state.humMin) || hum < state.humMin) state.humMin = hum;
    if (isnan(state.humMax) || hum > state.humMax) state.humMax = hum;
  }

  if (gasRaw > state.gasMax) state.gasMax = gasRaw;
}

// =====================================================
// Joystick HW-504
// =====================================================
void readJoystickUnsafe() {
  int xRaw = analogRead(JOY_X_PIN);
  int yRaw = analogRead(JOY_Y_PIN);
  bool button = digitalRead(JOY_SW_PIN) == LOW;

  int x = 0;
  int y = 0;

  if (xRaw < 1300) x = -1;
  else if (xRaw > 2800) x = 1;

  if (yRaw < 1300) y = -1;
  else if (yRaw > 2800) y = 1;

  static bool previousButton = false;

  if (button && !previousButton) {
    state.joyButtonCount++;
  }

  previousButton = button;

  state.joyXRaw = xRaw;
  state.joyYRaw = yRaw;
  state.joyX = x;
  state.joyY = y;
  state.joyButton = button;

  if (x == 0 && y == 0) state.joyDirection = "CENTER";
  else if (abs(x) > abs(y)) state.joyDirection = x > 0 ? "RIGHT" : "LEFT";
  else state.joyDirection = y > 0 ? "DOWN" : "UP";
}

// =====================================================
// LED / buzzer minimaliste pour réduire la charge
// =====================================================
void allLedsOff() {
  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}

void applyLedsUnsafe() {
  bool wifiOk = WiFi.status() == WL_CONNECTED;
  bool danger = state.riskScore >= 55 || state.intrusionLatched;
  bool warning = state.riskScore >= 30 && !danger;

  digitalWrite(GREEN_LED, wifiOk ? HIGH : LOW);
  digitalWrite(RED_LED, danger ? HIGH : LOW);
  digitalWrite(YELLOW_LED, warning ? HIGH : LOW);
  digitalWrite(BLUE_LED, (!danger && !warning) ? HIGH : LOW);
  digitalWrite(WHITE_LED, state.securityEnabled ? HIGH : LOW);
}

void alarmOn(bool enabled) {
  if (enabled) {
    tone(BUZZER_ALARM_PIN, 900);
  } else {
    noTone(BUZZER_ALARM_PIN);
  }
}

// =====================================================
// MQTT
// =====================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;

  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  msg.trim();

  xSemaphoreTake(stateMutex, portMAX_DELAY);

  if (msg == "securityOn") {
    state.securityEnabled = true;
    addEvent("MQTT", "INFO", "Sécurité activée via MQTT");
  } else if (msg == "securityOff") {
    state.securityEnabled = false;
    state.intrusionLatched = false;
    addEvent("MQTT", "INFO", "Sécurité désactivée via MQTT");
  } else if (msg == "ack") {
    state.intrusionLatched = false;
    addEvent("MQTT", "INFO", "Alarme acquittée via MQTT");
  }

  xSemaphoreGive(stateMutex);
}

bool mqttReconnect() {
  if (!mqttEnabled) return false;
  if (WiFi.status() != WL_CONNECTED) return false;
  if (millis() < mqttFaultUntilMs) return false;

  uint64_t chipid = ESP.getEfuseMac();

  char clientId[48];
  snprintf(clientId, sizeof(clientId), "ESP32-Othmane-%04X", (uint16_t)(chipid & 0xFFFF));

  if (mqttClient.connect(clientId)) {
    String cmdTopic = mqttBaseTopic + "/cmd";
    mqttClient.subscribe(cmdTopic.c_str());

    mqttConnected = true;

    xSemaphoreTake(stateMutex, portMAX_DELAY);
    addEvent("MQTT", "INFO", "MQTT connecté");
    xSemaphoreGive(stateMutex);

    return true;
  }

  mqttConnected = false;
  return false;
}

bool mqttPublishLine(const String& line, bool replayed = false) {
  if (!mqttEnabled) return false;
  if (WiFi.status() != WL_CONNECTED) return false;
  if (millis() < mqttFaultUntilMs) return false;
  if (!mqttClient.connected()) return false;

  String topic = mqttBaseTopic + "/data";

  unsigned long start = millis();
  bool ok = mqttClient.publish(topic.c_str(), line.c_str());
  lastMqttLatencyMs = millis() - start;

  if (ok) {
    mqttOkCount++;
    if (replayed) replayedCount++;
    lastMqttPublishMs = millis();
  } else {
    mqttFailCount++;
  }

  return ok;
}

void replayOfflineFile() {
  if (!LittleFS.exists(OFFLINE_FILE)) return;
  if (!mqttClient.connected()) return;

  File f = LittleFS.open(OFFLINE_FILE, "r");
  if (!f) return;

  bool allSent = true;
  int sentNow = 0;

  while (f.available() && sentNow < 8) {
    String line = f.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) continue;

    if (!mqttPublishLine(line, true)) {
      allSent = false;
      break;
    }

    sentNow++;
  }

  f.close();

  // Version simple : si on a pu tout lire/envoyer, on supprime.
  // Si la connexion retombe pendant le replay, on garde le fichier.
  if (allSent) {
    LittleFS.remove(OFFLINE_FILE);
    offlineStoredCount = 0;

    xSemaphoreTake(stateMutex, portMAX_DELAY);
    addEvent("MQTT", "INFO", "Replay offline terminé");
    xSemaphoreGive(stateMutex);
  }
}

// =====================================================
// JSON API
// =====================================================
String buildLiveJson() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);

  UBaseType_t queueWaiting = uxQueueMessagesWaiting(sensorQueue);
  UBaseType_t queueSpaces = uxQueueSpacesAvailable(sensorQueue);

  bool wifiOk = WiFi.status() == WL_CONNECTED;
  bool mqttOk = mqttClient.connected() && mqttConnected && mqttEnabled;

  String json = "{";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"temp\":" + floatJson(state.temp, 1) + ",";
  json += "\"humidity\":" + floatJson(state.humidity, 1) + ",";
  json += "\"gasRaw\":" + String(state.gasRaw) + ",";
  json += "\"gasPercent\":" + String(state.gasPercent, 1) + ",";
  json += "\"pir\":" + boolJson(state.pir) + ",";
  json += "\"hw499\":" + boolJson(state.hw499) + ",";
  json += "\"button\":" + boolJson(state.buttonPressed) + ",";
  json += "\"dhtOk\":" + boolJson(state.dhtOk) + ",";
  json += "\"gasOk\":" + boolJson(state.gasOk) + ",";
  json += "\"riskScore\":" + String(state.riskScore) + ",";
  json += "\"riskState\":\"" + state.riskState + "\",";
  json += "\"security\":" + boolJson(state.securityEnabled) + ",";
  json += "\"intrusion\":" + boolJson(state.intrusionLatched) + ",";
  json += "\"pirCount\":" + String(state.pirCount) + ",";
  json += "\"hw499Count\":" + String(state.hw499Count) + ",";
  json += "\"intrusionCount\":" + String(state.intrusionCount) + ",";
  json += "\"tempMin\":" + floatJson(state.tempMin, 1) + ",";
  json += "\"tempMax\":" + floatJson(state.tempMax, 1) + ",";
  json += "\"humMin\":" + floatJson(state.humMin, 1) + ",";
  json += "\"humMax\":" + floatJson(state.humMax, 1) + ",";
  json += "\"gasMax\":" + String(state.gasMax) + ",";
  json += "\"wifi\":" + boolJson(wifiOk) + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"mqttEnabled\":" + boolJson(mqttEnabled) + ",";
  json += "\"mqttConnected\":" + boolJson(mqttOk) + ",";
  json += "\"mqttLatency\":" + String(lastMqttLatencyMs) + ",";
  json += "\"mqttOkCount\":" + String(mqttOkCount) + ",";
  json += "\"mqttFailCount\":" + String(mqttFailCount) + ",";
  json += "\"offlineCount\":" + String(offlineStoredCount) + ",";
  json += "\"replayedCount\":" + String(replayedCount) + ",";
  json += "\"mqttFaultRemaining\":" + String(millis() < mqttFaultUntilMs ? (mqttFaultUntilMs - millis()) / 1000 : 0) + ",";
  json += "\"wifiOffRemaining\":" + String(millis() < wifiDisabledUntilMs ? (wifiDisabledUntilMs - millis()) / 1000 : 0) + ",";
  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"queueWaiting\":" + String(queueWaiting) + ",";
  json += "\"queueSpaces\":" + String(queueSpaces) + ",";
  json += "\"queueDrops\":" + String(queueDrops) + ",";
  json += "\"core0Load\":" + String(core0Load, 1) + ",";
  json += "\"core1Load\":" + String(core1Load, 1) + ",";
  json += "\"loopsSensors\":" + String(taskSensorsLoops) + ",";
  json += "\"loopsMqtt\":" + String(taskMqttLoops) + ",";
  json += "\"loopsSupervision\":" + String(taskSupervisionLoops) + ",";
  json += "\"joyX\":" + String(state.joyX) + ",";
  json += "\"joyY\":" + String(state.joyY) + ",";
  json += "\"joyButton\":" + boolJson(state.joyButton) + ",";
  json += "\"joyDirection\":\"" + state.joyDirection + "\"";
  json += "}";

  xSemaphoreGive(stateMutex);

  return json;
}

String buildEventsJson() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);

  String json = "[";

  for (int i = 0; i < eventTotal; i++) {
    int index = eventWriteIndex - 1 - i;
    if (index < 0) index += EVENT_COUNT;

    if (i > 0) json += ",";

    json += "{";
    json += "\"ts\":" + String(events[index].uptimeSec) + ",";
    json += "\"type\":\"" + String(events[index].type) + "\",";
    json += "\"level\":\"" + String(events[index].level) + "\",";
    json += "\"message\":\"" + escapeJson(String(events[index].message)) + "\"";
    json += "}";
  }

  json += "]";

  xSemaphoreGive(stateMutex);
  return json;
}

// =====================================================
// Serveur Web
// =====================================================
void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!authenticated(request)) return;
    request->send(200, "text/html", index_html);
  });

  server.on("/api/live", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!authenticated(request)) return;
    request->send(200, "application/json", buildLiveJson());
  });

  server.on("/api/events", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!authenticated(request)) return;
    request->send(200, "application/json", buildEventsJson());
  });

  server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!authenticated(request)) return;

    int limit = 60;
    if (request->hasParam("limit")) {
      limit = request->getParam("limit")->value().toInt();
      if (limit < 10) limit = 10;
      if (limit > 80) limit = 80;
    }

    request->send(200, "application/json", readTailJsonlAsArray(HISTORY_FILE, limit));
  });

  server.on("/api/joystick", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!authenticated(request)) return;

    xSemaphoreTake(stateMutex, portMAX_DELAY);

    String json = "{";
    json += "\"xRaw\":" + String(state.joyXRaw) + ",";
    json += "\"yRaw\":" + String(state.joyYRaw) + ",";
    json += "\"x\":" + String(state.joyX) + ",";
    json += "\"y\":" + String(state.joyY) + ",";
    json += "\"button\":" + boolJson(state.joyButton) + ",";
    json += "\"buttonCount\":" + String(state.joyButtonCount) + ",";
    json += "\"direction\":\"" + state.joyDirection + "\"";
    json += "}";

    xSemaphoreGive(stateMutex);

    request->send(200, "application/json", json);
  });

  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!authenticated(request)) return;
    if (!validToken(request)) {
      request->send(403, "application/json", "{\"ok\":false,\"error\":\"bad token\"}");
      return;
    }

    if (request->hasParam("tempLimit")) tempLimit = request->getParam("tempLimit")->value().toFloat();
    if (request->hasParam("humLow")) humLowLimit = request->getParam("humLow")->value().toFloat();
    if (request->hasParam("humHigh")) humHighLimit = request->getParam("humHigh")->value().toFloat();
    if (request->hasParam("gasLimit")) gasLimitRaw = request->getParam("gasLimit")->value().toInt();
    if (request->hasParam("mqtt")) mqttEnabled = request->getParam("mqtt")->value().toInt() == 1;
    if (request->hasParam("mqttBase")) mqttBaseTopic = request->getParam("mqttBase")->value();

    xSemaphoreTake(stateMutex, portMAX_DELAY);
    addEvent("CONFIG", "INFO", "Configuration mise à jour");
    xSemaphoreGive(stateMutex);

    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/action", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!authenticated(request)) return;
    if (!validToken(request)) {
      request->send(403, "application/json", "{\"ok\":false,\"error\":\"bad token\"}");
      return;
    }

    String cmd = request->hasParam("cmd") ? request->getParam("cmd")->value() : "";

    xSemaphoreTake(stateMutex, portMAX_DELAY);

    if (cmd == "securityOn") {
      state.securityEnabled = true;
      addEvent("SECURITE", "INFO", "Mode sécurité activé");
    } else if (cmd == "securityOff") {
      state.securityEnabled = false;
      state.intrusionLatched = false;
      addEvent("SECURITE", "INFO", "Mode sécurité désactivé");
    } else if (cmd == "ack") {
      state.intrusionLatched = false;
      addEvent("ALARME", "INFO", "Alarme acquittée");
    } else if (cmd == "mqttOn") {
      mqttEnabled = true;
      addEvent("MQTT", "INFO", "MQTT activé");
    } else if (cmd == "mqttOff") {
      mqttEnabled = false;
      mqttConnected = false;
      mqttClient.disconnect();
      addEvent("MQTT", "WARNING", "MQTT désactivé");
    } else if (cmd == "mqttFault5") {
      mqttFaultUntilMs = millis() + 5UL * 60UL * 1000UL;
      mqttClient.disconnect();
      addEvent("MQTT", "WARNING", "Panne MQTT simulée 5 min");
    } else if (cmd == "mqttFault30") {
      mqttFaultUntilMs = millis() + 30UL * 60UL * 1000UL;
      mqttClient.disconnect();
      addEvent("MQTT", "WARNING", "Panne MQTT simulée 30 min");
    } else if (cmd == "mqttFault60") {
      mqttFaultUntilMs = millis() + 60UL * 60UL * 1000UL;
      mqttClient.disconnect();
      addEvent("MQTT", "WARNING", "Panne MQTT simulée 1 h");
    } else if (cmd == "mqttFaultOff") {
      mqttFaultUntilMs = 0;
      addEvent("MQTT", "INFO", "Simulation panne MQTT arrêtée");
    } else if (cmd == "wifiOff60") {
      wifiDisabledUntilMs = millis() + 60UL * 1000UL;
      WiFi.disconnect(true);
      addEvent("WIFI", "WARNING", "WiFi coupé 1 min");
    } else if (cmd == "wifiOff30") {
      wifiDisabledUntilMs = millis() + 30UL * 60UL * 1000UL;
      WiFi.disconnect(true);
      addEvent("WIFI", "WARNING", "WiFi coupé 30 min");
    } else if (cmd == "wifiOn") {
      wifiDisabledUntilMs = 0;
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      addEvent("WIFI", "INFO", "Reconnexion WiFi demandée");
    } else if (cmd == "clearDb") {
      LittleFS.remove(OFFLINE_FILE);
      LittleFS.remove(HISTORY_FILE);
      offlineStoredCount = 0;
      addEvent("BDD", "WARNING", "Base locale vidée");
    } else if (cmd == "resetStats") {
      state.tempMin = NAN;
      state.tempMax = NAN;
      state.humMin = NAN;
      state.humMax = NAN;
      state.gasMax = 0;
      state.pirCount = 0;
      state.hw499Count = 0;
      state.intrusionCount = 0;
      queueDrops = 0;
      addEvent("SYSTEME", "INFO", "Statistiques remises à zéro");
    }

    xSemaphoreGive(stateMutex);

    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
}

// =====================================================
// Tâches FreeRTOS
// =====================================================

// Priorité 3 / Core 1
// Rôle : lecture capteurs et création des paquets.
// Priorité la plus haute car l'acquisition ne doit pas dépendre du réseau.
void taskSensors(void* parameter) {
  TickType_t lastWake = xTaskGetTickCount();

  bool previousPir = false;
  bool previousHw = false;
  bool previousButton = false;

  for (;;) {
    uint32_t start = micros();

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    bool dhtOk = !isnan(temp) && !isnan(hum);

    int gasRawLocal = analogRead(GAS_AO_PIN);
    bool pir = digitalRead(PIR_PIN) == HIGH;

    int hwRaw = digitalRead(HW499_PIN);
    bool hw = HW499_ACTIVE_LOW ? hwRaw == LOW : hwRaw == HIGH;

    bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;

    xSemaphoreTake(stateMutex, portMAX_DELAY);

    state.sequence++;

    if (dhtOk) {
      state.temp = temp;
      state.humidity = hum;
    }

    state.dhtOk = dhtOk;
    state.gasRaw = gasRawLocal;

    if (state.gasFiltered < 0) state.gasFiltered = gasRawLocal;
    else state.gasFiltered = state.gasFiltered * 0.85 + gasRawLocal * 0.15;

    state.gasPercent = (state.gasFiltered / 4095.0) * 100.0;

    // Détection simple capteur gaz suspect.
    state.gasOk = !(gasRawLocal < 5 || gasRawLocal > 4090);

    state.pir = pir;
    state.hw499 = hw;
    state.buttonPressed = buttonPressed;

    readJoystickUnsafe();

    if (pir && !previousPir) {
      state.pirCount++;
      addEvent("PIR", "INFO", "Mouvement détecté");

      if (state.securityEnabled) {
        state.intrusionLatched = true;
        state.intrusionCount++;
        addEvent("SECURITE", "CRITIQUE", "Intrusion détectée");
      }
    }

    if (hw && !previousHw) {
      state.hw499Count++;
      addEvent("HW499", "INFO", "Événement HW-499 détecté");
    }

    if (buttonPressed && !previousButton) {
      if (state.intrusionLatched) {
        state.intrusionLatched = false;
        addEvent("BOUTON", "INFO", "Alarme acquittée par bouton");
      } else {
        state.securityEnabled = !state.securityEnabled;
        addEvent("BOUTON", "INFO", state.securityEnabled ? "Sécurité activée" : "Sécurité désactivée");
      }
    }

    previousPir = pir;
    previousHw = hw;
    previousButton = buttonPressed;

    updateMinMax(state.temp, state.humidity, gasRawLocal);

    state.riskScore = computeRisk(state.temp, state.humidity, gasRawLocal, pir, hw, dhtOk);
    state.riskState = riskName(state.riskScore, dhtOk);

    SensorPacket p;
    p.seq = state.sequence;
    p.uptimeMs = millis();
    p.temp = state.temp;
    p.humidity = state.humidity;
    p.gasRaw = gasRawLocal;
    p.gasPercent = state.gasPercent;
    p.pir = pir;
    p.hw499 = hw;
    p.dhtOk = dhtOk;
    p.wifiOk = WiFi.status() == WL_CONNECTED;
    p.riskScore = state.riskScore;
    strncpy(p.riskState, state.riskState.c_str(), sizeof(p.riskState) - 1);
    p.riskState[sizeof(p.riskState) - 1] = 0;

    xSemaphoreGive(stateMutex);

    if (xQueueSend(sensorQueue, &p, 0) != pdTRUE) {
      queueDrops++;
    }

    taskSensorsLoops++;
    recordBusy(1, start);

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(2000));
  }
}

// Priorité 2 / Core 0
// Rôle : publication MQTT, stockage offline, replay.
// Le réseau peut attendre, donc priorité plus faible que les capteurs.
void taskMqtt(void* parameter) {
  for (;;) {
    uint32_t start = micros();

    if (!mqttEnabled) {
      mqttConnected = false;
      if (mqttClient.connected()) mqttClient.disconnect();
    } else {
      if (!mqttClient.connected()) {
        unsigned long now = millis();

        if (now - lastMqttReconnectAttemptMs > 5000) {
          lastMqttReconnectAttemptMs = now;
          mqttReconnect();
        }
      } else {
        mqttClient.loop();
        replayOfflineFile();
      }
    }

    SensorPacket p;

    if (xQueueReceive(sensorQueue, &p, pdMS_TO_TICKS(500)) == pdTRUE) {
      String line = packetToJson(p, false);

      // Historique local pour les graphes, même si MQTT fonctionne.
      appendLine(HISTORY_FILE, line, MAX_HISTORY_BYTES);

      bool sent = mqttPublishLine(line, false);

      if (!sent) {
        appendLine(OFFLINE_FILE, line, MAX_OFFLINE_BYTES);
        offlineStoredCount++;
      }
    }

    taskMqttLoops++;
    recordBusy(0, start);

    vTaskDelay(pdMS_TO_TICKS(150));
  }
}

// Priorité 2 / Core 1
// Rôle : supervision et sécurité.
// Surveille l'état global et pilote buzzer/LED.
void taskSupervision(void* parameter) {
  for (;;) {
    uint32_t start = micros();

    xSemaphoreTake(stateMutex, portMAX_DELAY);

    bool danger = state.riskScore >= 55 || state.intrusionLatched;
    bool wifiOk = WiFi.status() == WL_CONNECTED;

    applyLedsUnsafe();
    alarmOn(danger);

    static bool previousWifi = true;
    if (wifiOk != previousWifi) {
      addEvent("WIFI", wifiOk ? "INFO" : "WARNING", wifiOk ? "WiFi reconnecté" : "WiFi déconnecté");
      previousWifi = wifiOk;
    }

    xSemaphoreGive(stateMutex);

    taskSupervisionLoops++;
    recordBusy(1, start);

    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

// Priorité 1 / Core 0
// Rôle : reconnexion WiFi.
// Faible priorité car le système doit continuer sans réseau.
void taskWiFi(void* parameter) {
  for (;;) {
    uint32_t start = micros();

    if (millis() < wifiDisabledUntilMs) {
      if (WiFi.status() == WL_CONNECTED) WiFi.disconnect(true);
    } else {
      if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
      }
    }

    recordBusy(0, start);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// Priorité 1 / Core 0
// Rôle : heartbeat Web.
// Le serveur AsyncWebServer travaille surtout par callbacks, donc cette tâche reste légère.
void taskWeb(void* parameter) {
  for (;;) {
    uint32_t start = micros();

    taskWebLoops++;

    recordBusy(0, start);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Priorité 1 / Core 0
// Rôle : calcul périodique de charge CPU estimée.
void taskCpuMonitor(void* parameter) {
  for (;;) {
    portENTER_CRITICAL(&metricsMux);

    uint32_t c0 = core0BusyUs;
    uint32_t c1 = core1BusyUs;
    core0BusyUs = 0;
    core1BusyUs = 0;

    portEXIT_CRITICAL(&metricsMux);

    // Fenêtre de 1 seconde : 1 000 000 us = 100 %
    core0Load = min(100.0f, (c0 / 1000000.0f) * 100.0f);
    core1Load = min(100.0f, (c1 / 1000000.0f) * 100.0f);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Priorité 1 / Core 0
// Rôle : logs série. Utile en débogage, mais non prioritaire.
void taskSystemLog(void* parameter) {
  for (;;) {
    Serial.println("===== Station IoT ESGI optimisée =====");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("MQTT: ");
    Serial.println(mqttConnected ? "connecte" : "off/deconnecte");
    Serial.print("Core0 load: ");
    Serial.print(core0Load);
    Serial.print("% | Core1 load: ");
    Serial.print(core1Load);
    Serial.println("%");
    Serial.print("Heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.println("======================================");

    vTaskDelay(pdMS_TO_TICKS(8000));
  }
}

// =====================================================
// Setup
// =====================================================
void setup() {
  Serial.begin(115200);

  stateMutex = xSemaphoreCreateMutex();
  sensorQueue = xQueueCreate(12, sizeof(SensorPacket));

  dht.begin();

  pinMode(PIR_PIN, INPUT);
  pinMode(HW499_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(JOY_SW_PIN, INPUT_PULLUP);

  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(WHITE_LED, OUTPUT);

  pinMode(BUZZER_ALARM_PIN, OUTPUT);
  pinMode(BUZZER_MUSIC_PIN, OUTPUT);

  allLedsOff();

  analogReadResolution(12);
  analogSetPinAttenuation(GAS_AO_PIN, ADC_11db);
  analogSetPinAttenuation(JOY_X_PIN, ADC_11db);
  analogSetPinAttenuation(JOY_Y_PIN, ADC_11db);

  if (!LittleFS.begin(true)) {
    Serial.println("Erreur LittleFS");
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(1200);

  setupWebServer();

  xSemaphoreTake(stateMutex, portMAX_DELAY);
  addEvent("SYSTEME", "INFO", "Station IoT optimisée démarrée");
  xSemaphoreGive(stateMutex);

  // Répartition volontaire :
  // Core 1 : acquisition + supervision temps réel.
  // Core 0 : réseau, MQTT, Web, logs.
  xTaskCreatePinnedToCore(taskSensors, "TaskSensors", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(taskMqtt, "TaskMQTT", 6144, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(taskSupervision, "TaskSupervision", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskWiFi, "TaskWiFi", 3072, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskWeb, "TaskWeb", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskCpuMonitor, "TaskCpuMonitor", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskSystemLog, "TaskSystemLog", 3072, NULL, 1, NULL, 0);

  Serial.println("Setup termine : loop vide, FreeRTOS actif.");
}

// =====================================================
// loop vide : exigence du projet
// =====================================================
void loop() {
}
