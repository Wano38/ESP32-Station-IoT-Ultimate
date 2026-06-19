#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <LittleFS.h>

// IMPORTANT : ne pas inclure index.cpp ici.
// PlatformIO compile index.cpp séparément.
extern const char index_html[] PROGMEM;

// =====================================================
// STATION IoT ESP32 ULTIMATE - FreeRTOS
// Aucun appel bloquant Arduino
// loop() vide
//
// Capteurs : DHT22, PIR HC-SR501, MQ gaz, HW-499
// Actionneurs : 2 buzzers + 5 LEDs
// Fonctions : dashboard, sécurité, historique, risque,
// simulation démo, MQTT, musiques, effets lumineux, joystick HW-504 et jeux Web.
// Amélioration mission : Queue FreeRTOS capteurs -> MQTT,
// buffer offline/replay, latence MQTT, bouton physique,
// validation des commandes API, login web, base locale LittleFS,
// coupure WiFi volontaire pour tester offline/replay,
// et commentaires pédagogiques sur les priorités.
// =====================================================

// =======================
// WiFi
// =======================
const char* ssid = "iPhone de Othmane";
const char* password = "othmane59";

// =======================
// MQTT optionnel
// =======================
const char* mqttServer = "test.mosquitto.org";
const int mqttPort = 1883;
String mqttBaseTopic = "campus/groupe1/ESP32-Othmane";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool mqttEnabled = false;
bool mqttConnected = false;
unsigned long lastMqttPublishMs = 0;
unsigned long lastMqttReconnectAttemptMs = 0;

// Sécurité simple de l'API locale : les commandes Web sensibles
// doivent envoyer ce token. Le navigateur l'ajoute automatiquement.
// Pour un vrai produit industriel, il faudrait HTTPS + authentification forte.
const char* API_TOKEN = "1234";

// Login HTTP Basic demandé pour sécuriser le site local.
// Au premier accès au site, le navigateur demande : admin / esp32.
// Garde simple pour la soutenance ; en production il faudrait HTTPS.
const char* WEB_USER = "admin";
const char* WEB_PASS = "esp32";

// Base de données locale ESP32 : fichier JSON Lines sur LittleFS.
// Chaque ligne est un JSON de mesure non publiée.
// Si le WiFi/MQTT tombe 30 min ou 1 h, les mesures restent ici
// puis sont rejouées automatiquement quand MQTT revient.
const char* OFFLINE_DB_FILE = "/offline.jsonl";
const size_t OFFLINE_DB_MAX_BYTES = 700000;   // environ 1 h+ de mesures à 2 s selon taille JSON
bool littleFsOk = false;
unsigned long offlineFileCount = 0;
unsigned long offlineFileBytes = 0;
unsigned long offlineFileReplayed = 0;
unsigned long offlineFileWriteErrors = 0;

// Historique local permanent des mesures.
// Différence avec OFFLINE_DB_FILE :
// - offline.jsonl sert au replay MQTT puis peut être vidé/rejoué ;
// - history.jsonl garde une trace pour les graphiques du dashboard.
// Ainsi, si le WiFi est coupé 30 min ou 1 h, l'ESP32 continue
// d'enregistrer les températures et le site peut les afficher après reconnexion.
const char* HISTORY_DB_FILE = "/history.jsonl";
const size_t HISTORY_DB_MAX_BYTES = 900000;
const int HISTORY_API_MAX_POINTS = 120;
unsigned long historyFileCount = 0;
unsigned long historyFileBytes = 0;
unsigned long historyFileWriteErrors = 0;
unsigned long historyFileRotations = 0;

// Test de panne MQTT sans couper le WiFi.
// Avantage en démo : le site Web reste accessible, les graphiques live
// continuent, mais TaskMQTT stocke quand même en offline comme si le broker
// ou le réseau MQTT était indisponible.
bool mqttBlackoutActive = false;
unsigned long mqttBlackoutUntilMs = 0;
unsigned long mqttBlackoutCount = 0;

// Test de panne réseau depuis le dashboard.
// Quand networkForcedOff=true, le WiFi est coupé volontairement
// et les mesures continuent d'être envoyées vers la Queue puis stockées offline.
bool networkForcedOff = false;
unsigned long wifiOffUntilMs = 0;
unsigned long wifiCutCount = 0;

// Statistiques MQTT demandées pour la supervision.
unsigned long mqttPublishedCount = 0;
unsigned long mqttFailedCount = 0;
unsigned long lastPublishLatencyMs = 0;
unsigned long publishLatencySumMs = 0;
unsigned long offlineDroppedCount = 0;
unsigned long queueDroppedCount = 0;

// =======================
// DHT22
// =======================
#define DHT_TYPE DHT22
const int DHT_PIN = 4;
DHT dht(DHT_PIN, DHT_TYPE);

// =======================
// Capteurs
// =======================
const int PIR_PIN = 26;
const int HW499_PIN = 33;
const int GAS_AO_PIN = 34;

// Bouton physique demandé dans le sujet.
// Branchement conseillé : une patte sur GPIO25, l'autre sur GND.
// Le code utilise INPUT_PULLUP, donc appuyé = LOW.
const int BUTTON_PIN = 25;

// Joystick HW-504 / module joystick analogique.
// Branchement conseillé en 3V3 uniquement :
// VCC -> 3V3, GND -> GND, VRX -> GPIO32, VRY -> GPIO35, SW -> GPIO23.
// VRX et VRY sont deux sorties analogiques, SW est le bouton du joystick.
const int JOY_X_PIN = 32;
const int JOY_Y_PIN = 35;
const int JOY_SW_PIN = 23;
const int JOY_DEADZONE = 650;

// Si le HW-499 réagit à l'envers, mets false.
const bool HW499_ACTIVE_LOW = true;

// =======================
// LEDs
// =======================
const int RED_LED = 5;
const int BLUE_LED = 18;
const int GREEN_LED = 19;
const int YELLOW_LED = 21;
const int WHITE_LED = 22;

const int LED_COUNT = 5;
const int LED_PINS[LED_COUNT] = {
  RED_LED,
  BLUE_LED,
  GREEN_LED,
  YELLOW_LED,
  WHITE_LED
};

// =======================
// Buzzers
// =======================
const int BUZZER_ALARM_PIN = 12;
const int BUZZER_MUSIC_PIN = 27;

// =======================
// Seuils
// =======================
float tempLimit = 35.0;
float humLowLimit = 30.0;
float humHighLimit = 75.0;
int gasLimitRaw = 2500;

// =======================
// Données capteurs réelles
// =======================
float lastTemperatureC = NAN;
float lastHumidity = NAN;

float tempMin = NAN;
float tempMax = NAN;
float humMin = NAN;
float humMax = NAN;

int gasRaw = 0;
float gasFiltered = -1.0;
float gasPercent = 0.0;
int gasMax = 0;

// Détection de panne capteur : utile pour la maintenance.
// DHT OK = false si lecture NaN ; MQ suspect si ADC reste collé à 0 ou 4095.
bool dhtOk = false;
unsigned long dhtErrorCount = 0;
bool gasSensorOk = true;
int gasFaultStreak = 0;

bool pirMotion = false;
bool hw499Active = false;
bool previousPirState = false;
bool previousHw499State = false;

bool buttonPressed = false;
bool previousButtonPressed = false;
unsigned long buttonPressCount = 0;
unsigned long lastButtonPressMs = 0;
unsigned long lastButtonEdgeMs = 0;

// État joystick lu par TaskSensors et exposé à /api/joystick.
// Les jeux tournent dans le navigateur, mais les directions viennent du HW-504.
int joyXRaw = 2048;
int joyYRaw = 2048;
int joyXNorm = 0;     // -100 gauche, +100 droite
int joyYNorm = 0;     // -100 haut, +100 bas
bool joyButtonPressed = false;
bool previousJoyButtonPressed = false;
unsigned long joyButtonCount = 0;
unsigned long lastJoyButtonMs = 0;
String joyDirection = "CENTER";

unsigned long pirMotionCount = 0;
unsigned long hw499EventCount = 0;
unsigned long intrusionCount = 0;

unsigned long lastPirMotionMs = 0;
unsigned long lastHw499EventMs = 0;
unsigned long lastIntrusionMs = 0;

// =======================
// Mode simulation démo
// =======================
bool demoMode = false;
unsigned long demoUntilMs = 0;

float demoTemperature = 42.0;
float demoHumidity = 82.0;
int demoGasRaw = 3300;
bool demoPir = true;
bool demoHw499 = false;

// =======================
// États système
// =======================
bool manualMode = false;
bool securityEnabled = false;
bool intrusionLatched = false;
bool nightMode = false;
bool ecoMode = false;
bool soundEnabled = true;
bool lightEnabled = true;

unsigned long alarmSilencedUntilMs = 0;

// Mode lumière :
// 0 auto, 1 manuel, 2 cyber, 3 scanner, 4 pulse,
// 5 police, 6 wave, 7 off, 8 party, 9 ghost, 10 heartbeat
int lightMode = 0;
int lightStep = 0;
bool lightBlinkState = false;

// Alarme principale
bool alarmBuzzerActive = false;
bool alarmBuzzerStarted = false;
bool buzzerUp = true;
int buzzerFreq = 300;
unsigned long alarmTestUntil = 0;

// =======================
// Musiques second buzzer
// =======================
struct Note {
  uint16_t freq;
  uint16_t durationMs;
};

const Note SONG_STARTUP[] = {
  {440, 100}, {660, 100}, {880, 100}, {1320, 150},
  {0, 60}, {990, 120}, {1320, 180}, {1760, 220}
};

const Note SONG_SUCCESS[] = {
  {523, 100}, {659, 100}, {784, 100}, {1047, 230}
};

const Note SONG_CYBER[] = {
  {392, 75}, {0, 35}, {523, 75}, {0, 35}, {784, 75}, {0, 35},
  {1047, 95}, {784, 75}, {523, 75}, {392, 140}
};

const Note SONG_ALERT[] = {
  {900, 100}, {0, 60}, {900, 100}, {0, 60},
  {650, 130}, {0, 70}, {1000, 190}, {0, 60}, {1200, 220}
};

const Note SONG_MOTION[] = {
  {1500, 80}, {0, 45}, {1500, 80}, {0, 45}, {1800, 110}
};

const Note SONG_RETRO[] = {
  {660, 85}, {660, 85}, {0, 70}, {660, 85},
  {0, 70}, {510, 90}, {660, 90}, {0, 70},
  {770, 130}, {0, 100}, {380, 130}
};

const Note SONG_PARTY[] = {
  {440, 80}, {554, 80}, {659, 80}, {880, 110},
  {0, 40}, {880, 80}, {659, 80}, {554, 80}, {440, 120},
  {0, 40}, {740, 90}, {988, 160}
};

const Note SONG_ROBOT[] = {
  {200, 70}, {350, 70}, {0, 40}, {250, 70}, {500, 70},
  {0, 40}, {300, 70}, {600, 90}, {0, 50}, {150, 120}
};

const Note SONG_LAUGH[] = {
  {900, 70}, {0, 40}, {1200, 70}, {0, 40},
  {900, 70}, {0, 40}, {1200, 70}, {0, 40},
  {700, 140}
};

const Note* currentSong = nullptr;
int currentSongLength = 0;
int currentSongIndex = 0;
bool musicActive = false;
bool musicToneOn = false;
unsigned long nextMusicChangeMs = 0;
String currentSongName = "none";

// =======================
// Paramètres buzzer alarme
// =======================
const int FREQ_MIN = 300;
const int FREQ_MAX = 950;
const int FREQ_STEP = 18;

// =======================
// Historique événements
// =======================
const int EVENT_COUNT = 12;

struct EventItem {
  unsigned long ts;
  String type;
  String severity;
  String message;
};

EventItem events[EVENT_COUNT];
int eventWriteIndex = 0;
int eventTotal = 0;

