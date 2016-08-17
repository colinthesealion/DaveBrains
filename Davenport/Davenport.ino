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
const float MAX_SPEED = 2.2352; // m/s = 5 mph
const long GAMEPAD_NORMAL_MAX = 1000;
const unsigned int SAME_SAME_THRESHOLD = GAMEPAD_NORMAL_MAX / 10;

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
const unsigned long GAS_ENGAGE = 6 * (GAMEPAD_NORMAL_MAX / 10);

// Wheel speed sensor constants
const int WHEEL_SPEED_PIN[2] = { 2, 3 };
const float WHEEL_DIAMETER = 0.1524; // meters
const float WHEEL_CIRCUMFERENCE = PI * WHEEL_DIAMETER;
const int CLICKS_PER_REVOLUTION = 40;
const float METERS_PER_CLICK = WHEEL_CIRCUMFERENCE / CLICKS_PER_REVOLUTION;
const float ALPHA = 0.1;

// PID control
struct PID {
  float p;
  float i;
  float d;
};
const PID K = { 0.5, 0.1, 0.01 };

// Servos
Servo gas_pedal[2];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  initializeGamePad();
  initializeWheelSpeedSensors();
  initializeThrottle();
}

void initializeGamePad() {
  // Initialize the 0-point for the joystick, this means don't press it at start up, dummy
  for (int side = LEFT; side <= RIGHT; side++) {
    JOYSTICK_ZERO[side] = getGamepadPosition(side);
    long speed = JOYSTICK_ZERO[side] - JOYSTICK_MAX;
  }
}

void initializeWheelSpeedSensors() {
  attachInterrupt(digitalPinToInterrupt(WHEEL_SPEED_PIN[LEFT]), clickLeft, CHANGE);
  attachInterrupt(digitalPinToInterrupt(WHEEL_SPEED_PIN[RIGHT]), clickRight, CHANGE);
}

void initializeThrottle() {
  // Start the servo shield
  for (int i = LEFT; i <= RIGHT; i++) {
    gas_pedal[i].attach(GAS_PIN[i]);
    setThrottlePID(i, 0);
  }
}

void logSerial(String log_line) {
  Serial.print(millis());  
  Serial.print(" ");
  Serial.print(log_line);
}

long getGamepadPosition(int side) {
  long gamepad_position = analogRead(GAMEPAD_PIN[side]);
  String log_line = "";
  log_line.concat("GP ");
  log_line.concat((side == LEFT) ? "L " : "R ");
  log_line.concat(gamepad_position);
  logSerial(log_line);
  return gamepad_position;
}

void loop() {
  maybeLogConstants();
  float throttle[2];
  getThrottlePID(throttle);
  for (int side = LEFT; side <= RIGHT; side++) {
    setThrottlePID(side, throttle[side]);
  }

  // Wait for the next throttle update
  delay(1000 / REFRESH_RATE);
}

void maybeLogConstants() {
  if (random(100) < 10) {
    String log_line = "";
    log_line.concat("C ");
    log_line.concat(" "); log_line.concat(SAME_SAME_THRESHOLD);
    log_line.concat(" "); log_line.concat(ALPHA);
    log_line.concat(" "); log_line.concat(K.p);
    log_line.concat(" "); log_line.concat(K.i);
    log_line.concat(" "); log_line.concat(K.d);
    logSerial(log_line);
  }
}

// Set the throttle according to the PID signal
// The PID signal should be in range 0-MAX_SPEED
// And we want to map that to servo positions
void setThrottlePID (int side, float throttle) {
  if (throttle < 0) {
    throttle = 0;
  }
  long position = constrain(
    map(1000 * throttle, 0, 1000 * MAX_SPEED, THROTTLE_MIN[side], THROTTLE_MAX[side]),
    THROTTLE_MIN[side],
    THROTTLE_MAX[side]
  );
  gas_pedal[side].write(position);
  String log_line = "";
  log_line.concat("ST ");
  log_line.concat((side == LEFT) ? "L " : "R ");
  log_line.concat(position);
  logSerial(log_line);
}

