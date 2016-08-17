#define REFRESH_RATE 30
#define LEFT  0
#define RIGHT 1

const int WHEEL_SPEED_PIN[2] = { 2, 3 };
const float WHEEL_DIAMETER = 0.1524; // meters
const float WHEEL_CIRCUMFERENCE = PI * WHEEL_DIAMETER;
const int CLICKS_PER_REVOLUTION = 40;
const float METERS_PER_CLICK = WHEEL_CIRCUMFERENCE / CLICKS_PER_REVOLUTION;
const float ALPHA = 0.1;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  initializeWheelSpeedSensors();
}

void initializeWheelSpeedSensors() {
  //attachInterrupt(digitalPinToInterrupt(WHEEL_SPEED_PIN[LEFT]), clickLeft, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(WHEEL_SPEED_PIN[RIGHT]), clickRight, CHANGE);
  pinMode(WHEEL_SPEED_PIN[LEFT], INPUT);
  pinMode(WHEEL_SPEED_PIN[LEFT], OUTPUT);
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
  float current_speed = n_clicks[side] * METERS_PER_CLICK * REFRESH_RATE;
  n_clicks[side] = 0;
  if (initialized[side]) {
    floating_average[side] = ALPHA * current_speed + (1 - ALPHA) * floating_average[side];
  }
  else {
    floating_average[side] = current_speed;
    initialized[side] = true;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(digitalRead(WHEEL_SPEED_PIN[RIGHT]));
  //Serial.print("LEFT: ");
  //Serial.print(n_clicks[LEFT]);
  //Serial.print(" ");
  //Serial.println(getActualSpeed(LEFT));

//  Serial.print("RIGHT: ");
//  Serial.print(n_clicks[RIGHT]);
//  Serial.print(" ");
//  Serial.println(getActualSpeed(RIGHT));
  delay(1000 / REFRESH_RATE);
}
