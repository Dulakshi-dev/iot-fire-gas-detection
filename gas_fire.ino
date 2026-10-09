#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID   "TMPL6OedOWV-3"
#define BLYNK_TEMPLATE_NAME "FIRE and GAS Alert"
#define BLYNK_AUTH_TOKEN    "kbjC26uJBoHHmVZN44MjE34szvY9zJa7"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "HUAWEI nova Y61";
char pass[] = "06789124";

// Pin definitions
const int MQ2_PIN = 34;
const int FLAME_PIN = 35;
const int BUZZER_PIN = 26;
const int RELAY_PIN = 25;

const int GAS_THRESHOLD = 1000;
const bool RELAY_ACTIVE_LOW = false;

unsigned long readingCount = 0;
BlynkTimer timer;

// Cooldown so it doesn't spam alerts every 200ms while alarm is active
bool lastAlarmState = false;
bool lastGasAlert = false;
bool lastFlameAlert = false;

const char* FIREBASE_URL = "https://firegasdetection-fe143-default-rtdb.firebaseio.com/readings.json";

void logToFirebase(int gas, bool gasAlert, bool flame, bool alarm) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, FIREBASE_URL);
  http.addHeader("Content-Type", "application/json");
  String body = "{\"gas\":" + String(gas) +
                ",\"gasAlert\":" + (gasAlert ? "true" : "false") +
                ",\"flame\":" + (flame ? "true" : "false") +
                ",\"alarm\":" + (alarm ? "true" : "false") +
                ",\"ts\":{\".sv\":\"timestamp\"}}";
  http.POST(body);
  http.end();
}

void sendReadings() {
  readingCount++;
  int gasRaw = analogRead(MQ2_PIN);
  bool gasDetected = gasRaw > GAS_THRESHOLD;
  int flameDigital = digitalRead(FLAME_PIN);
  bool flameDetected = (flameDigital == LOW);
  bool alarm = gasDetected || flameDetected;

  digitalWrite(BUZZER_PIN, alarm ? HIGH : LOW);

  bool pumpOn = flameDetected;
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? (pumpOn ? LOW : HIGH) : (pumpOn ? HIGH : LOW));

  // Push live values to Blynk
  Blynk.virtualWrite(V0, gasRaw);
  Blynk.virtualWrite(V1, gasDetected ? 1 : 0);
  Blynk.virtualWrite(V2, flameDetected ? 1 : 0);
  Blynk.virtualWrite(V3, alarm ? 1 : 0);

  // Fire a one-time event notification on the RISING edge only
  if (gasDetected && !lastGasAlert) {
    Blynk.logEvent("gas_alert", "Gas level exceeded threshold!");
    Serial.println(">>> gas_alert LOGGED");
  }
  if (flameDetected && !lastFlameAlert) {
    Blynk.logEvent("fire_alert", "Flame detected! Pump activated.");
    Serial.println(">>> fire_alert LOGGED");
  }

  if (readingCount % 10 == 0 || gasDetected != lastGasAlert || flameDetected != lastFlameAlert) {
    logToFirebase(gasRaw, gasDetected, flameDetected, alarm);
  }

  lastGasAlert = gasDetected;
  lastFlameAlert = flameDetected;

  Serial.print("Reading #"); Serial.println(readingCount);
  Serial.print("Gas: "); Serial.print(gasRaw);
  Serial.print(" | Gas Detected: "); Serial.print(gasDetected ? "YES" : "NO");
  Serial.print(" | Flame: "); Serial.print(flameDetected ? "YES" : "NO");
  Serial.print(" | Alarm: "); Serial.println(alarm ? "ON" : "OFF");
}

void setup() {
  Serial.begin(115200);
  pinMode(FLAME_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);

  Blynk.begin(auth, ssid, pass);
  timer.setInterval(1000L, sendReadings); // send every 1s instead of 200ms
}

void loop() {
  Blynk.run();
  timer.run();
}
