#define DEBUG
#ifdef DEBUG
// Macro debug(block) exectues block
#define debug(block) block
#else
// Macro debug(block) is a no-op
#define debug(block) (void)0;
#endif

#define LEFT  0
#define RIGHT 1

// Brakes constants
const int BRAKE_HOLD_PIN[2] = { 4, 5 }; // RED
const int BRAKE_PULL_PIN[2] = { 6, 7 }; // GREEN
#define BRAKE_PULL_DURATION 50 // ms

void setup() {
  debug({ Serial.begin(9600); });
  initializeBrakes();
}

void initializeBrakes() {
  // Setup brake pins
  for (int side = LEFT; side <= RIGHT; side++) {
    pinMode(BRAKE_PULL_PIN[side],  OUTPUT);
    digitalWrite(BRAKE_PULL_PIN[side], HIGH);
    pinMode(BRAKE_HOLD_PIN[side], OUTPUT);
    digitalWrite(BRAKE_HOLD_PIN[side], HIGH);
  }  
}

void loop() {
  setBrakes(LEFT, true);
  delay(2000);
  setBrakes(LEFT, false);
  delay(2000);
  setBrakes(RIGHT, true);
  delay(2000);
  setBrakes(RIGHT, false);
  delay(2000);
}

// Engage the brakes
void setBrakes(int side, bool on) {
  // The relay board takes LOW to mean ON, HIGH to mean OFF
  debug({ Serial.print("setBrakes["); Serial.print(side); Serial.print("]: "); Serial.println(on ? "On" : "Off"); });
  digitalWrite(BRAKE_HOLD_PIN[side], on ? LOW : HIGH);
  static bool brakes_engaged[2] = { false, false };
  if (on && !brakes_engaged[side]) {
    digitalWrite(BRAKE_PULL_PIN[side], LOW);
    delay(BRAKE_PULL_DURATION);
    digitalWrite(BRAKE_PULL_PIN[side], HIGH);
  }
  brakes_engaged[side] = on;
}
