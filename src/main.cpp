#include <Arduino.h> 
#include <Wire.h> 
#include <stdint.h> 
#include <math.h>
// libraries for led screen and accel
#include "SparkFun_BMI270_Arduino_Library.h"
#include <SparkFun_Qwiic_OLED.h>

// Create objects for the screen and accelerometer

QwiicMicroOLED myOLED;
BMI270 imu;

volatile int buttonCounter = 0;
bool prevPressed = false;
int switchPin = 10;
unsigned long debounceDelay = 50;
unsigned long doublePressTime = 500;
unsigned long prevTime = 0;
unsigned long timePressed;

float theta = 0.0;
float psi = 0.0;
float phi = 0.0;

int arrowLeftX[] = {0,  1, 1, 1,  2,  2, 2, 2, 2,  3,  3,  3, 3, 3, 3, 3};
int arrowLeftY[] = {0, -1, 0, 1, -2, -1, 0, 1, 2, -3, -2, -1, 0, 1, 2, 3};

//int arrowUpX[] = {0, -1};
char pout[30];

enum PressType {
  NoPress, // 0
  SinglePress, //1 
  DoublePress, //2
  LongPress //3
};

enum MachineState {
  OffState,
  TwoAxis,
  XAxis,
  YAxis,
  RawData,
  StateLength
};

volatile PressType currentPress = NoPress;
volatile MachineState currentState = OffState;

// Triangle draw function
void drawTriangle(int xOff, int yOff, int xDir, int yDir, bool swap) {
  if (!swap) {
    for (int i = 0; i < 16; i++) {
      myOLED.pixel(xOff + xDir * arrowLeftX[i], yOff + yDir * arrowLeftY[i], 255);
    }
  }
  else {
    for (int i = 0; i < 16; i++) {
      myOLED.pixel(xOff + xDir * arrowLeftY[i], yOff + yDir * arrowLeftX[i], 255);
    }
  }
}

float getXAngle() {
  return atan2(imu.data.accelX, sqrt(imu.data.accelY * imu.data.accelY + imu.data.accelZ * imu.data.accelZ)) * 180.0 / PI;
}

float getYAngle() {
  return atan2(imu.data.accelY, sqrt(imu.data.accelX * imu.data.accelX + imu.data.accelZ * imu.data.accelZ)) * 180.0 / PI; 
  //phi = atan2(sqrt(imu.data.accelY * imu.data.accelY + imu.data.accelX * imu.data.accelX), imu.data.accelZ);
}

void buttonPress() {
  unsigned long currentTime = millis();
  if (currentTime - prevTime > debounceDelay) {
    prevPressed = true;
    timePressed = currentTime;
    if (currentTime - prevTime < doublePressTime) {
      currentPress = DoublePress;
    } 
    else {
      currentPress = SinglePress;
    }
  }
  prevTime = currentTime;
}

void setup() {
  delay(1000);
  Serial.begin(9600); 
 
  pinMode(switchPin, INPUT_PULLDOWN);
  attachInterrupt(switchPin, buttonPress, RISING); 

  // Start accel
  Wire.begin();
  while (!myOLED.begin()) {
    delay(1000);
  }

  // Start LED screen
  while (imu.beginI2C(0x68) != BMI2_OK) {
    delay(1000);
  }
  Serial.println("Everything started");
}

void loop() {
  if (currentPress == DoublePress ) { //&& currentState != OffState
    currentState = (MachineState)(((int)currentState + 1) % (int)StateLength);
    currentState = (MachineState)max((int)currentState, 1);
  }
  else if (currentPress == LongPress) {
    currentState = (MachineState)OffState;
  }

  if (currentPress != NoPress) {
    Serial.print("Current State type: ");
    Serial.println((int)currentState);
    currentPress = NoPress;
  }
  
  if (digitalRead(switchPin) && prevPressed) {
    if ((millis() - timePressed) >= 3000) {
      currentPress = LongPress;
      prevPressed = false;
    }
  }


  // erase screen
  myOLED.erase();
  // get imu data
  imu.getSensorData();
  theta = getXAngle();
  psi = getYAngle();

  Serial.println(currentPress);

  switch (currentState)
  {
  case OffState:
    break;
  case TwoAxis: 
    if (theta > 0) {
      drawTriangle(0, 23, 1, 1, false);
    }
    else {
      drawTriangle(63, 23, -1, 1, false);
    }
    if (psi > 0.0) {
      drawTriangle(32, 0, 1, 1, true);
    }
    else {
      drawTriangle(32, 45, 1, -1, true);
    }

    break;
  case XAxis: 
    if (theta > 0) {
      drawTriangle(0, 23, 1, 1, false);
    }
    else {
      drawTriangle(63, 23, -1, 1, false);
    }
    break;
  case YAxis: 
    if (psi > 0.0) {
      drawTriangle(32, 0, 1, 1, true);
    }
    else {
      drawTriangle(32, 45, 1, -1, true);
    }
    break;
  case RawData:
    sprintf(pout, "ax: %.2f", imu.data.accelX);
    myOLED.text(0,0, pout);
    sprintf(pout, "ay: %.2f", imu.data.accelY);
    myOLED.text(0,10, pout);
    sprintf(pout, "az: %.2f", imu.data.accelZ);
    myOLED.text(0,20, pout);

    break;
  default:
    break;
  }
  myOLED.display();
   
}