#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>

// DO NOT CHANGE, PIN ASSIGNMENTS
#define LATCHPIN 2
#define DATAPIN 3
#define CLOCKPIN 4
#define DOWNBUTTONPIN 12
#define UPBUTTONPIN 11
#define MODEBUTTONPIN 10
#define ROTCLKPIN 9
#define ROTDTPIN 8
#define ROTBTTNPIN 7

//EEPROM ADDRESSES
#define HOURMODEADDRESS 0
#define ROTATETIMEADDRESS 1

RTC_DS3231 rtc;
void setup()
{
  Serial.begin(57600); //DEBUG

  rtc.begin();
  pinMode(LATCHPIN, OUTPUT);
  pinMode(CLOCKPIN, OUTPUT);
  pinMode(DATAPIN, OUTPUT);
  pinMode(DOWNBUTTONPIN, INPUT);
  pinMode(UPBUTTONPIN, INPUT);
  pinMode(MODEBUTTONPIN, INPUT);
  digitalWrite(DOWNBUTTONPIN, HIGH);
  digitalWrite(UPBUTTONPIN, HIGH);
  digitalWrite(MODEBUTTONPIN, HIGH);

  pinMode(ROTCLKPIN, INPUT);
  pinMode(ROTDTPIN, INPUT);
  pinMode(ROTBTTNPIN, INPUT);

  lastStateCLK = digitalRead(ROTCLKPIN);
}

int hour, minute, second, year, month, day;
int colons;

int currentStateCLK;
int lastStateCLK;
unsigned long lastButtonPress = 0;

int rotatespeed = EEPROM.read(ROTATETIMEADDRESS);  //time in seconds, min 4, max 59
boolean HourMode = EEPROM.read(ROTATETIMEADDRESS); //0 for 12, 1 for 24
boolean dateOrTime = false;
unsigned long lastBlinkTime = 0;
unsigned long blinkOffTime = 200;
unsigned long blinkOnTIme = 800;
int digitMod = 0;
int nextSwitch = 0;
int mode = 0;

//0  Time
//1  Date
//2  Rotate time and date
//3  Set time
//4  Set date
//5  Set 12 or 24 hour
//6  Set rotation speed in secs

int readRotEncoder(int counter)
{
  // Read the current state of CLK
  currentStateCLK = digitalRead(ROTCLKPIN);
  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1)
  {
    // the encoder is rotating CCW so decrement
    if (digitalRead(ROTDTPIN) != currentStateCLK)
    {
      counter--;
    }
    else
    {
      // Encoder is rotating CW so increment
      counter++;
    }
  }
  // Remember last CLK state
  lastStateCLK = currentStateCLK;
  return counter;
}
boolean readRotButton()
{
  // Read the button state
  int btnState = digitalRead(ROTBTTNPIN);
  //If we detect LOW signal, button is pressed
  if (btnState == LOW)
  {
    //if 50ms have passed since last LOW pulse, it means that the
    //button has been pressed, released and pressed again
    if (millis() - lastButtonPress > 50)
    {
      lastButtonPress = millis();
      return true;
    }
    else
    {
      lastButtonPress = millis();
      return false;
    }
    // Remember last button press event
  }
  delay(1);
}
void loop() //bro change all this to rotate with up down buttons. option should edit the settings
{
  delay(10);
  if (digitalRead(UPBUTTONPIN) == LOW)
  {
    delay(200); //fake button debounce
    mode++;
    if (mode > 2)
    {
      mode = 0;
    }
  }
  if (digitalRead(DOWNBUTTONPIN) == LOW)
  {
    delay(200); //fake button debounce
    mode = mode - 1;
    if (mode < 0)
    {
      mode = 0;
    }
  }
  switch (mode)
  {
  case 0:
    timeMode();                            //add 12/24h modes
    if (digitalRead(MODEBUTTONPIN) == LOW) //enter set menu
    {
      delay(200); //fake button debounce
      setTime();
    }
    break;
  case 1:
    dateMode();
    if (digitalRead(MODEBUTTONPIN) == LOW) //enter set menu
    {
      delay(200); //fake button debounce
      setDate();
    }
    break;
  case 2:
    rotateMode();
    break;
  }
}