// =======================
// Alertes précédentes
// =======================
bool prevTempAlert = false;
bool prevHumAlert = false;
bool prevGasAlert = false;
bool prevPresenceAlert = false;
bool prevWifiOk = false;
String prevRiskState = "UNKNOWN";

// =======================
// Queue FreeRTOS + offline/replay
// =======================
// Le sujet demande une Queue entre TaskSensors et TaskMQTT.
// La tâche capteurs écrit des SensorPacket dans sensorQueue.
// La tâche MQTT les consomme, les publie et les garde en offline si le réseau tombe.
struct SensorPacket {
  uint32_t seq;
  unsigned long createdMs;
  float temp;
  float humidity;
  int gasRaw;
  float gasPercent;
  bool pir;
  bool hw499;
  bool button;
  bool wifiOk;
  bool mqttOk;
  bool networkForced;
  bool mqttBlackout;
  bool dhtOk;
  bool gasOk;
  uint8_t risk;
  char state[18];
};

QueueHandle_t sensorQueue = nullptr;
const int SENSOR_QUEUE_LENGTH = 8;
uint32_t sensorSequence = 0;

// Buffer offline en RAM : il permet de rejouer les mesures quand MQTT revient.
// Il ne survit pas à un redémarrage, mais répond au principe offline/replay.
const int OFFLINE_CAPACITY = 24;
SensorPacket offlineBuffer[OFFLINE_CAPACITY];
int offlineWriteIndex = 0;
int offlineCount = 0;

// =======================
// Serveur + mutex
// =======================
AsyncWebServer server(80);
SemaphoreHandle_t dataMutex;

// TaskWeb est créée pour correspondre à l'architecture demandée.
// Le serveur est asynchrone, donc cette tâche sert de heartbeat/supervision.
unsigned long webTaskHeartbeat = 0;

// =====================================================
// Supervision processeur / tâches FreeRTOS
// =====================================================
// L'ESP32 possède deux cœurs. Les tâches sont épinglées volontairement :
// - Core 1 : acquisition, supervision, alarmes et jeux de lumière.
// - Core 0 : WiFi, MQTT, serveur Web et logs.
// Ces compteurs mesurent le temps actif passé dans chaque tâche.
// Le dashboard calcule ensuite une charge estimée par cœur.
enum PerfTaskId {
  PERF_WIFI,
  PERF_SENSORS,
  PERF_MQTT,
  PERF_WEB,
  PERF_SUPERVISION,
  PERF_LIGHTS,
  PERF_ALARM_BUZZER,
  PERF_MUSIC_BUZZER,
  PERF_SYSTEM_LOG,
  PERF_COUNT
};

const char* PERF_NAMES[PERF_COUNT] = {
  "TaskWiFi",
  "TaskSensors",
  "TaskMQTT",
  "TaskWeb",
  "TaskSupervision",
  "TaskLights",
  "TaskAlarmBuzzer",
  "TaskMusicBuzzer",
  "TaskSystemLog"
};

const uint8_t PERF_CORES[PERF_COUNT] = {0, 1, 0, 0, 1, 1, 1, 1, 0};
const uint8_t PERF_PRIORITIES[PERF_COUNT] = {1, 3, 2, 1, 2, 1, 2, 1, 1};

volatile uint32_t perfRuntimeUs[PERF_COUNT] = {0};
volatile uint32_t perfLoops[PERF_COUNT] = {0};

// =====================================================
// Utilitaires
// =====================================================
String boolToJson(bool value) {
  return value ? "true" : "false";
}

String floatToJson(float value, int decimals = 1) {
  if (isnan(value)) return "null";
  return String(value, decimals);
}

void recordTaskRuntime(PerfTaskId id, unsigned long startUs) {
  if (id < 0 || id >= PERF_COUNT) return;

  unsigned long elapsed = micros() - startUs;
  perfRuntimeUs[id] += (uint32_t)elapsed;
  perfLoops[id]++;
}

int clampInt(int value, int low, int high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

String jsonEscape(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", " ");
  s.replace("\r", " ");
  return s;
}

// Login HTTP Basic pour le site et les APIs.
// Les fetch() du dashboard fonctionnent car ils viennent du même navigateur
// après le login initial.
bool requireLogin(AsyncWebServerRequest *request) {
  if (!request->authenticate(WEB_USER, WEB_PASS)) {
    request->requestAuthentication();
    return false;
  }
  return true;
}

// Les commandes sensibles demandent login + token.
// Le token évite qu'une page externe appelle les commandes par hasard.
bool isAuthorized(AsyncWebServerRequest *request) {
  return request->authenticate(WEB_USER, WEB_PASS) &&
         request->hasParam("token") &&
         request->getParam("token")->value() == String(API_TOKEN);
}

void sendForbidden(AsyncWebServerRequest *request) {
  request->send(403, "application/json", "{\"ok\":false,\"error\":\"forbidden\"}");
}

float boundedFloat(String value, float currentValue, float minValue, float maxValue) {
  float parsed = value.toFloat();
  if (parsed < minValue || parsed > maxValue) {
    return currentValue;
  }
  return parsed;
}

int boundedInt(String value, int currentValue, int minValue, int maxValue) {
  int parsed = value.toInt();
  if (parsed < minValue || parsed > maxValue) {
    return currentValue;
  }
  return parsed;
}

bool isSafeTopic(String topic) {
  if (topic.length() == 0 || topic.length() > 90) return false;

  for (unsigned int i = 0; i < topic.length(); i++) {
    char c = topic[i];

    bool ok = (c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '/' || c == '-' || c == '_' || c == '.';

    if (!ok) return false;
  }

  return true;
}

bool demoActiveUnsafe() {
  if (!demoMode) return false;

  if (millis() > demoUntilMs) {
    demoMode = false;
    return false;
  }

  return true;
}

float effectiveTempUnsafe() {
  return demoActiveUnsafe() ? demoTemperature : lastTemperatureC;
}

float effectiveHumidityUnsafe() {
  return demoActiveUnsafe() ? demoHumidity : lastHumidity;
}

float effectiveGasRawUnsafe() {
  return demoActiveUnsafe() ? demoGasRaw : gasFiltered;
}

float effectiveGasPercentUnsafe() {
  if (demoActiveUnsafe()) {
    return (demoGasRaw / 4095.0) * 100.0;
  }

  return gasPercent;
}

bool effectivePirUnsafe() {
  return demoActiveUnsafe() ? demoPir : pirMotion;
}

bool effectiveHw499Unsafe() {
  return demoActiveUnsafe() ? demoHw499 : hw499Active;
}

void setLed(int pin, bool state) {
  digitalWrite(pin, state ? HIGH : LOW);
}

void allLedsOffUnsafe() {
  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}

void setAllLedsUnsafe(bool state) {
  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], state ? HIGH : LOW);
  }
}

String lightModeNameUnsafe() {
  if (lightMode == 0) return "auto";
  if (lightMode == 1) return "manual";
  if (lightMode == 2) return "cyber";
  if (lightMode == 3) return "scanner";
  if (lightMode == 4) return "pulse";
  if (lightMode == 5) return "police";
  if (lightMode == 6) return "wave";
  if (lightMode == 7) return "off";
  if (lightMode == 8) return "party";
  if (lightMode == 9) return "ghost";
  if (lightMode == 10) return "heartbeat";
  return "unknown";
}

unsigned long secondsSince(unsigned long eventMs) {
  if (eventMs == 0) return 0;
  return (millis() - eventMs) / 1000;
}

// =====================================================
// Historique
// =====================================================
void addEventUnsafe(String type, String severity, String message) {
  events[eventWriteIndex].ts = millis() / 1000;
  events[eventWriteIndex].type = type;
  events[eventWriteIndex].severity = severity;
  events[eventWriteIndex].message = message;

  eventWriteIndex = (eventWriteIndex + 1) % EVENT_COUNT;

  if (eventTotal < EVENT_COUNT) {
    eventTotal++;
  }
}

String buildEventsJsonUnsafe() {
  String json = "[";
  for (int i = 0; i < eventTotal; i++) {
    int index = eventWriteIndex - 1 - i;
    if (index < 0) index += EVENT_COUNT;

    if (i > 0) json += ",";

    json += "{";
    json += "\"ts\":" + String(events[index].ts) + ",";
    json += "\"type\":\"" + jsonEscape(events[index].type) + "\",";
    json += "\"severity\":\"" + jsonEscape(events[index].severity) + "\",";
    json += "\"message\":\"" + jsonEscape(events[index].message) + "\"";
    json += "}";
  }
  json += "]";
  return json;
}

// =====================================================
// Alertes + risque
// =====================================================
bool sensorErrorUnsafe() {
  return isnan(effectiveTempUnsafe()) || isnan(effectiveHumidityUnsafe()) || !gasSensorOk;
}

bool tempAlertUnsafe() {
  float t = effectiveTempUnsafe();
  return !isnan(t) && t >= tempLimit;
}

bool humidityAlertUnsafe() {
  float h = effectiveHumidityUnsafe();
  return !isnan(h) && (h < humLowLimit || h > humHighLimit);
}

bool gasAlertUnsafe() {
  return effectiveGasRawUnsafe() >= 0 && effectiveGasRawUnsafe() >= gasLimitRaw;
}

bool presenceAlertUnsafe() {
  return effectivePirUnsafe() || effectiveHw499Unsafe();
}

int riskScoreUnsafe() {
  int risk = 0;

  if (sensorErrorUnsafe()) risk += 20;
  if (tempAlertUnsafe()) risk += 25;
  if (humidityAlertUnsafe()) risk += 15;
  if (gasAlertUnsafe()) risk += 40;

  if (securityEnabled && effectivePirUnsafe()) risk += 35;
  else if (effectivePirUnsafe()) risk += 12;

  if (effectiveHw499Unsafe()) risk += 10;
  if (intrusionLatched) risk += 35;
  if (WiFi.status() != WL_CONNECTED) risk += 10;

  return clampInt(risk, 0, 100);
}

String riskStateUnsafe() {
  int risk = riskScoreUnsafe();

  if (sensorErrorUnsafe()) return "SENSOR_ERROR";
  if (risk >= 80) return "CRITICAL";
  if (risk >= 55) return "DANGER";
  if (risk >= 30) return "WARNING";
  if (risk >= 10) return "INFO";
  return "NOMINAL";
}

// =====================================================
// Statistiques
// =====================================================
void updateMinMaxUnsafe() {
  if (!isnan(lastTemperatureC)) {
    if (isnan(tempMin) || lastTemperatureC < tempMin) tempMin = lastTemperatureC;
    if (isnan(tempMax) || lastTemperatureC > tempMax) tempMax = lastTemperatureC;
  }

  if (!isnan(lastHumidity)) {
    if (isnan(humMin) || lastHumidity < humMin) humMin = lastHumidity;
    if (isnan(humMax) || lastHumidity > humMax) humMax = lastHumidity;
  }
}

// =====================================================
// Musiques second buzzer
// =====================================================
void stopMusicUnsafe() {
  if (musicToneOn || musicActive) {
    noTone(BUZZER_MUSIC_PIN);
  }

  currentSong = nullptr;
  currentSongLength = 0;
  currentSongIndex = 0;
  musicActive = false;
  musicToneOn = false;
  nextMusicChangeMs = 0;
  currentSongName = "none";
}

void startSongUnsafe(const Note* song, int length, const String& name) {
  if (!soundEnabled || nightMode) return;
  if (song == nullptr || length <= 0) return;

  // Si l'alarme forte est active, priorité à l'alarme.
  if (alarmBuzzerActive || millis() < alarmTestUntil) return;

  currentSong = song;
  currentSongLength = length;
  currentSongIndex = 0;
  musicActive = true;
  musicToneOn = false;
  nextMusicChangeMs = 0;
  currentSongName = name;
}

