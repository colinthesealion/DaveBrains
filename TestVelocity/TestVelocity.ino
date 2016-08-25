#include <Servo.h>

#define LEFT  0
#define RIGHT 1

#define REFRESH_RATE  30 // How many times per second should we update the throttle/brakes

// Gamepad constants
const unsigned int GAMEPAD_PIN[2] = { 0, 1 }; // analog pins for the gamepad
static unsigned long JOYSTICK_ZERO[2];
#define JOYSTICK_MAX 0
#define JOYSTICK_THRESHOLD 5
const long GAMEPAD_NORMAL_MAX = 1000;
const unsigned int SAME_SAME_THRESHOLD = GAMEPAD_NORMAL_MAX / 10;

// Servo constants. When measured, the left throttle was able to move a lot less than the
// right. We are still debugging this. For now, we are setting the range or motion to be
// same number of degrees. In reality the min/max for left was 78/88 and for right 70/90.
// So we will use 78/88 for left and a range or 10 degrees starting at 90 for for right.
// We use 90 because that's rest state whereas 70 tugs to full throttle.
const unsigned int THROTTLE_MIN[2] = { 20, 5 };
const unsigned int THROTTLE_MAX[2] = { 50, 30 };
const unsigned int GAS_PIN[2] = { 9, 10 };
const unsigned int GAS_ENGAGE = 6 * (GAMEPAD_NORMAL_MAX / 10);
Servo gas_pedal[2];

const float ALPHA = 0.1; // For smoothing the velocity signal

// Wheel speed sensor constants
const int WHEEL_SPEED_PIN[2] = { 2, 3 };
const int MAX_SPEED = 224; // Clicks/s

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

// Keep track of the number of clicks on the wheel
volatile unsigned int n_clicks[2] = { 0, 0 };
void clickLeft() {
  n_clicks[LEFT]++;
}
void clickRight() {
  n_clicks[RIGHT]++;
}

void initializeThrottle() {
  // Start the servo shield
  for (int i = LEFT; i <= RIGHT; i++) {
    gas_pedal[i].attach(GAS_PIN[i]);
    gas_pedal[i].write(THROTTLE_MIN[i]);
  }
}

int getGamepadPosition(int side) {
  int gamepad_position = analogRead(GAMEPAD_PIN[side]);
  /*String log_line = "";
  log_line.concat("GP ");
  log_line.concat((side == LEFT) ? "L " : "R ");
  log_line.concat(gamepad_position);
  logSerial(log_line);*/
  return gamepad_position;
}


// Mapping the individual controllers with their own resting state values to
// a normalized value between 0 and 1000. Notice that the joysticks are physically
// upside down so we are mapping the max to zero and the min to GAMEPAD_NORMAL_MAX
// so that GAMEPAD_NORMAL_MAX is always full throttle and 0 is brake.
int getGamepadNormal (int side) {
  int gamepad_position = getGamepadPosition(side);
  int gamepad_position_normal = map(gamepad_position, 0, 2 * JOYSTICK_ZERO[side], GAMEPAD_NORMAL_MAX, 0);

  /*String log_line = "";
  log_line.concat("GPN ");
  log_line.concat((side == LEFT) ? "L " : "R ");
  log_line.concat(gamepad_position_normal);
  logSerial(log_line);*/
  return (gamepad_position_normal / 10) * 10;
}

void loop() {
  // put your main code here, to run repeatedly:
  /*int posL = getGamepadNormal(LEFT);
  int posR = getGamepadNormal(RIGHT);
  Serial.print(posL); Serial.print(" "); Serial.println(posR);
  delay(1000 / REFRESH_RATE);*/

  for (int side = LEFT; side <= LEFT; side++) {
    int pos = getGamepadNormal(side);
    if (pos >= GAS_ENGAGE) {
      gas_pedal[side].write(THROTTLE_MIN[side] + (THROTTLE_MAX[side] - THROTTLE_MIN[side]) / 2);
      Serial.print((side == LEFT) ? "L: " : "R: ");
      Serial.print(getSpeedSample(side));
      //Serial.print(getSpeedSmoothed(side));
      Serial.print(" of ");
      Serial.print(MAX_SPEED);
      Serial.println(" clicks per second");
    }
    else {
      gas_pedal[side].write(THROTTLE_MIN[side]);
    }
  }
  delay(1000 / REFRESH_RATE);
}

int getSpeedSample(int side) {
  long clicks = n_clicks[side];
  n_clicks[side] = 0;
  return clicks * REFRESH_RATE;
}

int getSpeedSmoothed(int side) {
  static float weighted_average[2] = { 0, 0 };
  static boolean initialized[2] = { false, false };
  int sample = getSpeedSample(side);
  if (initialized[side]) {
    weighted_average[side] = ALPHA * (float)sample + (1 - ALPHA) * weighted_average[side];
  }
  else {
    weighted_average[side] = (float)sample;
  }

  return (int)weighted_average[side];
}

