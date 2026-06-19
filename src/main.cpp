#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <LittleFS.h>

extern const char index_html[] PROGMEM;

// =====================================================
// STATION IoT ESP32 - VERSION BALANCED CORE
// -----------------------------------------------------
// Objectif : meilleure répartition Core 0 / Core 1,
// site très simple, toutes les fonctionnalités utiles gardées.
// =====================================================
//
// Core 0 : réseau, WiFi, MQTT, Web, LittleFS, logs
// Core 1 : capteurs, HC-SR04, joystick, LEDs, buzzer, supervision
//
// Priorités :
// TaskSensors      prio 3 / Core 1  -> acquisition mesures
// TaskActuators    prio 2 / Core 1  -> LEDs + buzzer
// TaskMqtt         prio 2 / Core 0  -> MQTT + offline/replay
// TaskWiFi         prio 1 / Core 0  -> reconnexion WiFi
// TaskCpuMonitor   prio 1 / Core 0  -> charge CPU estimée
// TaskLog          prio 1 / Core 0  -> logs série
//
// loop() reste vide.
// =====================================================

// =======================
// WiFi
// =======================
const char* ssid = "iPhone de Othmane";
const char* password = "othmane59";

// =======================
// Web security
// =======================
const char* WEB_USER = "admin";
const char* WEB_PASS = "esp32";
const char* API_TOKEN = "1234";

// =======================
// MQTT
// =======================
const char* mqttServer = "test.mosquitto.org";
const int mqttPort = 1883;
String mqttBaseTopic = "campus/groupe1/ESP32-Othmane";

WiFiClient espClient;
PubSubClient mqttClient(espClient);
AsyncWebServer server(80);

bool mqttEnabled = false;
bool mqttConnected = false;
unsigned long lastMqttReconnectAttemptMs = 0;
unsigned long lastMqttLatencyMs = 0;
unsigned long mqttFaultUntilMs = 0;
unsigned long wifiDisabledUntilMs = 0;

uint32_t mqttOkCount = 0;
uint32_t mqttFailCount = 0;
uint32_t offlineStoredCount = 0;
uint32_t replayedCount = 0;

// =======================
// Capteurs
// =======================
#define DHT_TYPE DHT22
const int DHT_PIN = 4;
DHT dht(DHT_PIN, DHT_TYPE);

const int GAS_AO_PIN = 34;
const int HW499_PIN = 33;
const bool HW499_ACTIVE_LOW = true;

const int BUTTON_PIN = 25;

// Joystick HW-504
const int JOY_X_PIN = 32;
const int JOY_Y_PIN = 35;
const int JOY_SW_PIN = 23;

// HC-SR04
const int HCSR04_ECHO_PIN = 26;   // ancien fil PIR OUT
const int HCSR04_TRIG_PIN = 14;   // nouveau fil TRIG

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

// Buzzer en LEDC initialisé proprement.
// Cela corrige les erreurs répétées :
// E ledc: ledc_set_duty(...) LEDC is not initialized
const int BUZZER_ALARM_CH = 0;
const int BUZZER_MUSIC_CH = 1;
bool alarmToneActive = false;

// =======================
// Seuils
// =======================
float tempLimit = 35.0;
float humLowLimit = 30.0;
float humHighLimit = 75.0;
int gasLimitRaw = 2500;
float radarLimitCm = 80.0;
float radarMaxCm = 250.0;

// =======================
// Modes
// =======================
bool manualLedMode = false;
bool soundEnabled = true;
bool securityEnabled = false;
bool intrusionLatched = false;

// Modes de démonstration
// Melody : petite séquence sonore avec les deux buzzers.
// Interruption critique : simule un incident industriel en forçant
// température, humidité et gaz à des valeurs dangereuses + sirène police.
bool melodyDemoActive = false;
bool criticalInterruptActive = false;
uint32_t melodyDemoUntilMs = 0;

// =======================
// Etat global partagé
// =======================
struct SharedState {
  uint32_t seq = 0;

  float temp = NAN;
  float humidity = NAN;
  bool dhtOk = false;

  int gasRaw = 0;
  float gasFiltered = -1;
  float gasPercent = 0;
  bool gasOk = true;

  float distanceCm = -1;
  bool radarOk = false;
  bool radarObject = false;
  uint32_t radarDetectCount = 0;

  bool hw499 = false;
  uint32_t hw499Count = 0;

  bool buttonPressed = false;

  int joyX = 0;
  int joyY = 0;
  bool joyButton = false;
  String joyDirection = "CENTER";

  bool securityEnabled = false;
  bool intrusionLatched = false;
  uint32_t intrusionCount = 0;

  int riskScore = 0;
  String riskState = "NORMAL";