void updateMusicStepUnsafe() {
  if (!musicActive || currentSong == nullptr) return;

  if (!soundEnabled || nightMode || alarmBuzzerActive || millis() < alarmTestUntil) {
    stopMusicUnsafe();
    return;
  }

  unsigned long now = millis();

  if (nextMusicChangeMs != 0 && now < nextMusicChangeMs) {
    return;
  }

  if (musicToneOn) {
    noTone(BUZZER_MUSIC_PIN);
    musicToneOn = false;
    nextMusicChangeMs = now + 45;
    return;
  }

  if (currentSongIndex >= currentSongLength) {
    stopMusicUnsafe();
    return;
  }

  Note note = currentSong[currentSongIndex];
  currentSongIndex++;

  if (note.freq > 0) {
    tone(BUZZER_MUSIC_PIN, note.freq);
    musicToneOn = true;
  } else {
    noTone(BUZZER_MUSIC_PIN);
    musicToneOn = false;
  }

  nextMusicChangeMs = now + note.durationMs;
}

// =====================================================
// Buzzer alarme
// =====================================================
void startAlarmBuzzerUnsafe() {
  if (!soundEnabled) return;
  if (millis() < alarmSilencedUntilMs) return;

  alarmBuzzerActive = true;
}

void stopAlarmBuzzerUnsafe() {
  alarmBuzzerActive = false;

  if (alarmBuzzerStarted) {
    noTone(BUZZER_ALARM_PIN);
    alarmBuzzerStarted = false;
  }

  buzzerFreq = FREQ_MIN;
  buzzerUp = true;
}

void updateAlarmBuzzerStepUnsafe() {
  bool testActive = millis() < alarmTestUntil;

  if (!soundEnabled || nightMode) {
    if (alarmBuzzerStarted) {
      noTone(BUZZER_ALARM_PIN);
      alarmBuzzerStarted = false;
    }
    return;
  }

  if (!alarmBuzzerActive && !testActive) {
    if (alarmBuzzerStarted) {
      noTone(BUZZER_ALARM_PIN);
      alarmBuzzerStarted = false;
    }
    return;
  }

  tone(BUZZER_ALARM_PIN, buzzerFreq);
  alarmBuzzerStarted = true;

  if (buzzerUp) {
    buzzerFreq += FREQ_STEP;
    if (buzzerFreq >= FREQ_MAX) {
      buzzerFreq = FREQ_MAX;
      buzzerUp = false;
    }
  } else {
    buzzerFreq -= FREQ_STEP;
    if (buzzerFreq <= FREQ_MIN) {
      buzzerFreq = FREQ_MIN;
      buzzerUp = true;
    }
  }
}

// =====================================================
// Lecture capteurs
// =====================================================
void readDHT22() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  xSemaphoreTake(dataMutex, portMAX_DELAY);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Erreur lecture DHT22");
    dhtOk = false;
    dhtErrorCount++;
    lastTemperatureC = NAN;
    lastHumidity = NAN;
  } else {
    dhtOk = true;
    lastTemperatureC = temperature;
    lastHumidity = humidity;
    updateMinMaxUnsafe();
  }

  xSemaphoreGive(dataMutex);
}

void readGasSensor() {
  int raw = analogRead(GAS_AO_PIN);

  xSemaphoreTake(dataMutex, portMAX_DELAY);

  gasRaw = raw;

  // Si AO reste collé à 0 ou 4095 plusieurs lectures de suite,
  // on considère que le capteur MQ est débranché/saturé/suspect.
  if (raw <= 5 || raw >= 4090) {
    gasFaultStreak++;
  } else {
    gasFaultStreak = 0;
  }
  gasSensorOk = gasFaultStreak < 10;

  if (gasFiltered < 0) {
    gasFiltered = raw;
  } else {
    gasFiltered = gasFiltered * 0.85 + raw * 0.15;
  }

  gasPercent = (gasFiltered / 4095.0) * 100.0;

  if ((int)gasFiltered > gasMax) {
    gasMax = (int)gasFiltered;
  }

  xSemaphoreGive(dataMutex);
}

void readJoystick() {
  int xRaw = analogRead(JOY_X_PIN);
  int yRaw = analogRead(JOY_Y_PIN);
  bool sw = digitalRead(JOY_SW_PIN) == LOW;

  int x = map(xRaw, 0, 4095, -100, 100);
  int y = map(yRaw, 0, 4095, -100, 100);

  if (abs(xRaw - 2048) < JOY_DEADZONE) x = 0;
  if (abs(yRaw - 2048) < JOY_DEADZONE) y = 0;

  String direction = "CENTER";
  if (abs(x) > abs(y)) {
    if (x > 0) direction = "RIGHT";
    else if (x < 0) direction = "LEFT";
  } else {
    if (y > 0) direction = "DOWN";
    else if (y < 0) direction = "UP";
  }

  unsigned long now = millis();

  xSemaphoreTake(dataMutex, portMAX_DELAY);
  joyXRaw = xRaw;
  joyYRaw = yRaw;
  joyXNorm = x;
  joyYNorm = y;
  joyButtonPressed = sw;
  joyDirection = direction;

  if (joyButtonPressed && !previousJoyButtonPressed && now - lastJoyButtonMs > 250) {
    joyButtonCount++;
    lastJoyButtonMs = now;
    addEventUnsafe("JOYSTICK", "INFO", "Bouton joystick appuyé");
  }

  previousJoyButtonPressed = joyButtonPressed;
  xSemaphoreGive(dataMutex);
}

void readFastSensors() {
  bool pir = digitalRead(PIR_PIN) == HIGH;

  int hwRaw = digitalRead(HW499_PIN);
  bool hw = HW499_ACTIVE_LOW ? (hwRaw == LOW) : (hwRaw == HIGH);

  // Bouton câblé entre GPIO25 et GND.
  bool button = digitalRead(BUTTON_PIN) == LOW;
  unsigned long now = millis();

  xSemaphoreTake(dataMutex, portMAX_DELAY);

  pirMotion = pir;
  hw499Active = hw;
  buttonPressed = button;

  if (pirMotion && !previousPirState) {
    pirMotionCount++;
    lastPirMotionMs = now;

    addEventUnsafe("PIR", "INFO", "Mouvement détecté par le PIR");

    if (securityEnabled) {
      intrusionLatched = true;
      intrusionCount++;
      lastIntrusionMs = now;
      addEventUnsafe("SECURITY", "CRITICAL", "Intrusion détectée en mode sécurité");
    }

    startSongUnsafe(SONG_MOTION, sizeof(SONG_MOTION) / sizeof(SONG_MOTION[0]), "motion");
  }

  if (hw499Active && !previousHw499State) {
    hw499EventCount++;
    lastHw499EventMs = now;

    addEventUnsafe("HW499", "INFO", "Événement détecté sur le HW-499");
    startSongUnsafe(SONG_ROBOT, sizeof(SONG_ROBOT) / sizeof(SONG_ROBOT[0]), "hw499");
  }

  // Action bouton :
  // - si une intrusion est active : acquittement de l'alarme
  // - sinon : activation/désactivation du mode sécurité
  if (buttonPressed && !previousButtonPressed && now - lastButtonEdgeMs > 250) {
    lastButtonEdgeMs = now;
    buttonPressCount++;
    lastButtonPressMs = now;

    if (intrusionLatched) {
      intrusionLatched = false;
      alarmSilencedUntilMs = now + 30000;
      stopAlarmBuzzerUnsafe();
      addEventUnsafe("BUTTON", "INFO", "Bouton : alarme acquittée");
      startSongUnsafe(SONG_SUCCESS, sizeof(SONG_SUCCESS) / sizeof(SONG_SUCCESS[0]), "button-ack");
    } else {
      securityEnabled = !securityEnabled;
      addEventUnsafe("BUTTON", "INFO", securityEnabled ? "Bouton : sécurité activée" : "Bouton : sécurité désactivée");
      startSongUnsafe(SONG_ROBOT, sizeof(SONG_ROBOT) / sizeof(SONG_ROBOT[0]), "button");
    }
  }

  previousPirState = pirMotion;
  previousHw499State = hw499Active;
  previousButtonPressed = buttonPressed;

  xSemaphoreGive(dataMutex);
}

// =====================================================
// Supervision + événements
// =====================================================
void evaluateSystemUnsafe() {
  if (demoMode && millis() > demoUntilMs) {
    demoMode = false;
    addEventUnsafe("DEMO", "INFO", "Mode simulation terminé");
  }

  bool tAlert = tempAlertUnsafe();
  bool hAlert = humidityAlertUnsafe();
  bool gAlert = gasAlertUnsafe();
  bool pAlert = presenceAlertUnsafe();
  bool wifiOk = WiFi.status() == WL_CONNECTED;
  String currentRisk = riskStateUnsafe();

  if (tAlert && !prevTempAlert) {
    addEventUnsafe("TEMP", "WARNING", "Température supérieure au seuil");
    startSongUnsafe(SONG_ALERT, sizeof(SONG_ALERT) / sizeof(SONG_ALERT[0]), "temp-alert");
  }

  if (hAlert && !prevHumAlert) {
    addEventUnsafe("HUMIDITY", "WARNING", "Humidité hors limites");
  }

  if (gAlert && !prevGasAlert) {
    addEventUnsafe("GAS", "CRITICAL", "Gaz supérieur au seuil");
    startSongUnsafe(SONG_ALERT, sizeof(SONG_ALERT) / sizeof(SONG_ALERT[0]), "gas-alert");
  }

  if (pAlert && !prevPresenceAlert) {
    addEventUnsafe("PRESENCE", "INFO", "Présence ou inclinaison détectée");
  }

  if (wifiOk && !prevWifiOk) {
    addEventUnsafe("WIFI", "INFO", "WiFi connecté");
  }

  if (!wifiOk && prevWifiOk) {
    addEventUnsafe("WIFI", "WARNING", "WiFi déconnecté");
  }

  if (currentRisk != prevRiskState) {
    addEventUnsafe("RISK", currentRisk, "État global : " + currentRisk);
    prevRiskState = currentRisk;
  }

  prevTempAlert = tAlert;
  prevHumAlert = hAlert;
  prevGasAlert = gAlert;
  prevPresenceAlert = pAlert;
  prevWifiOk = wifiOk;

  bool danger = tAlert || gAlert || intrusionLatched || currentRisk == "CRITICAL" || currentRisk == "DANGER";

  if (danger) {
    startAlarmBuzzerUnsafe();
  } else {
    stopAlarmBuzzerUnsafe();
  }
}

// =====================================================
// Effets lumineux
// =====================================================
void applyAutoLedsUnsafe() {
  if (!lightEnabled || lightMode == 7) {
    allLedsOffUnsafe();
    return;
  }

  bool error = sensorErrorUnsafe();
  bool danger = tempAlertUnsafe() || gasAlertUnsafe() || intrusionLatched;
  bool warning = humidityAlertUnsafe() || presenceAlertUnsafe();
  bool wifiOk = WiFi.status() == WL_CONNECTED;

  if (nightMode) {
    allLedsOffUnsafe();
    digitalWrite(GREEN_LED, wifiOk ? HIGH : LOW);
    digitalWrite(RED_LED, danger ? HIGH : LOW);
    return;
  }

  if (error) {
    digitalWrite(RED_LED, lightBlinkState ? HIGH : LOW);
    digitalWrite(YELLOW_LED, lightBlinkState ? LOW : HIGH);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(WHITE_LED, LOW);
    return;
  }

  digitalWrite(GREEN_LED, wifiOk ? HIGH : LOW);
  digitalWrite(BLUE_LED, danger ? LOW : HIGH);
  digitalWrite(RED_LED, danger ? HIGH : LOW);
  digitalWrite(YELLOW_LED, warning ? HIGH : LOW);
  digitalWrite(WHITE_LED, securityEnabled ? HIGH : LOW);
}

