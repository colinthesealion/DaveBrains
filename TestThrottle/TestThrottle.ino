#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define LEFT           0
#define RIGHT          1
#define SPEED_LIMIT  140

// Servo constants
const unsigned long THROTTLE_MIN[2] = { 480, 480 };
const unsigned long THROTTLE_MAX[2] = { 200, 200 };

// Servo shield
Adafruit_PWMServoDriver servo_shield;

void setup() {
  // put your setup code here, to run once:
  initializeThrottle();
}

void initializeThrottle() {
  // Start the servo shield
  servo_shield = Adafruit_PWMServoDriver();

  // Analog servos run at ~60 Hz updates
  servo_shield.begin();
  servo_shield.setPWMFreq(60);
}

void loop() {
  // put your main code here, to run repeatedly:
  setThrottle(LEFT, 0);
  delay(500);
  setThrottle(LEFT, SPEED_LIMIT);
  delay(500);
  setThrottle(RIGHT, 0);
  delay(500);
  setThrottle(RIGHT, SPEED_LIMIT);
  delay(500);
  delay(5000);
}

// Set the throttle
void setThrottle(int side, long speed) {
  Serial.print(side); Serial.print(": setting throttle to "); Serial.println(speed);
  
  // Map degrees to pulse length
  long pulse_length = map(speed, 0, SPEED_LIMIT, THROTTLE_MIN[side], THROTTLE_MAX[side]);
  Serial.print(side); Serial.print(": setting pulse length to "); Serial.println(pulse_length);

  // Set the throttle
  servo_shield.setPWM(side, 0, pulse_length);
}