  float tempMin = NAN;
  float tempMax = NAN;
  float humMin = NAN;
  float humMax = NAN;
  int gasMax = 0;
  float distanceMin = NAN;

  bool ledRed = false;
  bool ledBlue = false;
  bool ledGreen = false;
  bool ledYellow = false;
  bool ledWhite = false;
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
  bool dhtOk;

  int gasRaw;
  float gasPercent;

  float distanceCm;
  bool radarObject;
  bool radarOk;

  bool hw499;
  bool wifiOk;

  int riskScore;
  char riskState[16];
};

QueueHandle_t sensorQueue;
uint32_t queueDrops = 0;

// =======================
// Historique événements RAM
// =======================
const int EVENT_COUNT = 16;

struct EventItem {
  uint32_t uptimeSec;
  char type[16];
  char level[16];
  char message[88];
};

EventItem events[EVENT_COUNT];
int eventWriteIndex = 0;
int eventTotal = 0;

// =======================
// Supervision CPU estimée
// =======================
portMUX_TYPE metricsMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t core0BusyUs = 0;
volatile uint32_t core1BusyUs = 0;

float core0Load = 0;
float core1Load = 0;

uint32_t loopsSensors = 0;
uint32_t loopsActuators = 0;
uint32_t loopsMqtt = 0;
uint32_t loopsWiFi = 0;
uint32_t loopsCriticalInterrupt = 0;

// =======================
// LittleFS
// =======================
const char* OFFLINE_FILE = "/offline.jsonl";
const char* HISTORY_FILE = "/history.jsonl";
const size_t MAX_HISTORY_BYTES = 60000;
const size_t MAX_OFFLINE_BYTES = 60000;

// =====================================================
// Utilitaires
// =====================================================
String boolJson(bool v) {
  return v ? "true" : "false";
}

String floatJson(float v, int decimals = 1) {
  if (isnan(v)) return "null";
  return String(v, decimals);
}

String escapeJson(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", " ");
  s.replace("\r", " ");
  return s;
}

bool authenticated(AsyncWebServerRequest* request) {
  if (request->authenticate(WEB_USER, WEB_PASS)) return true;
  request->requestAuthentication();
  return false;
}

bool validToken(AsyncWebServerRequest* request) {
  return request->hasParam("token") && request->getParam("token")->value() == API_TOKEN;
}

void recordBusy(uint8_t core, uint32_t startUs) {
  uint32_t elapsed = micros() - startUs;

  portENTER_CRITICAL(&metricsMux);
  if (core == 0) core0BusyUs += elapsed;
  else core1BusyUs += elapsed;
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

void allLedsOff() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(WHITE_LED, LOW);
}

// =====================================================
// LittleFS
// =====================================================
void ensureFileLimit(const char* path, size_t maxBytes) {
  if (!LittleFS.exists(path)) return;

  File f = LittleFS.open(path, "r");
  if (!f) return;

  size_t size = f.size();
  f.close();

  if (size > maxBytes) {
    LittleFS.remove(path);
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    addEvent("BDD", "WARNING", "Fichier local purgé");
    xSemaphoreGive(stateMutex);
  }
}

void appendLine(const char* path, const String& line, size_t maxBytes) {
  ensureFileLimit(path, maxBytes);

  File f = LittleFS.open(path, "a");
  if (!f) return;

  f.println(line);
  f.close();
}