void updateLightShowUnsafe() {
  if (!lightEnabled) {
    allLedsOffUnsafe();
    return;
  }

  if (lightMode == 0) {
    applyAutoLedsUnsafe();
    return;
  }

  if (lightMode == 1) {
    return;
  }

  if (lightMode == 7) {
    allLedsOffUnsafe();
    return;
  }

  if (nightMode && lightMode != 10) {
    applyAutoLedsUnsafe();
    return;
  }

  lightStep++;
  lightBlinkState = !lightBlinkState;

  if (lightMode == 2) {
    // Cyber : balayage rapide futuriste
    allLedsOffUnsafe();
    int index = lightStep % LED_COUNT;
    digitalWrite(LED_PINS[index], HIGH);
    digitalWrite(WHITE_LED, lightBlinkState ? HIGH : LOW);
  }
  else if (lightMode == 3) {
    // Scanner : aller-retour
    allLedsOffUnsafe();
    int period = (LED_COUNT * 2) - 2;
    int pos = lightStep % period;
    if (pos >= LED_COUNT) pos = period - pos;
    digitalWrite(LED_PINS[pos], HIGH);
  }
  else if (lightMode == 4) {
    // Pulse : toutes les LEDs alternent
    setAllLedsUnsafe(lightBlinkState);
  }
  else if (lightMode == 5) {
    // Police : rouge/bleu
    digitalWrite(RED_LED, lightBlinkState ? HIGH : LOW);
    digitalWrite(BLUE_LED, lightBlinkState ? LOW : HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(WHITE_LED, lightBlinkState ? HIGH : LOW);
  }
  else if (lightMode == 6) {
    // Wave : vague cumulative
    allLedsOffUnsafe();
    int level = lightStep % (LED_COUNT + 1);
    for (int i = 0; i < level; i++) {
      digitalWrite(LED_PINS[i], HIGH);
    }
  }
  else if (lightMode == 8) {
    // Party : motif pseudo-aléatoire déterministe
    digitalWrite(RED_LED, (lightStep % 2) == 0);
    digitalWrite(BLUE_LED, (lightStep % 3) == 0);
    digitalWrite(GREEN_LED, (lightStep % 4) < 2);
    digitalWrite(YELLOW_LED, (lightStep % 5) < 2);
    digitalWrite(WHITE_LED, (lightStep % 7) < 3);
  }
  else if (lightMode == 9) {
    // Ghost : clignotement blanc + bleu
    digitalWrite(WHITE_LED, lightBlinkState ? HIGH : LOW);
    digitalWrite(BLUE_LED, lightBlinkState ? LOW : HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
  }
  else if (lightMode == 10) {
    // Heartbeat : double battement rouge
    int phase = lightStep % 8;
    bool beat = (phase == 0 || phase == 2);
    digitalWrite(RED_LED, beat ? HIGH : LOW);
    digitalWrite(WHITE_LED, beat ? HIGH : LOW);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
  }
}

// =====================================================
// JSON API
// =====================================================
String buildFullJsonUnsafe() {
  int risk = riskScoreUnsafe();
  String state = riskStateUnsafe();

  String json = "{";
  json += "\"device\":\"ESP32-Othmane\",";
  json += "\"temp\":" + floatToJson(effectiveTempUnsafe(), 1) + ",";
  json += "\"humidity\":" + floatToJson(effectiveHumidityUnsafe(), 1) + ",";
  json += "\"tempMin\":" + floatToJson(tempMin, 1) + ",";
  json += "\"tempMax\":" + floatToJson(tempMax, 1) + ",";
  json += "\"humMin\":" + floatToJson(humMin, 1) + ",";
  json += "\"humMax\":" + floatToJson(humMax, 1) + ",";
  json += "\"gasRaw\":" + String(effectiveGasRawUnsafe() < 0 ? 0 : (int)effectiveGasRawUnsafe()) + ",";
  json += "\"gasPercent\":" + String(effectiveGasPercentUnsafe(), 1) + ",";
  json += "\"gasMax\":" + String(gasMax) + ",";
  json += "\"pir\":" + boolToJson(effectivePirUnsafe()) + ",";
  json += "\"hw499\":" + boolToJson(effectiveHw499Unsafe()) + ",";
  json += "\"pirCount\":" + String(pirMotionCount) + ",";
  json += "\"hw499Count\":" + String(hw499EventCount) + ",";
  json += "\"intrusionCount\":" + String(intrusionCount) + ",";
  json += "\"lastPirAgo\":" + String(secondsSince(lastPirMotionMs)) + ",";
  json += "\"lastHw499Ago\":" + String(secondsSince(lastHw499EventMs)) + ",";
  json += "\"lastIntrusionAgo\":" + String(secondsSince(lastIntrusionMs)) + ",";
  json += "\"button\":" + boolToJson(buttonPressed) + ",";
  json += "\"buttonCount\":" + String(buttonPressCount) + ",";
  json += "\"lastButtonAgo\":" + String(secondsSince(lastButtonPressMs)) + ",";
  json += "\"joyX\":" + String(joyXNorm) + ",";
  json += "\"joyY\":" + String(joyYNorm) + ",";
  json += "\"joyButton\":" + boolToJson(joyButtonPressed) + ",";
  json += "\"joyDirection\":\"" + joyDirection + "\",";
  json += "\"tempAlert\":" + boolToJson(tempAlertUnsafe()) + ",";
  json += "\"humidityAlert\":" + boolToJson(humidityAlertUnsafe()) + ",";
  json += "\"gasAlert\":" + boolToJson(gasAlertUnsafe()) + ",";
  json += "\"presenceAlert\":" + boolToJson(presenceAlertUnsafe()) + ",";
  json += "\"sensorError\":" + boolToJson(sensorErrorUnsafe()) + ",";
  json += "\"riskScore\":" + String(risk) + ",";
  json += "\"riskState\":\"" + state + "\",";
  json += "\"manualMode\":" + boolToJson(manualMode) + ",";
  json += "\"securityEnabled\":" + boolToJson(securityEnabled) + ",";
  json += "\"intrusionLatched\":" + boolToJson(intrusionLatched) + ",";
  json += "\"nightMode\":" + boolToJson(nightMode) + ",";
  json += "\"ecoMode\":" + boolToJson(ecoMode) + ",";
  json += "\"soundEnabled\":" + boolToJson(soundEnabled) + ",";
  json += "\"lightEnabled\":" + boolToJson(lightEnabled) + ",";
  json += "\"lightMode\":\"" + lightModeNameUnsafe() + "\",";
  json += "\"alarmBuzzer\":" + boolToJson(alarmBuzzerActive || millis() < alarmTestUntil) + ",";
  json += "\"musicActive\":" + boolToJson(musicActive) + ",";
  json += "\"song\":\"" + currentSongName + "\",";
  json += "\"demoMode\":" + boolToJson(demoActiveUnsafe()) + ",";
  json += "\"mqttEnabled\":" + boolToJson(mqttEnabled) + ",";
  json += "\"mqttConnected\":" + boolToJson(mqttConnected) + ",";
  json += "\"mqttBase\":\"" + jsonEscape(mqttBaseTopic) + "\",";
  json += "\"queueWaiting\":" + String(sensorQueue == nullptr ? 0 : uxQueueMessagesWaiting(sensorQueue)) + ",";
  json += "\"queueFree\":" + String(sensorQueue == nullptr ? 0 : uxQueueSpacesAvailable(sensorQueue)) + ",";
  json += "\"queueDropped\":" + String(queueDroppedCount) + ",";
  json += "\"offlineCount\":" + String(offlineCount) + ",";
  json += "\"offlineDropped\":" + String(offlineDroppedCount) + ",";
  json += "\"mqttPublished\":" + String(mqttPublishedCount) + ",";
  json += "\"mqttFailed\":" + String(mqttFailedCount) + ",";
  json += "\"lastPublishLatency\":" + String(lastPublishLatencyMs) + ",";
  json += "\"avgPublishLatency\":" + String(mqttPublishedCount == 0 ? 0 : publishLatencySumMs / mqttPublishedCount) + ",";
  json += "\"littleFsOk\":" + boolToJson(littleFsOk) + ",";
  json += "\"offlineFileCount\":" + String(offlineFileCount) + ",";
  json += "\"offlineFileBytes\":" + String(offlineFileBytes) + ",";
  json += "\"offlineFileReplayed\":" + String(offlineFileReplayed) + ",";
  json += "\"offlineFileWriteErrors\":" + String(offlineFileWriteErrors) + ",";
  json += "\"historyFileCount\":" + String(historyFileCount) + ",";
  json += "\"historyFileBytes\":" + String(historyFileBytes) + ",";
  json += "\"historyFileWriteErrors\":" + String(historyFileWriteErrors) + ",";
  json += "\"historyFileRotations\":" + String(historyFileRotations) + ",";
  json += "\"mqttBlackoutActive\":" + boolToJson(mqttBlackoutActive && mqttBlackoutUntilMs > millis()) + ",";
  json += "\"mqttBlackoutRemaining\":" + String(mqttBlackoutActive && mqttBlackoutUntilMs > millis() ? (mqttBlackoutUntilMs - millis()) / 1000 : 0) + ",";
  json += "\"mqttBlackoutCount\":" + String(mqttBlackoutCount) + ",";
  json += "\"networkForcedOff\":" + boolToJson(networkForcedOff) + ",";
  json += "\"wifiOffRemaining\":" + String(networkForcedOff && wifiOffUntilMs > millis() ? (wifiOffUntilMs - millis()) / 1000 : 0) + ",";
  json += "\"wifiCutCount\":" + String(wifiCutCount) + ",";
  json += "\"dhtOk\":" + boolToJson(dhtOk) + ",";
  json += "\"dhtErrorCount\":" + String(dhtErrorCount) + ",";
  json += "\"gasSensorOk\":" + boolToJson(gasSensorOk) + ",";
  json += "\"wifi\":" + boolToJson(WiFi.status() == WL_CONNECTED) + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";

  return json;
}

String buildJoystickJsonUnsafe() {
  String json = "{";
  json += "\"xRaw\":" + String(joyXRaw) + ",";
  json += "\"yRaw\":" + String(joyYRaw) + ",";
  json += "\"x\":" + String(joyXNorm) + ",";
  json += "\"y\":" + String(joyYNorm) + ",";
  json += "\"button\":" + boolToJson(joyButtonPressed) + ",";
  json += "\"buttonCount\":" + String(joyButtonCount) + ",";
  json += "\"direction\":\"" + joyDirection + "\"";
  json += "}";
  return json;
}

String buildPerformanceJson() {
  String json = "{";
  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"minHeap\":" + String(ESP.getMinFreeHeap()) + ",";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"queueWaiting\":" + String(sensorQueue == nullptr ? 0 : uxQueueMessagesWaiting(sensorQueue)) + ",";
  json += "\"queueFree\":" + String(sensorQueue == nullptr ? 0 : uxQueueSpacesAvailable(sensorQueue)) + ",";
  json += "\"offlineFileCount\":" + String(offlineFileCount) + ",";
  json += "\"lastPublishLatency\":" + String(lastPublishLatencyMs) + ",";
  json += "\"mqttPublished\":" + String(mqttPublishedCount) + ",";
  json += "\"tasks\":[";

  for (int i = 0; i < PERF_COUNT; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"name\":\"" + String(PERF_NAMES[i]) + "\",";
    json += "\"core\":" + String(PERF_CORES[i]) + ",";
    json += "\"priority\":" + String(PERF_PRIORITIES[i]) + ",";
    json += "\"loops\":" + String(perfLoops[i]) + ",";
    json += "\"runtimeUs\":" + String(perfRuntimeUs[i]);
    json += "}";
  }

  json += "]}";
  return json;
}

String buildStatusJsonUnsafe() {
  String json = "{";
  json += "\"device\":\"ESP32-Othmane\",";
  json += "\"risk\":" + String(riskScoreUnsafe()) + ",";
  json += "\"state\":\"" + riskStateUnsafe() + "\",";
  json += "\"queueWaiting\":" + String(sensorQueue == nullptr ? 0 : uxQueueMessagesWaiting(sensorQueue)) + ",";
  json += "\"offlineCount\":" + String(offlineCount) + ",";
  json += "\"lastPublishLatency\":" + String(lastPublishLatencyMs) + ",";
  json += "\"alerts\":{";
  json += "\"temperature\":" + boolToJson(tempAlertUnsafe()) + ",";
  json += "\"humidity\":" + boolToJson(humidityAlertUnsafe()) + ",";
  json += "\"gas\":" + boolToJson(gasAlertUnsafe()) + ",";
  json += "\"motion\":" + boolToJson(effectivePirUnsafe()) + ",";
  json += "\"intrusion\":" + boolToJson(intrusionLatched);
  json += "},";
  json += "\"mode\":{";
  json += "\"security\":" + boolToJson(securityEnabled) + ",";
  json += "\"night\":" + boolToJson(nightMode) + ",";
  json += "\"eco\":" + boolToJson(ecoMode);
  json += "}";
  json += "}";

  return json;
}


SensorPacket buildCurrentSensorPacketUnsafe() {
  SensorPacket packet;
  packet.seq = ++sensorSequence;
  packet.createdMs = millis();
  packet.temp = lastTemperatureC;
  packet.humidity = lastHumidity;
  packet.gasRaw = gasFiltered < 0 ? 0 : (int)gasFiltered;
  packet.gasPercent = gasPercent;
  packet.pir = pirMotion;
  packet.hw499 = hw499Active;
  packet.button = buttonPressed;
  packet.wifiOk = WiFi.status() == WL_CONNECTED;
  packet.mqttOk = mqttConnected;
  packet.networkForced = networkForcedOff;
  packet.mqttBlackout = mqttBlackoutActive && mqttBlackoutUntilMs > millis();
  packet.dhtOk = dhtOk;
  packet.gasOk = gasSensorOk;
  packet.risk = (uint8_t)riskScoreUnsafe();

  String state = riskStateUnsafe();
  state.toCharArray(packet.state, sizeof(packet.state));

  return packet;
}

String buildPacketJson(const SensorPacket &packet, bool replay) {
  String source = "live";
  if (replay) source = "replay";
  else if (packet.networkForced || packet.mqttBlackout || !packet.wifiOk || !packet.mqttOk) source = "offline";

  String json = "{";
  json += "\"device\":\"ESP32-Othmane\",";
  json += "\"seq\":" + String(packet.seq) + ",";
  json += "\"createdMs\":" + String(packet.createdMs) + ",";
  json += "\"mqttReplay\":" + boolToJson(replay) + ",";
  json += "\"source\":\"" + source + "\",";
  json += "\"temp\":" + floatToJson(packet.temp, 1) + ",";
  json += "\"humidity\":" + floatToJson(packet.humidity, 1) + ",";
  json += "\"gasRaw\":" + String(packet.gasRaw) + ",";
  json += "\"gasPercent\":" + String(packet.gasPercent, 1) + ",";
  json += "\"pir\":" + boolToJson(packet.pir) + ",";
  json += "\"hw499\":" + boolToJson(packet.hw499) + ",";
  json += "\"button\":" + boolToJson(packet.button) + ",";
  json += "\"wifiOk\":" + boolToJson(packet.wifiOk) + ",";
  json += "\"mqttOk\":" + boolToJson(packet.mqttOk) + ",";
  json += "\"networkForced\":" + boolToJson(packet.networkForced) + ",";
  json += "\"mqttBlackout\":" + boolToJson(packet.mqttBlackout) + ",";
  json += "\"dhtOk\":" + boolToJson(packet.dhtOk) + ",";
  json += "\"gasOk\":" + boolToJson(packet.gasOk) + ",";
  json += "\"risk\":" + String(packet.risk) + ",";
  json += "\"state\":\"" + String(packet.state) + "\",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";

  return json;
}


void refreshOfflineDbStatsUnsafe() {
  if (!littleFsOk || !LittleFS.exists(OFFLINE_DB_FILE)) {
    offlineFileBytes = 0;
    offlineFileCount = 0;
    return;
  }

  File file = LittleFS.open(OFFLINE_DB_FILE, "r");
  if (!file) {
    offlineFileBytes = 0;
    offlineFileCount = 0;
    return;
  }

  offlineFileBytes = file.size();
  unsigned long lines = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 2) lines++;
  }
  file.close();
  offlineFileCount = lines;
}

