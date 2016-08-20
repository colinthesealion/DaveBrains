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

#define REFRESH_RATE  30 // How many times per second should we update the throttle/brakes

// Gamepad constants
const unsigned int GAMEPAD_PIN[2] = { 0, 1 }; // analog pins for the gamepad
static unsigned long JOYSTICK_ZERO[2];
#define JOYSTICK_MAX 0
#define JOYSTICK_THRESHOLD 5
static unsigned long MAX_SPEED = 1000;
const long GAMEPAD_NORMAL_MAX = 1000;

// Servo constants. When measured, the left throttle was able to move a lot less than the
// right. We are still debugging this. For now, we are setting the range or motion to be
// same number of degrees. In reality the min/max for left was 78/88 and for right 70/90.
// So we will use 78/88 for left and a range or 10 degrees starting at 90 for for right.
// We use 90 because that's rest state whereas 70 tugs to full throttle.
const unsigned long THROTTLE_MIN[2] = { 30, 20 };
const unsigned long THROTTLE_MAX[2] = { 40, 30 };
#define SERVO_MIN THROTTLE_MIN
#define SERVO_MAX THROTTLE_MAX
const unsigned int GAS_PIN[2] = { 9, 10 };

// Brakes constants
const int BRAKE_HOLD_PIN[2] = { 5, 4 }; // PURPLE
const int BRAKE_PULL_PIN[2] = { 7, 6 }; // PURPLE
#define BRAKE_PULL_DURATION 75 // ms

// Servos
Servo gas_pedal[2];

void setup() {
  // put your setup code here, to run once:
  debug({ Serial.begin(9600); });

  initializeGamePad();
  initializeThrottle();
  initializeBrakes();
}

void initializeGamePad() {
  // Initialize the 0-point for the joystick, this means don't press it at start up, dummy
  for (int side = LEFT; side <= RIGHT; side++) {
    JOYSTICK_ZERO[side] = getGamepadPosition(side);
    long speed = JOYSTICK_ZERO[side] - JOYSTICK_MAX;
    if (speed < MAX_SPEED) {
      MAX_SPEED = speed;
    }
    //debug({ Serial.print("JOYSTICK_ZERO["); Serial.print(side); Serial.print("]: "); Serial.println(JOYSTICK_ZERO[side]); });
  }
  //debug({ Serial.print("MAX_SPEED: "); Serial.println(MAX_SPEED); });
}

void initializeThrottle() {
  // Start the servo shield
  for (int i = LEFT; i <= RIGHT; i++) {
    gas_pedal[i].attach(GAS_PIN[i]);
    setThrottle(i, 0);
  }
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

long getGamepadPosition(int side) {
  long gamepad_position = analogRead(GAMEPAD_PIN[side]);
  return gamepad_position;
}

void loop() {
  for (int side = LEFT; side <= RIGHT; side++) {
    long gamepad_position = getGamepadNormal(side);
    debug({Serial.print("side=");Serial.print(side);Serial.print("pos=");Serial.print(gamepad_position);Serial.print("\n");});
    //long desired_speed = getDesiredSpeed(side);
    setThrottle(side, gamepad_position);
    setBrakes(side, gamepad_position);
  }

  // Wait for the next throttle update
  delay(1000 / REFRESH_RATE);
}

// Set the throttle
void setThrottle(int side, long gamepad_position) {
  //debug({ Serial.print(side); Serial.print(": setting throttle to "); Serial.println(speed); });

  // Map degrees to pulse length
  long gas_engage = 6 * (GAMEPAD_NORMAL_MAX / 10);
  Serial.print (gas_engage);
  long position = constrain(
                    map(gamepad_position,
                        gas_engage, GAMEPAD_NORMAL_MAX,
                        THROTTLE_MIN[side], THROTTLE_MAX[side]),
                    0,//SERVO_MIN[side],
                    180//SERVO_MAX[side]
                  );
  Serial.print(",");
  Serial.print(position);
  Serial.print("\n");
  //debug({ Serial.print(side); Serial.print(": setting pulse length to "); Serial.println(position); });

  // Set the throttle
  gas_pedal[side].write(position);
}

// Engage the brakes
void setBrakes(int side, long gamepad_position) {
  // The relay board takes LOW to mean ON, HIGH to mean OFF
  //debug({ Serial.print("setBrakes["); Serial.print(side); Serial.print("]: "); Serial.println(on ? "On" : "Off"); });
  bool on = gamepad_position < 3 * (GAMEPAD_NORMAL_MAX / 10);
  digitalWrite(BRAKE_HOLD_PIN[side], on ? LOW : HIGH);
  static bool brakes_engaged[2] = { false, false };
  if (on && !brakes_engaged[side]) {
    digitalWrite(BRAKE_PULL_PIN[side], LOW);
    delay(BRAKE_PULL_DURATION);
    digitalWrite(BRAKE_PULL_PIN[side], HIGH);
  }
  brakes_engaged[side] = on;
}

// Mapping the individual controllers with their own resting state values to
// a normalized value between 0 and 1000. Notice that the joysticks are physically
// upside down so we are mapping the max to zero and the min to GAMEPAD_NORMAL_MAX
// so that GAMEPAD_NORMAL_MAX is always full throttle and 0 is brake.
long getGamepadNormal (int side) {
  long gamepad_position = getGamepadPosition(side);
  return map(gamepad_position, 0, 2 * JOYSTICK_ZERO[side], GAMEPAD_NORMAL_MAX, 0);
}

