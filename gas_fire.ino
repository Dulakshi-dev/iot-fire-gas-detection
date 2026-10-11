#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID   "TMPL6OedOWV-3"
#define BLYNK_TEMPLATE_NAME "FIRE and GAS Alert"
#define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_TOKEN"
 
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
 
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "YOUR_WIFI_NAME";
char pass[] = "YOUR_WIFI_PASSWORD";
 
// Pin definitions
const int MQ2_PIN = 34;
const int FLAME_PIN = 35;
const int BUZZER_PIN = 26;
const int RELAY_PIN = 25;
const bool RELAY_ACTIVE_LOW = false;
 
// Gas detection settings
const int WARMUP_SECONDS = 120;
const int BASELINE_SAMPLES = 20;
const int GAS_MARGIN = 300;
const int GAS_HYST = 100;
 
const char* FIREBASE_URL = "https://YOUR-PROJECT-default-rtdb.firebaseio.com/readings.json";
 
int gasBaseline = 0;
long baselineSum = 0;
int baselineCount = 0;
bool gasReady = false;
bool gasState = false;
 
unsigned long readingCount = 0;
BlynkTimer timer;
 
bool lastGasAlert = false;
bool lastFlameAlert = false;
 
int readGasAvg() {
  long sum = 0;
  for (int i = 0; i < 16; i++) { sum += analogRead(MQ2_PIN); delay(5); }
  return sum / 16;
}
 
void logToFirebase(int gas, bool gasAlert, bool flame, bool alarm) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(3000);
  http.begin(client, FIREBASE_URL);
  http.addHeader("Content-Type", "application/json");
  String body = "{\"gas\":" + String(gas) +
                ",\"gasAlert\":" + (gasAlert ? "true" : "false") +
                ",\"flame\":" + (flame ? "true" : "false") +
                ",\"alarm\":" + (alarm ? "true" : "false") +
                ",\"ts\":{\".sv\":\"timestamp\"}}";
  int code = http.POST(body);
  Serial.print(">>> Firebase response: "); Serial.println(code);
  http.end();
}
 
void sendReadings() {
  readingCount++;
  int gasRaw = readGasAvg();
  bool gasDetected = false;
 
  if (!gasReady) {
    // Warm-up: learn the clean-air baseline, no gas alarm yet
    if (readingCount > (unsigned long)(WARMUP_SECONDS - BASELINE_SAMPLES)) {
      baselineSum += gasRaw;
      baselineCount++;
    }
    if (readingCount >= (unsigned long)WARMUP_SECONDS) {
      gasBaseline = baselineSum / baselineCount;
      gasReady = true;
      Serial.print(">>> Gas baseline: "); Serial.println(gasBaseline);
    }
  } else {
    if (!gasState && gasRaw > gasBaseline + GAS_MARGIN) gasState = true;
    else if (gasState && gasRaw < gasBaseline + GAS_MARGIN - GAS_HYST) gasState = false;
    gasDetected = gasState;
  }
 
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
 
  // One-time notification on the rising edge only
  if (gasDetected && !lastGasAlert) {
    Blynk.logEvent("gas_alert", "Gas level exceeded threshold!");
    Serial.println(">>> gas_alert LOGGED");
  }
  if (flameDetected && !lastFlameAlert) {
    Blynk.logEvent("fire_alert", "Flame detected! Pump activated.");
    Serial.println(">>> fire_alert LOGGED");
  }
 
  // Log to Firebase every 10 s or when the state changes
  if (readingCount % 10 == 0 || gasDetected != lastGasAlert || flameDetected != lastFlameAlert) {
    logToFirebase(gasRaw, gasDetected, flameDetected, alarm);
  }
  lastGasAlert = gasDetected;
  lastFlameAlert = flameDetected;
 
  Serial.print("Reading #"); Serial.println(readingCount);
  Serial.print("Gas: "); Serial.print(gasRaw);
  Serial.print(gasReady ? "" : " (warming up)");
  Serial.print(" | Gas Detected: "); Serial.print(gasDetected ? "YES" : "NO");
  Serial.print(" | Flame: "); Serial.print(flameDetected ? "YES" : "NO");
  Serial.print(" | Alarm: "); Serial.println(alarm ? "ON" : "OFF");
}
 
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Booting...");
 
  pinMode(FLAME_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
 
  Serial.print("Connecting to Wi-Fi: "); Serial.println(ssid);
  WiFi.begin(ssid, pass);
  for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) Serial.println("\nWi-Fi connected");
  else Serial.println("\nWi-Fi FAILED - check name, password, and 2.4 GHz");
 
  Blynk.begin(auth, ssid, pass);
  timer.setInterval(1000L, sendReadings);
}
 
void loop() {
  Blynk.run();
  timer.run();
}