void resetOfflineDatabaseUnsafe() {
  if (littleFsOk && LittleFS.exists(OFFLINE_DB_FILE)) {
    LittleFS.remove(OFFLINE_DB_FILE);
  }
  offlineFileCount = 0;
  offlineFileBytes = 0;
}

bool appendOfflinePayloadUnsafe(const String &payload) {
  if (!littleFsOk) return false;

  refreshOfflineDbStatsUnsafe();

  // Protection mémoire : si la base locale devient trop grosse, on la vide.
  // Dans le rapport : c'est un buffer circulaire simplifié ; amélioration possible = conserver les plus anciennes pages.
  if (offlineFileBytes > OFFLINE_DB_MAX_BYTES) {
    offlineDroppedCount += offlineFileCount;
    resetOfflineDatabaseUnsafe();
    addEventUnsafe("DB", "WARNING", "Base offline pleine : remise à zéro du fichier");
  }

  File file = LittleFS.open(OFFLINE_DB_FILE, "a");
  if (!file) {
    offlineFileWriteErrors++;
    return false;
  }

  file.println(payload);
  file.close();

  offlineFileCount++;
  offlineFileBytes += payload.length() + 1;
  return true;
}

bool popOfflinePayloadUnsafe(String &payload) {
  payload = "";
  if (!littleFsOk || !LittleFS.exists(OFFLINE_DB_FILE)) return false;

  File file = LittleFS.open(OFFLINE_DB_FILE, "r");
  if (!file || file.size() == 0) {
    if (file) file.close();
    resetOfflineDatabaseUnsafe();
    return false;
  }

  payload = file.readStringUntil('\n');
  String rest = file.readString();
  file.close();

  LittleFS.remove(OFFLINE_DB_FILE);
  if (rest.length() > 0) {
    File out = LittleFS.open(OFFLINE_DB_FILE, "w");
    if (out) {
      out.print(rest);
      out.close();
    }
  }

  refreshOfflineDbStatsUnsafe();
  return payload.length() > 2;
}


void refreshHistoryDbStatsUnsafe() {
  if (!littleFsOk || !LittleFS.exists(HISTORY_DB_FILE)) {
    historyFileBytes = 0;
    historyFileCount = 0;
    return;
  }

  File file = LittleFS.open(HISTORY_DB_FILE, "r");
  if (!file) {
    historyFileBytes = 0;
    historyFileCount = 0;
    return;
  }

  historyFileBytes = file.size();
  unsigned long lines = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 2) lines++;
  }
  file.close();
  historyFileCount = lines;
}

void resetHistoryDatabaseUnsafe() {
  if (littleFsOk && LittleFS.exists(HISTORY_DB_FILE)) {
    LittleFS.remove(HISTORY_DB_FILE);
  }
  historyFileCount = 0;
  historyFileBytes = 0;
}

bool appendHistoryPayloadUnsafe(const String &payload) {
  if (!littleFsOk) return false;

  // On évite de rescanner tout le fichier à chaque mesure.
  // Les compteurs sont chargés au démarrage puis mis à jour après chaque écriture.
  if (historyFileBytes == 0 && LittleFS.exists(HISTORY_DB_FILE)) {
    refreshHistoryDbStatsUnsafe();
  }

  // Historique borné : si le fichier devient trop gros, on repart à zéro.
  // C'est volontairement simple pour rester explicable en soutenance.
  if (historyFileBytes > HISTORY_DB_MAX_BYTES) {
    resetHistoryDatabaseUnsafe();
    historyFileRotations++;
    addEventUnsafe("HISTORY", "WARNING", "Historique local plein : rotation du fichier");
  }

  File file = LittleFS.open(HISTORY_DB_FILE, "a");
  if (!file) {
    historyFileWriteErrors++;
    return false;
  }

  file.println(payload);
  file.close();

  historyFileCount++;
  historyFileBytes += payload.length() + 1;
  return true;
}

bool appendHistoryPacketUnsafe(const SensorPacket &packet) {
  return appendHistoryPayloadUnsafe(buildPacketJson(packet, false));
}

String buildHistoryJsonUnsafe(int limit) {
  if (limit <= 0) limit = 60;
  if (limit > HISTORY_API_MAX_POINTS) limit = HISTORY_API_MAX_POINTS;

  String empty = "[]";
  if (!littleFsOk || !LittleFS.exists(HISTORY_DB_FILE)) return empty;

  File file = LittleFS.open(HISTORY_DB_FILE, "r");
  if (!file) return empty;

  // On garde seulement les dernières lignes demandées pour éviter de saturer la RAM.
  String lines[HISTORY_API_MAX_POINTS];
  int count = 0;
  int writeIndex = 0;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() < 2) continue;

    lines[writeIndex] = line;
    writeIndex = (writeIndex + 1) % limit;
    if (count < limit) count++;
  }
  file.close();

  String json = "[";
  for (int i = 0; i < count; i++) {
    int index = writeIndex - count + i;
    while (index < 0) index += limit;
    index = index % limit;

    if (i > 0) json += ",";
    json += lines[index];
  }
  json += "]";
  return json;
}

bool publishPayloadToMqtt(const String &payload, bool replay) {
  String topic;

  xSemaphoreTake(dataMutex, portMAX_DELAY);
  topic = mqttBaseTopic + "/data";
  xSemaphoreGive(dataMutex);

  bool ok = mqttClient.publish(topic.c_str(), payload.c_str());

  xSemaphoreTake(dataMutex, portMAX_DELAY);
  if (ok) {
    mqttPublishedCount++;
    offlineFileReplayed += replay ? 1 : 0;
  } else {
    mqttFailedCount++;
  }
  xSemaphoreGive(dataMutex);

  return ok;
}

void storeOfflinePacketUnsafe(const SensorPacket &packet) {
  // Priorité : base locale LittleFS. Elle survit à une panne WiFi longue
  // et même à un redémarrage si le fichier n'est pas supprimé.
  if (littleFsOk) {
    String payload = buildPacketJson(packet, true);
    if (appendOfflinePayloadUnsafe(payload)) {
      return;
    }
  }

  // Fallback RAM si LittleFS indisponible. Capacité plus faible.
  offlineBuffer[offlineWriteIndex] = packet;
  offlineWriteIndex = (offlineWriteIndex + 1) % OFFLINE_CAPACITY;

  if (offlineCount < OFFLINE_CAPACITY) {
    offlineCount++;
  } else {
    offlineDroppedCount++;
  }
}

bool popOfflinePacketUnsafe(SensorPacket &packet) {
  if (offlineCount <= 0) return false;

  int oldestIndex = offlineWriteIndex - offlineCount;
  if (oldestIndex < 0) oldestIndex += OFFLINE_CAPACITY;

  packet = offlineBuffer[oldestIndex];
  offlineCount--;

  return true;
}

void sendLatestSensorPacketToQueue() {
  if (sensorQueue == nullptr) return;

  SensorPacket packet;

  xSemaphoreTake(dataMutex, portMAX_DELAY);
  packet = buildCurrentSensorPacketUnsafe();
  appendHistoryPacketUnsafe(packet);
  xSemaphoreGive(dataMutex);

  if (xQueueSend(sensorQueue, &packet, 0) != pdTRUE) {
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    queueDroppedCount++;
    storeOfflinePacketUnsafe(packet);
    addEventUnsafe("QUEUE", "WARNING", "Queue capteurs pleine : mesure stockée offline");
    xSemaphoreGive(dataMutex);
  }
}

