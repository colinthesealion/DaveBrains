#include <Servo.h>

#define LEFT  0
#define RIGHT 1

#define REFRESH_RATE  30 // How many times per second should we update the throttle

// Gamepad constants
const unsigned int GAMEPAD_PIN[2] = { 0, 1 }; // analog pins for the gamepad
static unsigned long JOYSTICK_ZERO[2];
#define JOYSTICK_MAX 0
#define GAMEPAD_NORMAL_MAX 1000

// Servo constants. When measured, the left throttle was able to move a lot less than the
// right. We are still debugging this. For now, we are setting the range or motion to be
// same number of degrees. In reality the min/max for left was 78/88 and for right 70/90.
// So we will use 78/88 for left and a range or 10 degrees starting at 90 for for right.
// We use 90 because that's rest state whereas 70 tugs to full throttle.
const unsigned int THROTTLE_MIN[2] = { 20, 5 };
const unsigned int THROTTLE_MAX[2] = { 40, 25 };
const unsigned int GAS_PIN[2] = { 9, 10 };
const unsigned int GAS_ENGAGE = 6 * (GAMEPAD_NORMAL_MAX / 10);
Servo gas_pedal[2];

// Wheel speed sensor constants
const int WHEEL_SPEED_PIN[2] = { 2, 3 };
#define MAX_SPEED 224 // Clicks/s
#define STALL_SPEED 30

// PID control
struct PID {
  float p;
  float i;
  float d;
};
const PID K = { 1, 0.1, 0.01 };
const float ALPHA = 0.1; // For smoothing the velocity signal

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
  return gamepad_position;
}

// Mapping the individual controllers with their own resting state values to
// a normalized value between 0 and 1000. Notice that the joysticks are physically
// upside down so we are mapping the max to zero and the min to GAMEPAD_NORMAL_MAX
// so that GAMEPAD_NORMAL_MAX is always full throttle and 0 is brake.
int getGamepadNormal (int side) {
  int gamepad_position = getGamepadPosition(side);
  int gamepad_position_normal = map(gamepad_position, 0, 2 * JOYSTICK_ZERO[side], GAMEPAD_NORMAL_MAX, 0);
  return gamepad_position_normal;
}

void loop() {
  // put your main code here, to run repeatedly:
  for (int side = RIGHT; side <= RIGHT; side++) {
    int desired_speed = getDesiredSpeed(side);
    if (desired_speed > 0) {
      setThrottlePID(side, desired_speed);
    }
    else {
      preventStall(side);
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

  Serial.print("V ");
  Serial.print((side == LEFT) ? "L " : "R ");
  Serial.print(sample);
  Serial.print(" ");
  Serial.println(weighted_average[side]);
  
  return (int)weighted_average[side];
}

void setThrottlePID(int side, int set_point) {
  static int previous_error[2] = { 0, 0 };
  static int integral[2] = { 0, 0 };

  // The error is our desired speed minus our actual speed
  int error = set_point - getSpeedSmoothed(side);

  // The integral is error * dt = error / refresh_rate
  integral[side] += error / (float)REFRESH_RATE;

  // The derivative is (error - previous_error) / dt = (error - previous_error) * refresh_rate
  int derivative = (error - previous_error[side]) * (float)REFRESH_RATE;

  // And PID is pretty straightforward
  int throttle = K.p * error + K.i * integral[side] + K.d * derivative;

  // Save the current error for next iteration
  previous_error[side] = error;

  // Actually depress the pedal
  if (throttle <= 0) {
    gas_pedal[side].write(THROTTLE_MIN[side]);
  }
  else {
    int position = constrain(
      map(throttle, 0, (int)(K.p * MAX_SPEED), THROTTLE_MIN[side], THROTTLE_MAX[side]),
      THROTTLE_MIN[side], THROTTLE_MAX[side]
    );
    gas_pedal[side].write(position);
  }
}

// Once the engine starts moving, dropping the throttle to 0 will cause it to stall
// To prevent this when not actively throttling the engines:
//  1) If the speed is above the stall threshold, turn off the throttle
//  2) Wait for the speed to drop below the stall threshold
//  3) Engage the throttle just a tad
//  4) Wait for one second
//  5) Set the throttle to 0
void preventStall(int side) {
  static int count[2] = { 0, 0};
  static boolean stall_is_possible[2] = { false, false };

  // If our current speed is above STALL_SPEED, there is a chance we could stall
  if (getSpeedSmoothed(side) > STALL_SPEED) {
    stall_is_possible[side] = true;
    count[side] = 0;
    gas_pedal[side].write(THROTTLE_MIN[side]);
  }
  else if (stall_is_possible[side]) {
    // We are under the stall speed after being over the stall speed
    // So prevent a stall by reving the engine just a bit
    if (count[side] == 0) {
      gas_pedal[side].write(THROTTLE_MIN[side] + 5);
    }
    else if (count[side] >= REFRESH_RATE) {
      gas_pedal[side].write(THROTTLE_MIN[side]);
      stall_is_possible[side] = false; // We just finished preventing the stall
    }
    count[side]++;
  }
}

// Return the desird speed in clicks/s
int getDesiredSpeed(int side) {
  int position = getGamepadNormal(side);
  if (position <= 600) {
    Serial.print((side == LEFT) ? "L ": "R ");
    Serial.println("Desired speed is 0");
    return 0; // clicks/s = 0 mph
  }
  else if (position <= 700) {
    Serial.print((side == LEFT) ? "L ": "R ");
    Serial.println("Desired speed is 0.5");
    return 22; // clicks/s = 0.5 mph
  }
  else if (position <= 800) {
    Serial.print((side == LEFT) ? "L ": "R ");
    Serial.println("Desired speed is 1");
    return 44; // clicks/s = 1 mph
  }
  else if (position <= 900) {
    Serial.print((side == LEFT) ? "L ": "R ");
    Serial.println("Desired speed is 3");
    return 134; // clicks/s = 3 mph
  }
  else {
    Serial.print((side == LEFT) ? "L ": "R ");
    Serial.println("Desired speed is 5");
    return 224; // clicks/s = 5 mph
  }
}

