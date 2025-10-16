#include <Arduino.h>

#define BUTTON_PIN      13
#define BUTTON_TO_GND   false

#define CAR_RED     23
#define CAR_YELLOW  22
#define CAR_GREEN   21
#define PED_RED     17
#define PED_GREEN   16

const unsigned long CAR_GREEN_MS   = 12000;
const unsigned long CAR_YELLOW_MS  = 2000;
const unsigned long CAR_RED_MIN_MS = 2000;
const unsigned long PED_WALK_MS    = 10000;
const unsigned long RED_TO_GREEN_YELLOW_MS = 2000;

const unsigned long DEBOUNCE_MS = 50;
int lastStableBtn = HIGH;
int lastReadBtn   = HIGH;
unsigned long lastChangeMs = 0;

enum Phase {
  CAR_GREEN_PHASE,
  CAR_YELLOW_TO_RED_PHASE,
  CAR_RED_PHASE,
  CAR_RED_TO_GREEN_YELLOW_PHASE
};

Phase phase = CAR_GREEN_PHASE;
unsigned long phaseStartMs = 0;

bool pedRequest = false;
bool pedActive  = false;
unsigned long pedStartMs   = 0;

void setCarLights(bool r, bool y, bool g) {
  digitalWrite(CAR_RED,    r);
  digitalWrite(CAR_YELLOW, y);
  digitalWrite(CAR_GREEN,  g);
}

void setPedLights(bool r, bool g) {
  digitalWrite(PED_RED,   r);
  digitalWrite(PED_GREEN, g);
}

void gotoPhase(Phase p) {
  phase = p;
  phaseStartMs = millis();

  switch (p) {
    case CAR_GREEN_PHASE:
      setCarLights(false, false, true);
      setPedLights(true, false);
      break;
    case CAR_YELLOW_TO_RED_PHASE:
      setCarLights(false, true, false);
      setPedLights(true, false);
      break;
    case CAR_RED_PHASE:
      setCarLights(true, false, false);
      if (pedRequest && !pedActive) {
        pedActive  = true;
        pedStartMs = millis();
        setPedLights(false, true);
      } else {
        setPedLights(true, false);
      }
      break;
    case CAR_RED_TO_GREEN_YELLOW_PHASE:
      setCarLights(false, true, false);
      setPedLights(true, false);
      break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(CAR_RED, OUTPUT);
  pinMode(CAR_YELLOW, OUTPUT);
  pinMode(CAR_GREEN, OUTPUT);
  pinMode(PED_RED, OUTPUT);
  pinMode(PED_GREEN, OUTPUT);

  if (BUTTON_TO_GND) {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    lastStableBtn = lastReadBtn = HIGH;
  } else {
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);
    lastStableBtn = lastReadBtn = LOW;
  }

  setCarLights(false, false, true);
  setPedLights(true, false);
  phaseStartMs = millis();
}

void loop() {
  int raw = digitalRead(BUTTON_PIN);
  if (raw != lastReadBtn) {
    lastReadBtn = raw;
    lastChangeMs = millis();
  }
  if (millis() - lastChangeMs > DEBOUNCE_MS) {
    if (lastStableBtn != lastReadBtn) {
      lastStableBtn = lastReadBtn;
      bool pressed = BUTTON_TO_GND ? (lastStableBtn == LOW) : (lastStableBtn == HIGH);
      if (pressed) {
        pedRequest = true;
        Serial.println("[BTN] Pedestrian requested");
      }
    }
  }

  unsigned long now = millis();

  switch (phase) {
    case CAR_GREEN_PHASE:
      if (now - phaseStartMs >= CAR_GREEN_MS) {
        gotoPhase(CAR_YELLOW_TO_RED_PHASE);
      }
      break;

    case CAR_YELLOW_TO_RED_PHASE:
      if (now - phaseStartMs >= CAR_YELLOW_MS) {
        gotoPhase(CAR_RED_PHASE);
      }
      break;

    case CAR_RED_PHASE:
      if (pedActive) {
        if (now - pedStartMs >= PED_WALK_MS) {
          pedActive  = false;
          pedRequest = false;
          setPedLights(true, false);
          gotoPhase(CAR_RED_TO_GREEN_YELLOW_PHASE);
        }
      } else {
        if (now - phaseStartMs >= CAR_RED_MIN_MS) {
          gotoPhase(CAR_RED_TO_GREEN_YELLOW_PHASE);
        }
      }
      break;

    case CAR_RED_TO_GREEN_YELLOW_PHASE:
      if (now - phaseStartMs >= RED_TO_GREEN_YELLOW_MS) {
        gotoPhase(CAR_GREEN_PHASE);
      }
      break;
  }
}