bool publishPacketToMqtt(const SensorPacket &packet, bool replay) {
  String topic;

  xSemaphoreTake(dataMutex, portMAX_DELAY);
  topic = mqttBaseTopic + "/data";
  xSemaphoreGive(dataMutex);

  String payload = buildPacketJson(packet, replay);
  bool ok = mqttClient.publish(topic.c_str(), payload.c_str());

  unsigned long latency = millis() - packet.createdMs;

  xSemaphoreTake(dataMutex, portMAX_DELAY);
  lastPublishLatencyMs = latency;

  if (ok) {
    mqttPublishedCount++;
    publishLatencySumMs += latency;
  } else {
    mqttFailedCount++;
  }

  xSemaphoreGive(dataMutex);

  return ok;
}

// =====================================================
// Commandes
// =====================================================
void applyLightModeUnsafe(String mode) {
  if (mode == "auto") {
    manualMode = false;
    lightMode = 0;
  }
  else if (mode == "manual") {
    manualMode = true;
    lightMode = 1;
  }
  else if (mode == "cyber") lightMode = 2;
  else if (mode == "scanner") lightMode = 3;
  else if (mode == "pulse") lightMode = 4;
  else if (mode == "police") lightMode = 5;
  else if (mode == "wave") lightMode = 6;
  else if (mode == "off") {
    manualMode = true;
    lightMode = 7;
    allLedsOffUnsafe();
  }
  else if (mode == "party") lightMode = 8;
  else if (mode == "ghost") lightMode = 9;
  else if (mode == "heartbeat") lightMode = 10;

  lightStep = 0;
}

void playSongByNameUnsafe(String song) {
  if (song == "startup") {
    addEventUnsafe("JOYSTICK", "INFO", "HW-504 : VRX=GPIO32, VRY=GPIO35, SW=GPIO23");
  startSongUnsafe(SONG_STARTUP, sizeof(SONG_STARTUP) / sizeof(SONG_STARTUP[0]), "startup");
  }
  else if (song == "success") {
    startSongUnsafe(SONG_SUCCESS, sizeof(SONG_SUCCESS) / sizeof(SONG_SUCCESS[0]), "success");
  }
  else if (song == "cyber") {
    startSongUnsafe(SONG_CYBER, sizeof(SONG_CYBER) / sizeof(SONG_CYBER[0]), "cyber");
  }
  else if (song == "alert") {
    startSongUnsafe(SONG_ALERT, sizeof(SONG_ALERT) / sizeof(SONG_ALERT[0]), "alert");
  }
  else if (song == "retro") {
    startSongUnsafe(SONG_RETRO, sizeof(SONG_RETRO) / sizeof(SONG_RETRO[0]), "retro");
  }
  else if (song == "party") {
    startSongUnsafe(SONG_PARTY, sizeof(SONG_PARTY) / sizeof(SONG_PARTY[0]), "party");
  }
  else if (song == "robot") {
    startSongUnsafe(SONG_ROBOT, sizeof(SONG_ROBOT) / sizeof(SONG_ROBOT[0]), "robot");
  }
  else if (song == "laugh") {
    startSongUnsafe(SONG_LAUGH, sizeof(SONG_LAUGH) / sizeof(SONG_LAUGH[0]), "laugh");
  }
  else if (song == "motion") {
    startSongUnsafe(SONG_MOTION, sizeof(SONG_MOTION) / sizeof(SONG_MOTION[0]), "motion");
  }
  else if (song == "stop") {
    stopMusicUnsafe();
  }
}

void startDemoUnsafe(String type) {
  demoMode = true;
  demoUntilMs = millis() + 30000;

  if (type == "temp") {
    demoTemperature = tempLimit + 5;
    demoHumidity = 55;
    demoGasRaw = 800;
    demoPir = false;
    demoHw499 = false;
  }
  else if (type == "humidity") {
    demoTemperature = 24;
    demoHumidity = humHighLimit + 10;
    demoGasRaw = 800;
    demoPir = false;
    demoHw499 = false;
  }
  else if (type == "gas") {
    demoTemperature = 24;
    demoHumidity = 50;
    demoGasRaw = gasLimitRaw + 700;
    if (demoGasRaw > 4095) demoGasRaw = 4095;
    demoPir = false;
    demoHw499 = false;
  }
  else if (type == "motion") {
    demoTemperature = 24;
    demoHumidity = 50;
    demoGasRaw = 800;
    demoPir = true;
    demoHw499 = false;
  }
  else if (type == "hw499") {
    demoTemperature = 24;
    demoHumidity = 50;
    demoGasRaw = 800;
    demoPir = false;
    demoHw499 = true;
  }
  else {
    demoTemperature = tempLimit + 8;
    demoHumidity = humHighLimit + 5;
    demoGasRaw = gasLimitRaw + 700;
    if (demoGasRaw > 4095) demoGasRaw = 4095;
    demoPir = true;
    demoHw499 = true;
  }

  addEventUnsafe("DEMO", "INFO", "Simulation activée : " + type);
  startSongUnsafe(SONG_CYBER, sizeof(SONG_CYBER) / sizeof(SONG_CYBER[0]), "demo");
}

// =====================================================
// MQTT
// =====================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  message.trim();

  xSemaphoreTake(dataMutex, portMAX_DELAY);

  addEventUnsafe("MQTT", "INFO", "Commande reçue : " + message);

  if (message == "securityOn") securityEnabled = true;
  else if (message == "securityOff") securityEnabled = false;
  else if (message == "nightOn") nightMode = true;
  else if (message == "nightOff") nightMode = false;
  else if (message == "ecoOn") ecoMode = true;
  else if (message == "ecoOff") ecoMode = false;
  else if (message == "ack") {
    intrusionLatched = false;
    alarmSilencedUntilMs = millis() + 30000;
    stopAlarmBuzzerUnsafe();
  }
  else if (message.startsWith("light:")) {
    applyLightModeUnsafe(message.substring(6));
  }
  else if (message.startsWith("music:")) {
    playSongByNameUnsafe(message.substring(6));
  }
  else if (message.startsWith("demo:")) {
    startDemoUnsafe(message.substring(5));
  }
  else if (message.startsWith("mqttBlackout:")) {
    int minutes = boundedInt(message.substring(13), 5, 1, 120);
    mqttEnabled = true;
    mqttBlackoutActive = true;
    mqttBlackoutUntilMs = millis() + (unsigned long)minutes * 60UL * 1000UL;
    mqttBlackoutCount++;
    mqttConnected = false;
    if (mqttClient.connected()) mqttClient.disconnect();
  }
  else if (message == "mqttBlackoutOff") {
    mqttBlackoutActive = false;
    mqttBlackoutUntilMs = 0;
  }

  xSemaphoreGive(dataMutex);
}

bool mqttReconnect() {
  if (!mqttEnabled) return false;
  if (WiFi.status() != WL_CONNECTED) return false;

  uint64_t chipid = ESP.getEfuseMac();
  char clientId[40];
  snprintf(clientId, sizeof(clientId), "ESP32-%04X%08X",
           (uint16_t)(chipid >> 32), (uint32_t)chipid);

  if (mqttClient.connect(clientId)) {
    String cmdTopic = mqttBaseTopic + "/cmd";
    mqttClient.subscribe(cmdTopic.c_str());

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    mqttConnected = true;
    addEventUnsafe("MQTT", "INFO", "MQTT connecté");
    xSemaphoreGive(dataMutex);

    return true;
  }

  xSemaphoreTake(dataMutex, portMAX_DELAY);
  mqttConnected = false;
  xSemaphoreGive(dataMutex);

  return false;
}