void timeMode()
{
  getDateTime();

  //printtime(); //debug

  if (second % 2 == 0) //too slow, change to millis timer for 500 milliseconds
  {
    colons = 16;
  }
  else
  {
    colons = 0;
  }

  writeToNixie(hour, minute, second, colons);
}
void dateMode()
{
  getDateTime();

  //printtime(); //debug
  if (second % 2 == 0) //too slow, change to millis timer for 500 milliseconds
  {
    colons = 10;
  }
  else
  {
    colons = 0;
  }
  writeToNixie(month, day, year, colons);
}
void rotateMode()
{
  if (digitalRead(MODEBUTTONPIN) == LOW)
  {
    delay(200); //fake button debounce
    setRotationSpeed();
  }
  if (second == nextSwitch)
  {
    nextSwitch = ((second + rotatespeed) % 60);
    dateOrTime = !dateOrTime;
  }
  if (dateOrTime)
  {
    timeMode();
  }
  else
  {
    dateMode();
  }
}
void setTime()
{
  digitMod = 0;
  while (digitMod < 4)
  {
    delay(50);
    //printtime(); //debug
    if (digitalRead(MODEBUTTONPIN) == LOW)
    {
      lastBlinkTime = millis(); //reset timer
      delay(200);               //fake button debounce
      digitMod++;
    }
    if (digitMod == 3)
    {
      setHourDisplay();
      delay(200); //fake button debounce
      digitMod++;
    }
    if ((millis() - lastBlinkTime) > blinkOnTIme)
    {
      //switch off
      switch (digitMod)
      {
      case 0:
        writeToNixie(255, minute, second, 16); //255 will blank the display
        break;
      case 1:
        writeToNixie(hour, 255, second, 16);
        break;
      case 2:
        writeToNixie(hour, minute, 255, 16);
        break;
      }
      if ((millis() - lastBlinkTime) > blinkOnTIme + blinkOffTime)
      {
        lastBlinkTime = millis(); //reset timer
        writeToNixie(hour, minute, second, 16);
      }
    }
    if (digitalRead(UPBUTTONPIN) == LOW)
    {
      delay(200);               //fake button debounce
      lastBlinkTime = millis(); //reset timer
      writeToNixie(hour, minute, second, 16);
      switch (digitMod)
      {
      case 0:
        hour++;
        if (hour > 23)
        {
          hour = 0;
        }
        break;
      case 1:
        minute++;

        if (minute > 59)
        {
          minute = 0;
        }
        break;
      case 2:
        second++;
        if (second > 59)
        {
          second = 0;
        }
        break;
      }
    }
    if (digitalRead(DOWNBUTTONPIN) == LOW)
    {
      delay(200);               //fake button debounce
      lastBlinkTime = millis(); //reset timer
      writeToNixie(hour, minute, second, 16);
      switch (digitMod)
      {
      case 0:
        hour = hour - 1;
        if (hour < 0)
        {
          hour = 23;
        }
        break;
      case 1:
        minute = minute - 1;
        if (minute < 0)
        {
          minute = 59;
        }
        break;
      case 2:
        second = second - 1;
        if (second < 0)
        {
          second = 59;
        }
        break;
      }
    }
  }
  rtc.adjust(DateTime(year, month, day, hour, minute, second));
}
void setDate()
{
}
void setHourDisplay()
{
  boolean origBool = HourMode;
  while (digitalRead(MODEBUTTONPIN) != LOW)
  {
    delay(100);
    if (digitalRead(UPBUTTONPIN) == LOW)
    {
      delay(200);
      HourMode = !HourMode;
    }
    if (digitalRead(DOWNBUTTONPIN) == LOW)
    {
      delay(200);
      HourMode = !HourMode;
    }
    if (HourMode)
    {
      writeToNixie(12, 255, 255, 0);
    }
    else
    {
      writeToNixie(24, 255, 255, 0);
    }
  }
  if (origBool != HourMode) //thi sis not working
  {
    EEPROM.write(HOURMODEADDRESS, (boolean)HourMode); //commmit change to EEPROM
    delay(500);
  }
}
void setRotationSpeed()
{
  int origRotateSpeed = rotatespeed;
  while (digitalRead(MODEBUTTONPIN) != LOW)
  {
    delay(100);
    if (digitalRead(UPBUTTONPIN) == LOW)
    {
      delay(200);
      rotatespeed++;
      if (rotatespeed > 59)
      {
        rotatespeed = 59;
      }
    }
    if (digitalRead(DOWNBUTTONPIN) == LOW)
    {
      delay(200);
      rotatespeed = rotatespeed - 1;
      if (rotatespeed < 2)
      {
        rotatespeed = 2;
      }
    }
    writeToNixie(255, 255, rotatespeed, 0);
  }
  if (origRotateSpeed != rotatespeed)
  {
    EEPROM.write(ROTATETIMEADDRESS, rotatespeed); //commit change to EEPROM
    delay(500);
  }
}

void getDateTime()
{
  DateTime now = rtc.now();
  hour = now.hour();
  minute = now.minute();
  second = now.second();
  year = now.year();
  month = now.month();
  day = now.day();
}

void writeToNixie(int hours, int minutes, int seconds, int divider) //write data to nixie tubes
{
  printNixieToSerial(hours, minutes, seconds, divider);
  digitalWrite(LATCHPIN, LOW);
  shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, convertToNixe(hours));
  shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, convertToNixe(minutes));
  shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, convertToNixe(seconds));
  shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, divider);
  digitalWrite(LATCHPIN, HIGH);
}

int convertToNixe(int num) //convert 2 digit number to display on nixie tubes
{
  int nixienum;
  if (num == 255) //blank the display
  {
    nixienum = num;
  }
  else
  {
    nixienum = ((num / 10) % 10) | ((num % 10) << 4); //hours (tens digit) or-ed with (ones place shifted 4 left)
  }
  return nixienum;
}

void printNixieToSerial(int hours, int minutes, int seconds, int divider)
{

  Serial.print(hours, DEC);
  Serial.print(':');
  Serial.print(minutes, DEC);
  Serial.print(':');
  Serial.print(seconds, DEC);
  Serial.print('-');
  Serial.print(divider, DEC);
  Serial.print('-');
  Serial.print(mode, DEC);
  Serial.println();
}
void printtime()
{ //debug
  Serial.print(year, DEC);
  Serial.print('/');
  Serial.print(month, DEC);
  Serial.print('/');
  Serial.print(day, DEC);
  Serial.println();
  Serial.print(hour, DEC);
  Serial.print(':');
  Serial.print(minute, DEC);
  Serial.print(':');
  Serial.print(second, DEC);
  Serial.println();
}