String readTailJsonlAsArray(const char* path, int maxLines) {
  if (!LittleFS.exists(path)) return "[]";

  File f = LittleFS.open(path, "r");
  if (!f) return "[]";

  String buffer[60];
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
// Capteurs
// =====================================================
float readDistanceCm() {
  digitalWrite(HCSR04_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(HCSR04_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(HCSR04_TRIG_PIN, LOW);

  // Timeout court : évite de bloquer le Core 1 trop longtemps.
  unsigned long duration = pulseIn(HCSR04_ECHO_PIN, HIGH, 18000UL);

  if (duration == 0) return -1.0;

  float distance = duration / 58.0;

  if (distance < 2 || distance > 400) return -1.0;

  return distance;
}

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

  state.joyX = x;
  state.joyY = y;
  state.joyButton = button;

  if (x == 0 && y == 0) state.joyDirection = "CENTER";
  else if (abs(x) > abs(y)) state.joyDirection = x > 0 ? "RIGHT" : "LEFT";
  else state.joyDirection = y > 0 ? "DOWN" : "UP";
}

// =====================================================
// Risque
// =====================================================
int computeRisk(float temp, float hum, int gasRaw, bool radarObject, bool hw499, bool dhtOk) {
  int risk = 0;

  if (!dhtOk) risk += 25;
  if (dhtOk && temp >= tempLimit) risk += 25;
  if (dhtOk && (hum < humLowLimit || hum > humHighLimit)) risk += 15;
  if (gasRaw >= gasLimitRaw) risk += 30;

  if (securityEnabled && radarObject) risk += 35;
  else if (radarObject) risk += 15;

  if (hw499) risk += 10;
  if (intrusionLatched) risk += 25;
  if (WiFi.status() != WL_CONNECTED) risk += 10;

  return min(100, risk);
}

const char* riskName(int score, bool dhtOk) {
  if (!dhtOk) return "CAPTEUR";
  if (score >= 80) return "CRITIQUE";
  if (score >= 55) return "DANGER";
  if (score >= 30) return "ATTENTION";
  if (score >= 10) return "INFO";
  return "NORMAL";
}

void updateMinMaxUnsafe(float temp, float hum, int gasRaw, float distance) {
  if (!isnan(temp)) {
    if (isnan(state.tempMin) || temp < state.tempMin) state.tempMin = temp;
    if (isnan(state.tempMax) || temp > state.tempMax) state.tempMax = temp;
  }

  if (!isnan(hum)) {
    if (isnan(state.humMin) || hum < state.humMin) state.humMin = hum;
    if (isnan(state.humMax) || hum > state.humMax) state.humMax = hum;
  }

  if (gasRaw > state.gasMax) state.gasMax = gasRaw;

  if (distance > 0) {
    if (isnan(state.distanceMin) || distance < state.distanceMin) {
      state.distanceMin = distance;
    }
  }
}

// =====================================================
// LEDs / buzzer
// =====================================================
void applyManualLedsUnsafe() {
  digitalWrite(RED_LED, state.ledRed ? HIGH : LOW);
  digitalWrite(BLUE_LED, state.ledBlue ? HIGH : LOW);
  digitalWrite(GREEN_LED, state.ledGreen ? HIGH : LOW);
  digitalWrite(YELLOW_LED, state.ledYellow ? HIGH : LOW);
  digitalWrite(WHITE_LED, state.ledWhite ? HIGH : LOW);
}

void applyAutoLedsUnsafe() {
  bool wifiOk = WiFi.status() == WL_CONNECTED;
  bool danger = state.riskScore >= 55 || state.intrusionLatched;
  bool warning = state.riskScore >= 30 && !danger;

  state.ledGreen = wifiOk;
  state.ledRed = danger;
  state.ledYellow = warning;
  state.ledBlue = (!danger && !warning);
  state.ledWhite = state.securityEnabled;

  applyManualLedsUnsafe();
}

void alarmToneOff() {
  ledcWriteTone(BUZZER_ALARM_CH, 0);
  ledcWrite(BUZZER_ALARM_CH, 0);
  alarmToneActive = false;
}

void musicToneOff() {
  ledcWriteTone(BUZZER_MUSIC_CH, 0);
  ledcWrite(BUZZER_MUSIC_CH, 0);
}

void setAlarmTone(bool enabled) {
  if (enabled && !alarmToneActive) {
    ledcWriteTone(BUZZER_ALARM_CH, 900);
    ledcWrite(BUZZER_ALARM_CH, 128);
    alarmToneActive = true;
  } else if (!enabled && alarmToneActive) {
    alarmToneOff();
  }
}

void playPoliceSirenStep() {
  static bool toneFlip = false;
  static uint32_t lastFlipMs = 0;

  if (millis() - lastFlipMs >= 330) {
    lastFlipMs = millis();
    toneFlip = !toneFlip;
  }

  if (toneFlip) {
    ledcWriteTone(BUZZER_ALARM_CH, 880);
    ledcWrite(BUZZER_ALARM_CH, 128);
    ledcWriteTone(BUZZER_MUSIC_CH, 660);
    ledcWrite(BUZZER_MUSIC_CH, 120);
  } else {
    ledcWriteTone(BUZZER_ALARM_CH, 660);
    ledcWrite(BUZZER_ALARM_CH, 128);
    ledcWriteTone(BUZZER_MUSIC_CH, 880);
    ledcWrite(BUZZER_MUSIC_CH, 120);
  }
}

void playMelodyStep() {
  // Petite mélodie non bloquante avec deux buzzers.
  // Buzzer alarme = note principale, buzzer musique = harmonique.
  const int notesA[] = {523, 659, 784, 1046, 784, 659, 523, 0};
  const int notesB[] = {392, 494, 587, 784, 587, 494, 392, 0};
  const int count = 8;

  static uint32_t lastStepMs = 0;
  static int step = 0;

  if (millis() - lastStepMs >= 180) {
    lastStepMs = millis();
    step = (step + 1) % count;

    int a = notesA[step];
    int b = notesB[step];

    if (a > 0) {
      ledcWriteTone(BUZZER_ALARM_CH, a);
      ledcWrite(BUZZER_ALARM_CH, 95);
    } else {
      ledcWriteTone(BUZZER_ALARM_CH, 0);
      ledcWrite(BUZZER_ALARM_CH, 0);
    }

    if (b > 0) {
      ledcWriteTone(BUZZER_MUSIC_CH, b);
      ledcWrite(BUZZER_MUSIC_CH, 85);
    } else {
      ledcWriteTone(BUZZER_MUSIC_CH, 0);
      ledcWrite(BUZZER_MUSIC_CH, 0);
    }
  }
}

void updateBuzzerUnsafe() {
  if (!soundEnabled) {
    alarmToneOff();
    musicToneOff();
    return;
  }

  if (criticalInterruptActive) {
    playPoliceSirenStep();
    return;
  }

  if (melodyDemoActive) {
    if (millis() < melodyDemoUntilMs) {
      playMelodyStep();
      return;
    }

    melodyDemoActive = false;
    alarmToneOff();
    musicToneOff();
  }

  bool danger = state.riskScore >= 55 || state.intrusionLatched;
  setAlarmTone(danger);
  musicToneOff();
}

// =====================================================
// MQTT
// =====================================================
String packetToJson(const SensorPacket& p, bool replayed = false) {
  String json = "{";
  json += "\"device\":\"ESP32-Othmane\",";
  json += "\"seq\":" + String(p.seq) + ",";
  json += "\"createdMs\":" + String(p.uptimeMs) + ",";
  json += "\"temp\":" + floatJson(p.temp, 1) + ",";
  json += "\"humidity\":" + floatJson(p.humidity, 1) + ",";
  json += "\"dhtOk\":" + boolJson(p.dhtOk) + ",";
  json += "\"gasRaw\":" + String(p.gasRaw) + ",";
  json += "\"gasPercent\":" + String(p.gasPercent, 1) + ",";
  json += "\"distanceCm\":" + floatJson(p.distanceCm, 1) + ",";
  json += "\"radarObject\":" + boolJson(p.radarObject) + ",";
  json += "\"radarOk\":" + boolJson(p.radarOk) + ",";
  json += "\"hw499\":" + boolJson(p.hw499) + ",";
  json += "\"wifi\":" + boolJson(p.wifiOk) + ",";
  json += "\"riskScore\":" + String(p.riskScore) + ",";
  json += "\"riskState\":\"" + String(p.riskState) + "\",";
  json += "\"replayed\":" + boolJson(replayed);
  json += "}";

  return json;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;

  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  msg.trim();

  xSemaphoreTake(stateMutex, portMAX_DELAY);

  if (msg == "securityOn") {
    securityEnabled = true;
    state.securityEnabled = true;
    addEvent("MQTT", "INFO", "Sécurité activée via MQTT");
  } else if (msg == "securityOff") {
    securityEnabled = false;
    intrusionLatched = false;
    state.securityEnabled = false;
    state.intrusionLatched = false;
    addEvent("MQTT", "INFO", "Sécurité désactivée via MQTT");
  } else if (msg == "ack") {
    intrusionLatched = false;
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

  while (f.available() && sentNow < 6) {
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
  json += "\"dhtOk\":" + boolJson(state.dhtOk) + ",";
  json += "\"gasRaw\":" + String(state.gasRaw) + ",";
  json += "\"gasPercent\":" + String(state.gasPercent, 1) + ",";
  json += "\"gasOk\":" + boolJson(state.gasOk) + ",";
  json += "\"distanceCm\":" + floatJson(state.distanceCm, 1) + ",";
  json += "\"radarObject\":" + boolJson(state.radarObject) + ",";
  json += "\"radarOk\":" + boolJson(state.radarOk) + ",";
  json += "\"radarDetectCount\":" + String(state.radarDetectCount) + ",";
  json += "\"radarLimit\":" + String(radarLimitCm, 1) + ",";
  json += "\"radarMax\":" + String(radarMaxCm, 1) + ",";
  json += "\"hw499\":" + boolJson(state.hw499) + ",";
  json += "\"hw499Count\":" + String(state.hw499Count) + ",";
  json += "\"security\":" + boolJson(state.securityEnabled) + ",";
  json += "\"intrusion\":" + boolJson(state.intrusionLatched) + ",";
  json += "\"intrusionCount\":" + String(state.intrusionCount) + ",";
  json += "\"riskScore\":" + String(state.riskScore) + ",";
  json += "\"riskState\":\"" + state.riskState + "\",";
  json += "\"tempMin\":" + floatJson(state.tempMin, 1) + ",";
  json += "\"tempMax\":" + floatJson(state.tempMax, 1) + ",";
  json += "\"humMin\":" + floatJson(state.humMin, 1) + ",";
  json += "\"humMax\":" + floatJson(state.humMax, 1) + ",";
  json += "\"gasMax\":" + String(state.gasMax) + ",";
  json += "\"distanceMin\":" + floatJson(state.distanceMin, 1) + ",";
  json += "\"ledRed\":" + boolJson(state.ledRed) + ",";
  json += "\"ledBlue\":" + boolJson(state.ledBlue) + ",";
  json += "\"ledGreen\":" + boolJson(state.ledGreen) + ",";
  json += "\"ledYellow\":" + boolJson(state.ledYellow) + ",";
  json += "\"ledWhite\":" + boolJson(state.ledWhite) + ",";
  json += "\"manualLedMode\":" + boolJson(manualLedMode) + ",";
  json += "\"soundEnabled\":" + boolJson(soundEnabled) + ",";
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
  json += "\"loopsSensors\":" + String(loopsSensors) + ",";
  json += "\"loopsActuators\":" + String(loopsActuators) + ",";
  json += "\"loopsMqtt\":" + String(loopsMqtt) + ",";
  json += "\"loopsWiFi\":" + String(loopsWiFi) + ",";
  json += "\"loopsCritical\":" + String(loopsCriticalInterrupt) + ",";
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

    int limit = 40;
    if (request->hasParam("limit")) {
      limit = request->getParam("limit")->value().toInt();
      if (limit < 10) limit = 10;
      if (limit > 60) limit = 60;
    }

    request->send(200, "application/json", readTailJsonlAsArray(HISTORY_FILE, limit));
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
      securityEnabled = true;
      state.securityEnabled = true;
      addEvent("SECURITE", "INFO", "Mode sécurité activé");
    } else if (cmd == "securityOff") {
      securityEnabled = false;
      intrusionLatched = false;
      criticalInterruptActive = false;
      melodyDemoActive = false;
      state.securityEnabled = false;
      state.intrusionLatched = false;
      alarmToneOff();
      musicToneOff();
      addEvent("SECURITE", "INFO", "Mode sécurité désactivé");
    } else if (cmd == "ack") {
      criticalInterruptActive = false;
      melodyDemoActive = false;
      intrusionLatched = false;
      state.intrusionLatched = false;
      alarmToneOff();
      musicToneOff();
      addEvent("ALARME", "INFO", "Alarme acquittée");
    } else if (cmd == "autoLeds") {
      manualLedMode = false;
      addEvent("LED", "INFO", "Mode LEDs automatique");
    } else if (cmd == "manualLeds") {
      manualLedMode = true;
      addEvent("LED", "INFO", "Mode LEDs manuel");
    } else if (cmd == "allLedsOff") {
      manualLedMode = true;
      state.ledRed = false;
      state.ledBlue = false;
      state.ledGreen = false;
      state.ledYellow = false;
      state.ledWhite = false;
      applyManualLedsUnsafe();
      addEvent("LED", "INFO", "Toutes les LEDs éteintes");
    } else if (cmd == "soundOn") {
      soundEnabled = true;
      addEvent("SON", "INFO", "Son activé");
    } else if (cmd == "melodyDemo") {
      soundEnabled = true;
      criticalInterruptActive = false;
      melodyDemoActive = true;
      melodyDemoUntilMs = millis() + 9000UL;
      addEvent("SON", "INFO", "Mélodie deux buzzers lancée");
    } else if (cmd == "melodyStop") {
      melodyDemoActive = false;
      alarmToneOff();
      musicToneOff();
      addEvent("SON", "INFO", "Mélodie arrêtée");
    } else if (cmd == "criticalOn") {
      soundEnabled = true;
      criticalInterruptActive = true;
      melodyDemoActive = false;
      securityEnabled = true;
      intrusionLatched = true;
      state.securityEnabled = true;
      state.intrusionLatched = true;
      addEvent("INTERRUPTION", "CRITIQUE", "Simulation critique activée");
    } else if (cmd == "criticalOff") {
      criticalInterruptActive = false;
      intrusionLatched = false;
      state.intrusionLatched = false;
      alarmToneOff();
      musicToneOff();
      addEvent("INTERRUPTION", "INFO", "Simulation critique désactivée");
    } else if (cmd == "soundOff") {
      soundEnabled = false;
      melodyDemoActive = false;
      criticalInterruptActive = false;
      alarmToneOff();
      musicToneOff();
      addEvent("SON", "INFO", "Son désactivé");
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
    } else if (cmd == "mqttFaultOff") {
      mqttFaultUntilMs = 0;
      addEvent("MQTT", "INFO", "Simulation panne MQTT arrêtée");
    } else if (cmd == "wifiOff60") {
      wifiDisabledUntilMs = millis() + 60UL * 1000UL;
      WiFi.disconnect(true);
      addEvent("WIFI", "WARNING", "WiFi coupé 1 min");
    } else if (cmd == "wifiOff300") {
      wifiDisabledUntilMs = millis() + 5UL * 60UL * 1000UL;
      WiFi.disconnect(true);
      addEvent("WIFI", "WARNING", "WiFi coupé 5 min");
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
    } else if (cmd == "criticalToggle") {
      criticalInterruptActive = !criticalInterruptActive;
      soundEnabled = true;
      securityEnabled = criticalInterruptActive ? true : securityEnabled;
      intrusionLatched = criticalInterruptActive ? true : false;
      state.securityEnabled = securityEnabled;
      state.intrusionLatched = intrusionLatched;
      if (!criticalInterruptActive) {
        alarmToneOff();
        musicToneOff();
      }
      addEvent("INTERRUPTION", criticalInterruptActive ? "CRITIQUE" : "INFO", criticalInterruptActive ? "Interruption critique ON" : "Interruption critique OFF");
    } else if (cmd == "resetStats") {
      state.tempMin = NAN;
      state.tempMax = NAN;
      state.humMin = NAN;
      state.humMax = NAN;
      state.gasMax = 0;
      state.distanceMin = NAN;
      state.radarDetectCount = 0;
      state.hw499Count = 0;
      state.intrusionCount = 0;
      queueDrops = 0;
      addEvent("SYSTEME", "INFO", "Statistiques remises à zéro");
    } else if (cmd == "led") {
      manualLedMode = true;
      String led = request->hasParam("led") ? request->getParam("led")->value() : "";
      bool on = request->hasParam("on") ? request->getParam("on")->value().toInt() == 1 : false;

      if (led == "red") state.ledRed = on;
      else if (led == "blue") state.ledBlue = on;
      else if (led == "green") state.ledGreen = on;
      else if (led == "yellow") state.ledYellow = on;
      else if (led == "white") state.ledWhite = on;

      applyManualLedsUnsafe();
      addEvent("LED", "INFO", "Commande LED manuelle");
    }

    xSemaphoreGive(stateMutex);

    request->send(200, "application/json", "{\"ok\":true}");
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
    if (request->hasParam("radarLimit")) radarLimitCm = request->getParam("radarLimit")->value().toFloat();
    if (request->hasParam("mqtt")) mqttEnabled = request->getParam("mqtt")->value().toInt() == 1;
    if (request->hasParam("mqttBase")) mqttBaseTopic = request->getParam("mqttBase")->value();

    xSemaphoreTake(stateMutex, portMAX_DELAY);
    addEvent("CONFIG", "INFO", "Configuration mise à jour");
    xSemaphoreGive(stateMutex);

    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.begin();
}

// =====================================================
// Tasks
// =====================================================
void taskCriticalInterrupt(void* parameter) {
  // Tâche dédiée au mode interruption critique.
  // Elle ne tourne réellement que quand criticalInterruptActive = true.
  // Rôle : forcer les valeurs affichées et maintenir l'état critique
  // pour la démonstration sans modifier le câblage réel.
  for (;;) {
    uint32_t start = micros();

    if (criticalInterruptActive) {
      xSemaphoreTake(stateMutex, portMAX_DELAY);

      state.temp = 99.9;
      state.humidity = 100.0;
      state.dhtOk = true;

      state.gasRaw = 4095;
      state.gasFiltered = 4095;
      state.gasPercent = 100.0;
      state.gasOk = true;

      state.distanceCm = 20.0;
      state.radarOk = true;
      state.radarObject = true;

      state.hw499 = true;
      state.securityEnabled = true;
      state.intrusionLatched = true;

      state.riskScore = 100;
      state.riskState = "CRITIQUE";

      xSemaphoreGive(stateMutex);
    }

    loopsCriticalInterrupt++;
    recordBusy(1, start);

    vTaskDelay(pdMS_TO_TICKS(120));
  }
}

void taskSensors(void* parameter) {
  TickType_t lastWake = xTaskGetTickCount();

  bool prevRadarObject = false;
  bool prevHw = false;
  bool prevButton = false;

  for (;;) {
    uint32_t start = micros();

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    bool dhtOk = !isnan(temp) && !isnan(hum);

    int gasRawLocal = analogRead(GAS_AO_PIN);

    float distance = readDistanceCm();
    bool radarOk = distance > 0;
    bool radarObject = radarOk && distance <= radarLimitCm;

    int hwRaw = digitalRead(HW499_PIN);
    bool hw = HW499_ACTIVE_LOW ? hwRaw == LOW : hwRaw == HIGH;

    bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;

    // Mode interruption critique :
    // on simule un incident industriel en forçant les valeurs capteurs.
    // Les vraies lectures physiques ne sont pas supprimées, elles sont juste remplacées
    // pendant la démonstration.
    if (criticalInterruptActive) {
      temp = 99.9;
      hum = 100.0;
      dhtOk = true;
      gasRawLocal = 4095;
      distance = 20.0;
      radarOk = true;
      radarObject = true;
      hw = true;
    }

    xSemaphoreTake(stateMutex, portMAX_DELAY);

    state.seq++;

    if (dhtOk) {
      state.temp = temp;
      state.humidity = hum;
    }

    state.dhtOk = dhtOk;
    state.gasRaw = gasRawLocal;

    if (state.gasFiltered < 0) state.gasFiltered = gasRawLocal;
    else state.gasFiltered = state.gasFiltered * 0.85 + gasRawLocal * 0.15;

    state.gasPercent = (state.gasFiltered / 4095.0) * 100.0;
    state.gasOk = !(gasRawLocal < 5 || gasRawLocal > 4090);

    state.distanceCm = distance;
    state.radarOk = radarOk;
    state.radarObject = radarObject;

    state.hw499 = hw;
    state.buttonPressed = buttonPressed;

    state.securityEnabled = securityEnabled;
    state.intrusionLatched = intrusionLatched;

    readJoystickUnsafe();

    if (radarObject && !prevRadarObject) {
      state.radarDetectCount++;
      addEvent("RADAR", "INFO", "Objet détecté HC-SR04");

      if (securityEnabled) {
        intrusionLatched = true;
        state.intrusionLatched = true;
        state.intrusionCount++;
        addEvent("SECURITE", "CRITIQUE", "Intrusion radar");
      }
    }

    if (hw && !prevHw) {
      state.hw499Count++;
      addEvent("HW499", "INFO", "Événement HW-499");
    }

    if (buttonPressed && !prevButton) {
      if (intrusionLatched) {
        intrusionLatched = false;
        state.intrusionLatched = false;
        addEvent("BOUTON", "INFO", "Alarme acquittée");
      } else {
        securityEnabled = !securityEnabled;
        state.securityEnabled = securityEnabled;
        addEvent("BOUTON", "INFO", securityEnabled ? "Sécurité activée" : "Sécurité désactivée");
      }
    }

    prevRadarObject = radarObject;
    prevHw = hw;
    prevButton = buttonPressed;

    updateMinMaxUnsafe(state.temp, state.humidity, gasRawLocal, distance);

    state.riskScore = computeRisk(state.temp, state.humidity, gasRawLocal, radarObject, hw, dhtOk);
    state.riskState = riskName(state.riskScore, dhtOk);

    SensorPacket p;
    p.seq = state.seq;
    p.uptimeMs = millis();
    p.temp = state.temp;
    p.humidity = state.humidity;
    p.dhtOk = dhtOk;
    p.gasRaw = gasRawLocal;
    p.gasPercent = state.gasPercent;
    p.distanceCm = distance;
    p.radarObject = radarObject;
    p.radarOk = radarOk;
    p.hw499 = hw;
    p.wifiOk = WiFi.status() == WL_CONNECTED;
    p.riskScore = state.riskScore;
    strncpy(p.riskState, state.riskState.c_str(), sizeof(p.riskState) - 1);
    p.riskState[sizeof(p.riskState) - 1] = 0;

    xSemaphoreGive(stateMutex);

    if (xQueueSend(sensorQueue, &p, 0) != pdTRUE) queueDrops++;

    loopsSensors++;
    recordBusy(1, start);

    // DHT22 = lecture lente. 1500 ms est plus léger que 1000 ms.
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(1500));
  }
}

void taskActuators(void* parameter) {
  for (;;) {
    uint32_t start = micros();

    xSemaphoreTake(stateMutex, portMAX_DELAY);

    if (manualLedMode) applyManualLedsUnsafe();
    else applyAutoLedsUnsafe();

    updateBuzzerUnsafe();

    xSemaphoreGive(stateMutex);

    loopsActuators++;
    recordBusy(1, start);

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void taskMqtt(void* parameter) {
  for (;;) {
    // IMPORTANT : on ne compte pas le temps bloqué dans xQueueReceive()
    // comme de la charge CPU. Avant, ce timeout de 300 ms gonflait
    // artificiellement Core 0 à 60-70 % dans la supervision.
    uint32_t activeStart = micros();

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

    recordBusy(0, activeStart);

    SensorPacket p;

    // Cette attente est volontairement bloquante mais ne consomme pas vraiment le CPU.
    if (xQueueReceive(sensorQueue, &p, pdMS_TO_TICKS(300)) == pdTRUE) {
      activeStart = micros();

      String line = packetToJson(p, false);

      appendLine(HISTORY_FILE, line, MAX_HISTORY_BYTES);

      bool sent = mqttPublishLine(line, false);

      if (!sent) {
        appendLine(OFFLINE_FILE, line, MAX_OFFLINE_BYTES);
        offlineStoredCount++;
      }

      recordBusy(0, activeStart);
    }

    loopsMqtt++;

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void taskWiFi(void* parameter) {
  for (;;) {
    uint32_t activeStart = micros();

    if (millis() < wifiDisabledUntilMs) {
      if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect(true);
      }
    } else {
      if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
      }
    }

    loopsWiFi++;
    recordBusy(0, activeStart);

    vTaskDelay(pdMS_TO_TICKS(6000));
  }
}

void taskCpuMonitor(void* parameter) {
  for (;;) {
    portENTER_CRITICAL(&metricsMux);

    uint32_t c0 = core0BusyUs;
    uint32_t c1 = core1BusyUs;
    core0BusyUs = 0;
    core1BusyUs = 0;

    portEXIT_CRITICAL(&metricsMux);

    core0Load = min(100.0f, (c0 / 1000000.0f) * 100.0f);
    core1Load = min(100.0f, (c1 / 1000000.0f) * 100.0f);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void taskLog(void* parameter) {
  for (;;) {
    Serial.println("===== ESP32 Core Balance Corrige =====");
    Serial.print("Core0 reseau: ");
    Serial.print(core0Load);
    Serial.print("% | Core1 capteurs: ");
    Serial.print(core1Load);
    Serial.println("%");

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("MQTT: ");
    Serial.println(mqttConnected ? "connecte" : "off");

    Serial.print("Distance: ");
    Serial.println(state.distanceCm);

    Serial.print("Heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.println("===============================");

    vTaskDelay(pdMS_TO_TICKS(10000));
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

  pinMode(GAS_AO_PIN, INPUT);
  pinMode(HW499_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(HCSR04_TRIG_PIN, OUTPUT);
  pinMode(HCSR04_ECHO_PIN, INPUT);
  digitalWrite(HCSR04_TRIG_PIN, LOW);

  pinMode(JOY_SW_PIN, INPUT_PULLUP);

  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(WHITE_LED, OUTPUT);

  pinMode(BUZZER_ALARM_PIN, OUTPUT);
  pinMode(BUZZER_MUSIC_PIN, OUTPUT);

  // Initialisation LEDC des buzzers.
  // On n'utilise plus tone()/noTone() pour éviter les erreurs LEDC non initialisé.
  ledcSetup(BUZZER_ALARM_CH, 2000, 8);
  ledcAttachPin(BUZZER_ALARM_PIN, BUZZER_ALARM_CH);
  ledcWrite(BUZZER_ALARM_CH, 0);

  ledcSetup(BUZZER_MUSIC_CH, 2000, 8);
  ledcAttachPin(BUZZER_MUSIC_PIN, BUZZER_MUSIC_CH);
  ledcWrite(BUZZER_MUSIC_CH, 0);

  allLedsOff();

  analogReadResolution(12);
  analogSetPinAttenuation(GAS_AO_PIN, ADC_11db);
  analogSetPinAttenuation(JOY_X_PIN, ADC_11db);
  analogSetPinAttenuation(JOY_Y_PIN, ADC_11db);

  if (!LittleFS.begin(true)) Serial.println("Erreur LittleFS");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(1000);

  setupWebServer();

  xSemaphoreTake(stateMutex, portMAX_DELAY);
  addEvent("SYSTEME", "INFO", "Station futuriste Core balance démarrée");
  xSemaphoreGive(stateMutex);

  // Core 1 : temps réel / capteurs / actionneurs
  xTaskCreatePinnedToCore(taskSensors, "TaskSensors", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(taskActuators, "TaskActuators", 3072, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskCriticalInterrupt, "TaskCriticalInterrupt", 3072, NULL, 2, NULL, 1);

  // Core 0 : réseau / MQTT / Web / logs
  xTaskCreatePinnedToCore(taskMqtt, "TaskMQTT", 6144, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(taskWiFi, "TaskWiFi", 3072, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskCpuMonitor, "TaskCpuMonitor", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskLog, "TaskLog", 3072, NULL, 1, NULL, 0);

  Serial.println("Setup terminé : loop vide, Core0 reseau, Core1 capteurs, CPU accounting corrige.");
}

void loop() {
}