// =====================================================
// API Web
// =====================================================
void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireLogin(request)) return;
    request->send(200, "text/html", index_html);
  });

  server.on("/api/sensors", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireLogin(request)) return;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    String json = buildFullJsonUnsafe();
    xSemaphoreGive(dataMutex);

    request->send(200, "application/json", json);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireLogin(request)) return;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    String json = buildStatusJsonUnsafe();
    xSemaphoreGive(dataMutex);

    request->send(200, "application/json", json);
  });

  server.on("/api/joystick", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireLogin(request)) return;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    String json = buildJoystickJsonUnsafe();
    xSemaphoreGive(dataMutex);

    request->send(200, "application/json", json);
  });

  server.on("/api/performance", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireLogin(request)) return;
    String json = buildPerformanceJson();
    request->send(200, "application/json", json);
  });

  server.on("/api/events", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireLogin(request)) return;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    String json = buildEventsJsonUnsafe();
    xSemaphoreGive(dataMutex);

    request->send(200, "application/json", json);
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireLogin(request)) return;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    String response = floatToJson(effectiveTempUnsafe(), 1);
    xSemaphoreGive(dataMutex);

    request->send(200, "text/plain", response);
  });

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireLogin(request)) return;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    String response = floatToJson(effectiveHumidityUnsafe(), 1);
    xSemaphoreGive(dataMutex);

    request->send(200, "text/plain", response);
  });

  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireLogin(request)) return;
    bool writeRequest = request->hasParam("tempLimit") ||
                        request->hasParam("humLow") ||
                        request->hasParam("humHigh") ||
                        request->hasParam("gasLimit") ||
                        request->hasParam("mqtt") ||
                        request->hasParam("mqttBase");

    if (writeRequest && !isAuthorized(request)) {
      sendForbidden(request);
      return;
    }

    xSemaphoreTake(dataMutex, portMAX_DELAY);

    if (request->hasParam("tempLimit")) {
      tempLimit = boundedFloat(request->getParam("tempLimit")->value(), tempLimit, 0.0, 80.0);
    }

    if (request->hasParam("humLow")) {
      humLowLimit = boundedFloat(request->getParam("humLow")->value(), humLowLimit, 0.0, 100.0);
    }

    if (request->hasParam("humHigh")) {
      humHighLimit = boundedFloat(request->getParam("humHigh")->value(), humHighLimit, 0.0, 100.0);
    }

    if (humLowLimit > humHighLimit) {
      float tmp = humLowLimit;
      humLowLimit = humHighLimit;
      humHighLimit = tmp;
    }

    if (request->hasParam("gasLimit")) {
      gasLimitRaw = boundedInt(request->getParam("gasLimit")->value(), gasLimitRaw, 0, 4095);
    }

    if (request->hasParam("mqtt")) {
      mqttEnabled = request->getParam("mqtt")->value().toInt() == 1;
    }

    if (request->hasParam("mqttBase")) {
      String requestedTopic = request->getParam("mqttBase")->value();
      if (isSafeTopic(requestedTopic)) {
        mqttBaseTopic = requestedTopic;
      } else {
        addEventUnsafe("SECURITY", "WARNING", "Topic MQTT refusé : caractères invalides");
      }
    }

    String json = "{";
    json += "\"tempLimit\":" + String(tempLimit, 1) + ",";
    json += "\"humLow\":" + String(humLowLimit, 1) + ",";
    json += "\"humHigh\":" + String(humHighLimit, 1) + ",";
    json += "\"gasLimit\":" + String(gasLimitRaw) + ",";
    json += "\"mqtt\":" + boolToJson(mqttEnabled) + ",";
    json += "\"mqttBase\":\"" + jsonEscape(mqttBaseTopic) + "\"";
    json += "}";

    xSemaphoreGive(dataMutex);

    request->send(200, "application/json", json);
  });

  server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireLogin(request)) return;

    int limit = 80;
    if (request->hasParam("limit")) {
      limit = boundedInt(request->getParam("limit")->value(), 80, 10, HISTORY_API_MAX_POINTS);
    }

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    String json = buildHistoryJsonUnsafe(limit);
    xSemaphoreGive(dataMutex);

    request->send(200, "application/json", json);
  });

  server.on("/api/history/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireLogin(request)) return;

    if (!littleFsOk || !LittleFS.exists(HISTORY_DB_FILE)) {
      request->send(200, "text/plain", "");
      return;
    }

    request->send(LittleFS, HISTORY_DB_FILE, "application/x-ndjson", true);
  });

  server.on("/api/history/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthorized(request)) {
      sendForbidden(request);
      return;
    }

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    resetHistoryDatabaseUnsafe();
    addEventUnsafe("HISTORY", "INFO", "Historique local vidé manuellement");
    xSemaphoreGive(dataMutex);

    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/db/stats", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireLogin(request)) return;

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    refreshOfflineDbStatsUnsafe();
    String json = "{";
    json += "\"littleFsOk\":" + boolToJson(littleFsOk) + ",";
    json += "\"file\":\"" + String(OFFLINE_DB_FILE) + "\",";
    json += "\"count\":" + String(offlineFileCount) + ",";
    json += "\"bytes\":" + String(offlineFileBytes) + ",";
    json += "\"replayed\":" + String(offlineFileReplayed) + ",";
    json += "\"writeErrors\":" + String(offlineFileWriteErrors) + ",";
    json += "\"maxBytes\":" + String(OFFLINE_DB_MAX_BYTES) + ",";
    refreshHistoryDbStatsUnsafe();
    json += "\"historyFile\":\"" + String(HISTORY_DB_FILE) + "\",";
    json += "\"historyCount\":" + String(historyFileCount) + ",";
    json += "\"historyBytes\":" + String(historyFileBytes) + ",";
    json += "\"historyMaxBytes\":" + String(HISTORY_DB_MAX_BYTES) + ",";
    json += "\"historyWriteErrors\":" + String(historyFileWriteErrors) + ",";
    json += "\"historyRotations\":" + String(historyFileRotations);
    json += "}";
    xSemaphoreGive(dataMutex);

    request->send(200, "application/json", json);
  });

  server.on("/api/db/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!requireLogin(request)) return;

    if (!littleFsOk || !LittleFS.exists(OFFLINE_DB_FILE)) {
      request->send(200, "text/plain", "");
      return;
    }

    request->send(LittleFS, OFFLINE_DB_FILE, "application/x-ndjson", true);
  });

  server.on("/api/db/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthorized(request)) {
      sendForbidden(request);
      return;
    }

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    resetOfflineDatabaseUnsafe();
    addEventUnsafe("DB", "INFO", "Base offline locale vidée manuellement");
    xSemaphoreGive(dataMutex);

    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/control", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthorized(request)) {
      sendForbidden(request);
      return;
    }

    String cmd = request->hasParam("cmd") ? request->getParam("cmd")->value() : "";

    xSemaphoreTake(dataMutex, portMAX_DELAY);

    if (cmd == "auto") {
      manualMode = false;
      lightMode = 0;
      addEventUnsafe("MODE", "INFO", "Mode automatique activé");
      playSongByNameUnsafe("success");
    }
    else if (cmd == "manual") {
      manualMode = true;
      lightMode = 1;
      stopAlarmBuzzerUnsafe();
      digitalWrite(WHITE_LED, HIGH);
      addEventUnsafe("MODE", "INFO", "Mode manuel activé");
      playSongByNameUnsafe("cyber");
    }
    else if (cmd == "alloff") {
      manualMode = true;
      lightMode = 7;
      stopAlarmBuzzerUnsafe();
      stopMusicUnsafe();
      allLedsOffUnsafe();
      addEventUnsafe("MODE", "INFO", "Tout éteint");
    }
    else if (cmd == "securityOn") {
      securityEnabled = true;
      addEventUnsafe("SECURITY", "INFO", "Mode sécurité activé");
      playSongByNameUnsafe("robot");
    }
    else if (cmd == "securityOff") {
      securityEnabled = false;
      intrusionLatched = false;
      stopAlarmBuzzerUnsafe();
      addEventUnsafe("SECURITY", "INFO", "Mode sécurité désactivé");
      playSongByNameUnsafe("success");
    }
    else if (cmd == "ack") {
      intrusionLatched = false;
      alarmSilencedUntilMs = millis() + 30000;
      stopAlarmBuzzerUnsafe();
      addEventUnsafe("ALARM", "INFO", "Alarme acquittée 30 secondes");
    }
    else if (cmd == "nightOn") {
      nightMode = true;
      addEventUnsafe("MODE", "INFO", "Mode nuit activé");
    }
    else if (cmd == "nightOff") {
      nightMode = false;
      addEventUnsafe("MODE", "INFO", "Mode nuit désactivé");
      playSongByNameUnsafe("success");
    }
    else if (cmd == "ecoOn") {
      ecoMode = true;
      addEventUnsafe("MODE", "INFO", "Mode économie activé");
    }
    else if (cmd == "ecoOff") {
      ecoMode = false;
      addEventUnsafe("MODE", "INFO", "Mode économie désactivé");
    }
    else if (cmd == "soundOn") {
      soundEnabled = true;
      playSongByNameUnsafe("success");
    }
    else if (cmd == "soundOff") {
      soundEnabled = false;
      stopMusicUnsafe();
      stopAlarmBuzzerUnsafe();
    }
    else if (cmd == "lightOn") {
      lightEnabled = true;
      playSongByNameUnsafe("success");
    }
    else if (cmd == "lightOff") {
      lightEnabled = false;
      allLedsOffUnsafe();
    }
    else if (cmd == "wifiOff") {
      int minutes = request->hasParam("minutes") ? request->getParam("minutes")->value().toInt() : 30;
      minutes = clampInt(minutes, 1, 120);
      networkForcedOff = true;
      wifiOffUntilMs = millis() + (unsigned long)minutes * 60UL * 1000UL;
      wifiCutCount++;
      mqttConnected = false;
      addEventUnsafe("WIFI", "WARNING", "WiFi coupé volontairement pour test offline " + String(minutes) + " min");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    }
    else if (cmd == "wifiOn") {
      networkForcedOff = false;
      wifiOffUntilMs = 0;
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      addEventUnsafe("WIFI", "INFO", "WiFi réactivé manuellement");
    }
    else if (cmd == "mqttBlackout") {
      int minutes = request->hasParam("minutes") ? boundedInt(request->getParam("minutes")->value(), 5, 1, 120) : 5;
      mqttEnabled = true;
      mqttBlackoutActive = true;
      mqttBlackoutUntilMs = millis() + (unsigned long)minutes * 60UL * 1000UL;
      mqttBlackoutCount++;
      if (mqttClient.connected()) mqttClient.disconnect();
      mqttConnected = false;
      addEventUnsafe("MQTT", "WARNING", "Simulation panne MQTT " + String(minutes) + " min");
    }
    else if (cmd == "mqttBlackoutOff") {
      mqttBlackoutActive = false;
      mqttBlackoutUntilMs = 0;
      addEventUnsafe("MQTT", "INFO", "Simulation panne MQTT arrêtée");
    }
    else if (cmd == "resetStats") {
      pirMotionCount = 0;
      hw499EventCount = 0;
      intrusionCount = 0;
      lastPirMotionMs = 0;
      lastHw499EventMs = 0;
      lastIntrusionMs = 0;
      tempMin = NAN;
      tempMax = NAN;
      humMin = NAN;
      humMax = NAN;
      gasMax = 0;
      intrusionLatched = false;
      addEventUnsafe("SYSTEM", "INFO", "Statistiques réinitialisées");
      playSongByNameUnsafe("success");
    }
    else if (cmd == "clearEvents") {
      eventWriteIndex = 0;
      eventTotal = 0;
      addEventUnsafe("SYSTEM", "INFO", "Historique vidé");
    }
    else if (cmd == "testAlarm") {
      alarmTestUntil = millis() + 3000;
      addEventUnsafe("TEST", "WARNING", "Test alarme buzzer");
    }
    else if (cmd == "calibrateGas") {
      gasLimitRaw = (effectiveGasRawUnsafe() < 0 ? gasRaw : (int)effectiveGasRawUnsafe()) + 400;
      if (gasLimitRaw > 4095) gasLimitRaw = 4095;
      addEventUnsafe("GAS", "INFO", "Seuil gaz calibré automatiquement");
      playSongByNameUnsafe("success");
    }
    else if (cmd == "light") {
      String mode = request->hasParam("mode") ? request->getParam("mode")->value() : "auto";
      applyLightModeUnsafe(mode);
      addEventUnsafe("LIGHT", "INFO", "Mode lumière : " + mode);
      playSongByNameUnsafe("cyber");
    }
    else if (cmd == "music") {
      String song = request->hasParam("song") ? request->getParam("song")->value() : "success";
      playSongByNameUnsafe(song);
      addEventUnsafe("MUSIC", "INFO", "Musique : " + song);
    }
    else if (cmd == "demo") {
      String type = request->hasParam("type") ? request->getParam("type")->value() : "all";
      startDemoUnsafe(type);
    }
    else if (cmd == "demoOff") {
      demoMode = false;
      addEventUnsafe("DEMO", "INFO", "Simulation désactivée");
    }
    else if (cmd == "led") {
      manualMode = true;
      lightMode = 1;

      String led = request->hasParam("led") ? request->getParam("led")->value() : "";
      bool state = request->hasParam("state") ? request->getParam("state")->value().toInt() == 1 : false;

      if (led == "red") digitalWrite(RED_LED, state ? HIGH : LOW);
      else if (led == "blue") digitalWrite(BLUE_LED, state ? HIGH : LOW);
      else if (led == "green") digitalWrite(GREEN_LED, state ? HIGH : LOW);
      else if (led == "yellow") digitalWrite(YELLOW_LED, state ? HIGH : LOW);
      else if (led == "white") digitalWrite(WHITE_LED, state ? HIGH : LOW);
    }

    String json = "{";
    json += "\"ok\":true,";
    json += "\"cmd\":\"" + jsonEscape(cmd) + "\",";
    json += "\"manualMode\":" + boolToJson(manualMode) + ",";
    json += "\"securityEnabled\":" + boolToJson(securityEnabled) + ",";
    json += "\"lightMode\":\"" + lightModeNameUnsafe() + "\",";
    json += "\"risk\":" + String(riskScoreUnsafe()) + ",";
    json += "\"riskState\":\"" + riskStateUnsafe() + "\",";
    json += "\"gasLimit\":" + String(gasLimitRaw);
    json += "}";

    xSemaphoreGive(dataMutex);

    request->send(200, "application/json", json);
  });

  server.begin();
}

// =====================================================
// Tâches FreeRTOS
// =====================================================
void taskWifi(void *parameter) {
  // Priorité 1 : la connexion réseau est utile mais non critique pour l'acquisition.
  // Si le WiFi tombe, TaskSensors continue et TaskMQTT stocke offline.
  for (;;) {
    unsigned long perfStart = micros();
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    bool forcedOff = networkForcedOff;
    unsigned long untilMs = wifiOffUntilMs;
    xSemaphoreGive(dataMutex);

    if (forcedOff) {
      if (millis() >= untilMs) {
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        networkForcedOff = false;
        wifiOffUntilMs = 0;
        addEventUnsafe("WIFI", "INFO", "Fin coupure WiFi volontaire : reconnexion");
        xSemaphoreGive(dataMutex);

        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
      } else {
        if (WiFi.getMode() != WIFI_OFF) {
          WiFi.disconnect(true);
          WiFi.mode(WIFI_OFF);
        }
      }
    } else if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi déconnecté : reconnexion...");
      WiFi.mode(WIFI_STA);
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }

    recordTaskRuntime(PERF_WIFI, perfStart);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void taskSensors(void *parameter) {
  // TaskSensors : acquisition des capteurs.
  // Elle lit les capteurs rapides régulièrement, puis envoie les mesures
  // complètes dans une Queue vers TaskMQTT.
  TickType_t lastWakeTime = xTaskGetTickCount();
  unsigned long lastSlowReadMs = 0;

  for (;;) {
    unsigned long perfStart = micros();
    readFastSensors();
    readJoystick();

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    bool eco = ecoMode;
    xSemaphoreGive(dataMutex);

    unsigned long now = millis();
    unsigned long slowInterval = eco ? 5000 : 2000;

    if (lastSlowReadMs == 0 || now - lastSlowReadMs >= slowInterval) {
      lastSlowReadMs = now;

      readDHT22();
      readGasSensor();

      // Communication inter-tâches demandée par le sujet :
      // TaskSensors -> sensorQueue -> TaskMQTT.
      sendLatestSensorPacketToQueue();
    }

    recordTaskRuntime(PERF_SENSORS, perfStart);
    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(eco ? 250 : 80));
  }
}

