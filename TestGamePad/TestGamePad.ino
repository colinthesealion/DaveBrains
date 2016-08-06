// For if you want to debug
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

// Gamepad constants
const unsigned int GAMEPAD_PIN[2] = { 0, 1 }; // analog pins for the gamepad
static unsigned long JOYSTICK_ZERO[2];
#define JOYSTICK_MAX 0
#define JOYSTICK_THRESHOLD 5
static unsigned long MAX_SPEED = 436;

void setup() {
  // put your setup code here, to run once:
  debug({ Serial.begin(9600); });

  initializeGamePad();
}

void initializeGamePad() {
  // Initialize the 0-point for the joystick, this means don't press it at start up, dummy
  for (int side = LEFT; side <= RIGHT; side++) {
    JOYSTICK_ZERO[side] = getJoystickPosition(side);
    long speed = JOYSTICK_MAX - JOYSTICK_ZERO[side];
    if (speed < MAX_SPEED) {
      MAX_SPEED = speed;
    }
    debug({ Serial.print("JOYSTICK_ZERO["); Serial.print(side); Serial.print("]: "); Serial.println(JOYSTICK_ZERO[side]); });
  }
  debug({ Serial.print("MAX_SPEED: "); Serial.println(MAX_SPEED); });
}

long getJoystickPosition(int side) {
  long joystick_position = analogRead(GAMEPAD_PIN[side]);
  return joystick_position;
}

void loop() {
  for (int side = LEFT; side <= RIGHT; side++) {
    long pos = getJoystickPosition(side);
    debug({ Serial.print("Joystick Position["); Serial.print(side); Serial.print("]: "); Serial.println(pos); });
    long desired_speed = getDesiredSpeed(side);
    debug({ Serial.print("Desired Speed["); Serial.print(side); Serial.print("]: "); Serial.println(desired_speed); });
  }
  delay(1000);
}

// Get the desired speed
long getDesiredSpeed(int side) {
  long stick_value = getJoystickPosition(side);
  if (stick_value >= (JOYSTICK_ZERO[side] + 2 * JOYSTICK_THRESHOLD)) {
    // Brake
    return -1;
  }
  else if (stick_value <= (JOYSTICK_ZERO[side] - JOYSTICK_THRESHOLD)) {
    return map(stick_value, JOYSTICK_ZERO[side], JOYSTICK_MAX, 0, MAX_SPEED);
  }
  else {
    return 0;
  }
}
