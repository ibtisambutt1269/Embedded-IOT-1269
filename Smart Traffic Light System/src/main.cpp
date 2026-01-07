
/*************************************************
 * Smart Traffic Management System with Toll Plaza
 * ESP32 + 3 Traffic Lanes + 1 Toll IR + Servo + Blynk
 *************************************************/

// ===== BLYNK TEMPLATE DEFINES MUST BE FIRST =====
#define BLYNK_TEMPLATE_ID   "TMPL61eb68bj0"
#define BLYNK_TEMPLATE_NAME "Smart Traffic System"
#define BLYNK_AUTH_TOKEN    "3PbsbVZFchkO0GEG5by_rSO5lW1TAT5k"

// ===== LIBRARIES =====
#include <WiFi.h>
#include <ESP32Servo.h>
#include <BlynkSimpleEsp32.h>

// ===== WIFI =====
char ssid[] = "Saim Butt";
char pass[] = "123456780";

// ================= CONFIG =================
#define NUM_LANES 3

// ===== LED PINS =====
const uint8_t greenLedPins[NUM_LANES]  = {16, 17, 18};
const uint8_t yellowLedPins[NUM_LANES] = {21, 22, 32};
const uint8_t redLedPins[NUM_LANES]    = {25, 19, 23};

// ===== IR SENSORS =====
const uint8_t trafficIrPins[NUM_LANES] = {13, 26, 14};
#define TOLL_IR_PIN 33

// ===== SERVO =====
#define SERVO_PIN 4
Servo tollServo;

// ===== TIMING =====
const unsigned long GREEN_TIME  = 5000;
const unsigned long YELLOW_TIME = 2000;
const unsigned long DEBOUNCE_MS = 80;

// ===== STATE =====
enum State { RED, YELLOW, GREEN };
State laneState[NUM_LANES];
uint8_t currentLane = 0;
unsigned long lastChange = 0;

// IR counting
int vehicleCount[NUM_LANES] = {0};
bool irTriggered[NUM_LANES] = {false};
unsigned long lastIrTime[NUM_LANES] = {0};

// ===== BLYNK CLOUD VARIABLES =====
bool autoMode = true;          // V0
bool tollManual = false;       // V1
bool preemptionEnable = true;  // V2
unsigned long cloudGreenTime = 5000; // V3

// ===== BLYNK INPUTS =====
BLYNK_WRITE(V0) { autoMode = param.asInt(); }
BLYNK_WRITE(V1) { tollManual = param.asInt(); }
BLYNK_WRITE(V2) { preemptionEnable = param.asInt(); }
BLYNK_WRITE(V3) { cloudGreenTime = param.asInt() * 1000; }


// ================= FUNCTIONS =================
void setLane(uint8_t lane, State s) {
  laneState[lane] = s;
  digitalWrite(greenLedPins[lane],  s == GREEN);
  digitalWrite(yellowLedPins[lane], s == YELLOW);
  digitalWrite(redLedPins[lane],    s == RED);
}

void allRed() {
  for (int i = 0; i < NUM_LANES; i++)
    setLane(i, RED);
}
// ===== MANUAL LANE CONTROL =====
BLYNK_WRITE(V10) { // Lane 1 button
  if (!autoMode) {
    allRed();
    setLane(0, GREEN);
    currentLane = 0;
    lastChange = millis();
  }
}

BLYNK_WRITE(V11) { // Lane 2 button
  if (!autoMode) {
    allRed();
    setLane(1, GREEN);
    currentLane = 1;
    lastChange = millis();
  }
}

BLYNK_WRITE(V12) { // Lane 3 button
  if (!autoMode) {
    allRed();
    setLane(2, GREEN);
    currentLane = 2;
    lastChange = millis();
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_LANES; i++) {
    pinMode(greenLedPins[i], OUTPUT);
    pinMode(yellowLedPins[i], OUTPUT);
    pinMode(redLedPins[i], OUTPUT);
    pinMode(trafficIrPins[i], INPUT_PULLUP);
  }

  pinMode(TOLL_IR_PIN, INPUT_PULLUP);

  tollServo.attach(SERVO_PIN);
  tollServo.write(0); // Gate closed

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  allRed();
  setLane(0, GREEN);
  lastChange = millis();

  Serial.println("Smart Traffic System Started");
}

// ================= LOOP =================
void loop() {
  Blynk.run();
  unsigned long now = millis();

  // ---------- TRAFFIC IR COUNT ----------
  for (int i = 0; i < NUM_LANES; i++) {
    if (laneState[i] != RED) continue;

    if (digitalRead(trafficIrPins[i]) == LOW &&
        !irTriggered[i] &&
        now - lastIrTime[i] > DEBOUNCE_MS) {

      irTriggered[i] = true;
      vehicleCount[i]++;
      lastIrTime[i] = now;
      Serial.printf("Lane %d count = %d\n", i + 1, vehicleCount[i]);
    }

    if (digitalRead(trafficIrPins[i]) == HIGH)
      irTriggered[i] = false;

    // PRIORITY PREEMPTION
    if (preemptionEnable && autoMode && vehicleCount[i] >= 2) {
      allRed();
      setLane(i, GREEN);
      currentLane = i;
      vehicleCount[i] = 0;
      lastChange = now;
      return;
    }
  }

  // ---------- NORMAL CYCLE ----------
  if (autoMode) {
    unsigned long activeGreenTime = cloudGreenTime;

    if (laneState[currentLane] == GREEN &&
        now - lastChange >= activeGreenTime) {
      setLane(currentLane, YELLOW);
      lastChange = now;
    }
    else if (laneState[currentLane] == YELLOW &&
             now - lastChange >= YELLOW_TIME) {
      setLane(currentLane, RED);
      currentLane = (currentLane + 1) % NUM_LANES;
      setLane(currentLane, GREEN);
      vehicleCount[currentLane] = 0;
      lastChange = now;
    }
  }

  // ===== TOLL PLAZA (5 sec auto open) =====
static bool servoOpen = false;
static unsigned long servoOpenedAt = 0;

if (tollManual) {
  tollServo.write(90);          // Manual open
  servoOpenedAt = millis();     // Reset timer
  servoOpen = true;
}
else if (digitalRead(TOLL_IR_PIN) == LOW && !servoOpen) {
  tollServo.write(90);          // Auto open
  servoOpenedAt = millis();     // Start 5 sec timer
  servoOpen = true;
}
else if (servoOpen && millis() - servoOpenedAt >= 5000) {  // 5 sec open
  tollServo.write(0);           // Close gate
  servoOpen = false;
}

  // ---------- SEND DATA TO BLYNK ----------
  Blynk.virtualWrite(V4, vehicleCount[0]);
  Blynk.virtualWrite(V5, vehicleCount[1]);
  Blynk.virtualWrite(V6, vehicleCount[2]);
  Blynk.virtualWrite(V7, autoMode ? "AUTO" : "MANUAL");
  Blynk.virtualWrite(V8, preemptionEnable ? "ENABLED" : "DISABLED");
  Blynk.virtualWrite(V9, currentLane + 1);

  delay(10);
}