void taskSupervisor(void *parameter) {
  for (;;) {
    unsigned long perfStart = micros();
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    evaluateSystemUnsafe();
    xSemaphoreGive(dataMutex);

    recordTaskRuntime(PERF_SUPERVISION, perfStart);
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void taskLights(void *parameter) {
  for (;;) {
    unsigned long perfStart = micros();
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    updateLightShowUnsafe();
    xSemaphoreGive(dataMutex);

    recordTaskRuntime(PERF_LIGHTS, perfStart);
    vTaskDelay(pdMS_TO_TICKS(120));
  }
}

void taskAlarmBuzzer(void *parameter) {
  for (;;) {
    unsigned long perfStart = micros();
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    updateAlarmBuzzerStepUnsafe();
    xSemaphoreGive(dataMutex);

    recordTaskRuntime(PERF_ALARM_BUZZER, perfStart);
    vTaskDelay(pdMS_TO_TICKS(15));
  }
}

void taskMusicBuzzer(void *parameter) {
  for (;;) {
    unsigned long perfStart = micros();
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    updateMusicStepUnsafe();
    xSemaphoreGive(dataMutex);

    recordTaskRuntime(PERF_MUSIC_BUZZER, perfStart);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void taskWeb(void *parameter) {
  // TaskWeb : le serveur ESPAsyncWebServer fonctionne de manière asynchrone.
  // Cette tâche garde un heartbeat pour montrer que la partie Web est bien
  // supervisée sans mettre de logique métier dans loop().
  for (;;) {
    unsigned long perfStart = micros();
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    webTaskHeartbeat++;
    xSemaphoreGive(dataMutex);

    recordTaskRuntime(PERF_WEB, perfStart);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void taskMqtt(void *parameter) {
  // TaskMQTT : consomme la Queue envoyée par TaskSensors.
  // Si MQTT/WiFi tombe, les mesures sont gardées dans un buffer offline.
  SensorPacket packet;

  for (;;) {
    unsigned long perfStart = micros();
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    bool enabled = mqttEnabled;
    bool forcedNetworkTest = networkForcedOff;

    if (mqttBlackoutActive && mqttBlackoutUntilMs > 0 && millis() >= mqttBlackoutUntilMs) {
      mqttBlackoutActive = false;
      mqttBlackoutUntilMs = 0;
      addEventUnsafe("MQTT", "INFO", "Simulation panne MQTT terminée");
    }

    bool mqttBlackoutTest = mqttBlackoutActive && mqttBlackoutUntilMs > millis();
    xSemaphoreGive(dataMutex);

    // Simulation panne MQTT : le WiFi reste actif donc le dashboard continue,
    // mais TaskMQTT force le stockage offline. Parfait pour montrer les graphes.
    if (mqttBlackoutTest) {
      if (mqttClient.connected()) mqttClient.disconnect();

      xSemaphoreTake(dataMutex, portMAX_DELAY);
      mqttConnected = false;
      xSemaphoreGive(dataMutex);

      while (sensorQueue != nullptr && xQueueReceive(sensorQueue, &packet, 0) == pdTRUE) {
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        storeOfflinePacketUnsafe(packet);
        mqttFailedCount++;
        xSemaphoreGive(dataMutex);
      }

      recordTaskRuntime(PERF_MQTT, perfStart);
    vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    // Si MQTT est désactivé volontairement ET qu'on n'est pas en test réseau,
    // on ne garde pas les mesures.
    // Si le WiFi est coupé par le bouton de test, on stocke quand même offline.
    if (!enabled && !forcedNetworkTest) {
      if (mqttClient.connected()) mqttClient.disconnect();

      xSemaphoreTake(dataMutex, portMAX_DELAY);
      mqttConnected = false;
      xSemaphoreGive(dataMutex);

      // MQTT désactivé volontairement : on vide la queue sans stocker offline.
      while (sensorQueue != nullptr && xQueueReceive(sensorQueue, &packet, 0) == pdTRUE) {
        // rien
      }

      recordTaskRuntime(PERF_MQTT, perfStart);
    vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    if (!mqttClient.connected()) {
      unsigned long now = millis();

      if (now - lastMqttReconnectAttemptMs > 5000) {
        lastMqttReconnectAttemptMs = now;
        mqttReconnect();
      }

      // Réseau indisponible : on vide la Queue vers le buffer offline.
      while (sensorQueue != nullptr && xQueueReceive(sensorQueue, &packet, 0) == pdTRUE) {
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        storeOfflinePacketUnsafe(packet);
        mqttFailedCount++;
        xSemaphoreGive(dataMutex);
      }

      recordTaskRuntime(PERF_MQTT, perfStart);
    vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    mqttClient.loop();

    // Replay de la base locale LittleFS : priorité aux mesures les plus anciennes
    // gardées pendant une panne réseau longue.
    String fileReplayPayload;
    bool hasFileReplay = false;

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    hasFileReplay = popOfflinePayloadUnsafe(fileReplayPayload);
    xSemaphoreGive(dataMutex);

    if (hasFileReplay) {
      bool ok = publishPayloadToMqtt(fileReplayPayload, true);

      if (!ok) {
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        appendOfflinePayloadUnsafe(fileReplayPayload);
        mqttConnected = false;
        xSemaphoreGive(dataMutex);

        mqttClient.disconnect();
        recordTaskRuntime(PERF_MQTT, perfStart);
    vTaskDelay(pdMS_TO_TICKS(500));
        continue;
      }
    }

    // Replay offline RAM prioritaire si LittleFS est indisponible.
    // On rejoue les anciennes mesures d'abord.
    SensorPacket replayPacket;
    bool hasReplay = false;

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    hasReplay = popOfflinePacketUnsafe(replayPacket);
    xSemaphoreGive(dataMutex);

    if (hasReplay) {
      bool ok = publishPacketToMqtt(replayPacket, true);

      if (!ok) {
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        storeOfflinePacketUnsafe(replayPacket);
        mqttConnected = false;
        xSemaphoreGive(dataMutex);

        mqttClient.disconnect();
        recordTaskRuntime(PERF_MQTT, perfStart);
    vTaskDelay(pdMS_TO_TICKS(500));
        continue;
      }
    }

    // Publication des nouvelles mesures provenant de TaskSensors.
    if (sensorQueue != nullptr && xQueueReceive(sensorQueue, &packet, pdMS_TO_TICKS(50)) == pdTRUE) {
      bool ok = publishPacketToMqtt(packet, false);

      if (!ok) {
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        storeOfflinePacketUnsafe(packet);
        mqttFailedCount++;
        mqttConnected = false;
        xSemaphoreGive(dataMutex);

        mqttClient.disconnect();
      }
    }

    recordTaskRuntime(PERF_MQTT, perfStart);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void taskSystemLog(void *parameter) {
  for (;;) {
    unsigned long perfStart = micros();
    xSemaphoreTake(dataMutex, portMAX_DELAY);

    Serial.println("======== STATION IoT ULTIMATE ========");
    Serial.print("WiFi: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connecté" : "Déconnecté");

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
    }

    Serial.print("Température: ");
    Serial.print(effectiveTempUnsafe());
    Serial.print(" °C | Humidité: ");
    Serial.print(effectiveHumidityUnsafe());
    Serial.println(" %");

    Serial.print("Gaz raw: ");
    Serial.print((int)effectiveGasRawUnsafe());
    Serial.print(" | Gaz %: ");
    Serial.println(effectiveGasPercentUnsafe());

    Serial.print("PIR: ");
    Serial.print(effectivePirUnsafe() ? "MOUVEMENT" : "RAS");
    Serial.print(" | Compteur PIR: ");
    Serial.println(pirMotionCount);

    Serial.print("Sécurité: ");
    Serial.print(securityEnabled ? "ON" : "OFF");
    Serial.print(" | Intrusion: ");
    Serial.println(intrusionLatched ? "OUI" : "NON");

    Serial.print("Risque: ");
    Serial.print(riskScoreUnsafe());
    Serial.print(" | État: ");
    Serial.println(riskStateUnsafe());

    Serial.print("Mode lumière: ");
    Serial.println(lightModeNameUnsafe());

    Serial.print("Musique: ");
    Serial.println(currentSongName);

    Serial.print("MQTT: ");
    Serial.println(mqttConnected ? "Connecté" : "Déconnecté");

    Serial.print("Queue capteurs: ");
    Serial.print(sensorQueue == nullptr ? 0 : uxQueueMessagesWaiting(sensorQueue));
    Serial.print(" | Offline: ");
    Serial.print(offlineCount);
    Serial.print(" | Latence MQTT: ");
    Serial.print(lastPublishLatencyMs);
    Serial.println(" ms");

    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Uptime: %lu secondes\n", millis() / 1000);
    Serial.println("======================================");

    xSemaphoreGive(dataMutex);

    recordTaskRuntime(PERF_SYSTEM_LOG, perfStart);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// =====================================================
// Setup
// =====================================================
void setup() {
  Serial.begin(115200);

  dataMutex = xSemaphoreCreateMutex();
  sensorQueue = xQueueCreate(SENSOR_QUEUE_LENGTH, sizeof(SensorPacket));

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

  allLedsOffUnsafe();

  analogReadResolution(12);
  analogSetPinAttenuation(GAS_AO_PIN, ADC_11db);
  analogSetPinAttenuation(JOY_X_PIN, ADC_11db);
  analogSetPinAttenuation(JOY_Y_PIN, ADC_11db);

  littleFsOk = LittleFS.begin(true);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(1500);

  setupWebServer();

  xSemaphoreTake(dataMutex, portMAX_DELAY);
  addEventUnsafe("SYSTEM", "INFO", "Station IoT supervision + jeux démarrée");

  if (sensorQueue == nullptr) {
    addEventUnsafe("QUEUE", "CRITICAL", "Échec création Queue FreeRTOS");
  } else {
    addEventUnsafe("QUEUE", "INFO", "Queue capteurs -> MQTT créée");
  }

  if (littleFsOk) {
    refreshOfflineDbStatsUnsafe();
    refreshHistoryDbStatsUnsafe();
    addEventUnsafe("DB", "INFO", "Base locale LittleFS prête");
  } else {
    addEventUnsafe("DB", "CRITICAL", "LittleFS indisponible : fallback RAM seulement");
  }

  addEventUnsafe("JOYSTICK", "INFO", "HW-504 : VRX=GPIO32, VRY=GPIO35, SW=GPIO23");
  startSongUnsafe(SONG_STARTUP, sizeof(SONG_STARTUP) / sizeof(SONG_STARTUP[0]), "startup");
  xSemaphoreGive(dataMutex);

  xTaskCreatePinnedToCore(taskWifi, "TaskWiFi", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskSensors, "TaskSensors", 6144, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(taskMqtt, "TaskMQTT", 6144, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(taskWeb, "TaskWeb", 3072, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskSupervisor, "TaskSupervision", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskLights, "TaskLights", 3072, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskAlarmBuzzer, "TaskAlarmBuzzer", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskMusicBuzzer, "TaskMusicBuzzer", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskSystemLog, "TaskSystemLog", 4096, NULL, 1, NULL, 0);

  Serial.println("Setup terminé : station IoT Ultimate lancée.");
}

// =====================================================
// loop vide : aucune logique ici
// =====================================================
void loop() {
}