// Mapping the individual controllers with their own resting state values to
// a normalized value between 0 and 1000. Notice that the joysticks are physically
// upside down so we are mapping the max to zero and the min to GAMEPAD_NORMAL_MAX
// so that GAMEPAD_NORMAL_MAX is always full throttle and 0 is brake.
long getGamepadNormal (int side) {
  long gamepad_position = getGamepadPosition(side);
  long gamepad_position_normal = map(gamepad_position, 0, 2 * JOYSTICK_ZERO[side], GAMEPAD_NORMAL_MAX, 0);
  String log_line = "";
  log_line.concat("GPN ");
  log_line.concat((side == LEFT) ? "L " : "R ");
  log_line.concat(gamepad_position_normal);
  logSerial(log_line);
}

// Use PID control to determine the throttle signal
void getThrottlePID (float *throttle) {
  static float previous_error[2] = { 0, 0 };
  static float integral[2] = { 0, 0 };

  // Determine the game pad positions
  long gamepad_position[2];
  for (int side = LEFT; side <= RIGHT; side++) {
    gamepad_position[side] = getGamepadNormal(side);
    if (gamepad_position[side] <= GAS_ENGAGE) {
      gamepad_position[side] = 0;
    }
  }

  // If the game pad positions are close, make them the same
  long diff = gamepad_position[LEFT] - gamepad_position[RIGHT];
  if (abs(diff) <= SAME_SAME_THRESHOLD) {
    if (diff > 0) {
      gamepad_position[RIGHT] = gamepad_position[LEFT];
    }
    else {
      gamepad_position[LEFT] = gamepad_position[RIGHT];
    }
  }

  for (int side = LEFT; side <= RIGHT; side++) {
    // The set point is our desired speed
    float set_point = MAX_SPEED * (gamepad_position[side] - GAS_ENGAGE) / (float)(GAMEPAD_NORMAL_MAX - GAS_ENGAGE);

    // The error is our desired speed minus our actual speed
    float error = set_point - getActualSpeed(side);
    String log_line = "";
    log_line.concat("E ");
    log_line.concat((side == LEFT) ? "L " : "R ");
    log_line.concat(error);
    logSerial(log_line);

    // The integral is error * dt = error / refresh_rate
    integral[side] += error / (float)REFRESH_RATE;

    // The derivative is (error - previous_error) / dt = (error - previous_error) * refresh_rate
    float derivative = (error - previous_error[side]) * (float)REFRESH_RATE;

    // And PID is pretty straightforward
    throttle[side] = K.p * error + K.i * integral[side] + K.d * derivative;

    log_line = "";
    log_line.concat("GT ");
    log_line.concat((side == LEFT) ? "L " : "R ");
    log_line.concat(throttle[side]);
    logSerial(log_line);

    // Save the current error for next iteration
    previous_error[side] = error;
  }

  return;
}

// Keep track of the number of clicks on the wheel
volatile unsigned long n_clicks[2] = { 0, 0 };
void clickLeft() {
  n_clicks[LEFT]++;
}
void clickRight() {
  n_clicks[RIGHT]++;
}

// Compute the current speed using an exponentially decaying weighted average
float getActualSpeed (int side) {
  static float floating_average[2] = { 0, 0 };
  static boolean initialized[2] = { false, false };
  unsigned long previous_n_clicks = n_clicks[side];
  n_clicks[side] = 0;
  float current_speed = previous_n_clicks * METERS_PER_CLICK * REFRESH_RATE;
  if (initialized[side]) {
    floating_average[side] = ALPHA * current_speed + (1 - ALPHA) * floating_average[side];
  }
  else {
    floating_average[side] = current_speed;
    initialized[side] = true;
  }

  String log_line = "";
  log_line.concat("V ");
  log_line.concat((side == LEFT) ? "L " : "R ");
  log_line.concat(previous_n_clicks);
  log_line.concat(" ");
  log_line.concat(current_speed);
  log_line.concat(" ");
  log_line.concat(floating_average[side]);
  logSerial(log_line);
}

