#include <Servo.h>

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

// Servo constants
const unsigned long THROTTLE_MIN[2] = { 0, 0 };
const unsigned long THROTTLE_MAX[2] = { 180, 180 };
#define SERVO_MIN THROTTLE_MIN
#define SERVO_MAX THROTTLE_MAX
const unsigned int GAS_PIN[2] = { 9, 10 };

// Servos
Servo gas_pedal[2];

void setup() {
  // put your setup code here, to run once:
  debug({ Serial.begin(9600); });

  initializeGamePad();
  initializeThrottle();
}

void initializeGamePad() {
  // Initialize the 0-point for the joystick, this means don't press it at start up, dummy
  for (int side = LEFT; side <= RIGHT; side++) {
    JOYSTICK_ZERO[side] = getJoystickPosition(side);
    long speed = JOYSTICK_ZERO[side] - JOYSTICK_MAX;
    if (speed < MAX_SPEED) {
      MAX_SPEED = speed;
    }
    debug({ Serial.print("JOYSTICK_ZERO["); Serial.print(side); Serial.print("]: "); Serial.println(JOYSTICK_ZERO[side]); });
  }
  debug({ Serial.print("MAX_SPEED: "); Serial.println(MAX_SPEED); });
}

void initializeThrottle() {
  // Start the servo shield
  for (int i = LEFT; i <= RIGHT; i++) {
    gas_pedal[i].attach(GAS_PIN[i]);
  }
  setThrottle(LEFT, 0);
  setThrottle(RIGHT, 0);
}

long getJoystickPosition(int side) {
  long joystick_position = analogRead(GAMEPAD_PIN[side]);
  return joystick_position;
}

void loop() {
  for (int side = LEFT; side <= RIGHT; side++) {
    long desired_speed = getDesiredSpeed(side);
    setThrottle(side, desired_speed);
  }
  delay(30);
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

// Set the throttle
void setThrottle(int side, long speed) {
  debug({ Serial.print(side); Serial.print(": setting throttle to "); Serial.println(speed); });
  
  // Map degrees to pulse length
  long position = constrain(
     map(speed, 0, MAX_SPEED, THROTTLE_MIN[side], THROTTLE_MAX[side]),
     SERVO_MIN[side],
     SERVO_MAX[side]
  );
  debug({ Serial.print(side); Serial.print(": setting pulse length to "); Serial.println(position); });

  // Set the throttle
  gas_pedal[side].write(position);
}
