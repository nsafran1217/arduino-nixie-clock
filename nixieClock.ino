#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>

// DO NOT CHANGE, PIN ASSIGNMENTS
#define LATCHPIN 2
#define DATAPIN 3
#define CLOCKPIN 4
//#define DOWNBUTTONPIN 12
//#define UPBUTTONPIN 11
//#define MODEBUTTONPIN 10
#define ROTCLKPIN 10
#define ROTDTPIN 9
#define ROTBTTNPIN 8

//EEPROM ADDRESSES
#define twelveHourModeADDRESS 0
#define ROTATETIMEADDRESS 1
#define TRANSMODEADDRESS 2

RTC_DS3231 rtc;

int hour, minute, second, year, month, day;
int colons;

int currentStateCLK;
int lastStateCLK;
unsigned long lastButtonPress = 0;

int rotatespeed = EEPROM.read(ROTATETIMEADDRESS);            //time in seconds, min 4, max 59
boolean twelveHourMode = EEPROM.read(twelveHourModeADDRESS); //0 for 12, 1 for 24
int TransMode = EEPROM.read(TRANSMODEADDRESS);
boolean dateOrTime = false;
unsigned long lastBlinkTime = 0;
unsigned long blinkTime = 500;
unsigned long waitForDisplay = 0; //for displaying mode
int nextSwitch = 0;
int displayMode = 0;

int lastHourTen, lastHourOne, lastMinuteTen, lastMinuteOne, lastSecondTen, lastSecondOne;
int HourTen, HourOne, MinuteTen, MinuteOne, SecondTen, SecondOne;
int secondsCombined, minutesCombined, hoursCombined;

//0  Time
//1  Date
//2  Rotate time and date

void setup()
{
  Serial.begin(57600); //DEBUG

  rtc.begin();
  pinMode(LATCHPIN, OUTPUT);
  pinMode(CLOCKPIN, OUTPUT);
  pinMode(DATAPIN, OUTPUT);
  //pinMode(DOWNBUTTONPIN, INPUT);
  //pinMode(UPBUTTONPIN, INPUT);
  //pinMode(MODEBUTTONPIN, INPUT);
  //digitalWrite(DOWNBUTTONPIN, HIGH);
  //digitalWrite(UPBUTTONPIN, HIGH);
  //digitalWrite(MODEBUTTONPIN, HIGH);

  pinMode(ROTCLKPIN, INPUT);
  pinMode(ROTDTPIN, INPUT);
  pinMode(ROTBTTNPIN, INPUT);
  digitalWrite(ROTBTTNPIN, HIGH);

  lastStateCLK = digitalRead(ROTCLKPIN);
  getDateTime();
  lastHourTen = (hour / 10) % 10;
  lastHourOne = hour % 10;
  lastMinuteTen = (minute / 10) % 10;
  lastMinuteOne = minute % 10;
  lastSecondTen = (second / 10) % 10;
  lastSecondOne = second % 10;
}

int readRotEncoder(int counter) //takes a counter in and spits it back out if it changes
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
boolean readRotButton() //true if button pressed
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
  else
  {
    return false;
  }
}

int changeMode(int mode, int numOfModes) //numofmodes starts at 0! display mode on tube, easy for selecting
{
  int lastmode = mode;
  mode = readRotEncoder(mode);
  if (mode > 2) //if mode invalid, loop around
  {
    mode = 0;
  }
  else if (mode < 0)
  {
    mode = 2;
  }
  if (mode != lastmode)
  {
    waitForDisplay = millis();
    while (millis() - waitForDisplay < 500)
    {
      delay(10);
      writeToNixie(mode, 255, 255, 0);
      mode = changeMode(mode, numOfModes);
    }
  }
  return mode;
}
void loop() //bro change all this to rotate with up down buttons. option should edit the settings
{
  delay(10);
  if (readRotButton()) //enter set menu
  {
    settingsMenu();
  }
  displayMode = changeMode(displayMode, 2);
  switch (displayMode)
  {
  case 0:
    if (TransMode == 0)
    {
      normalTimeMode();
    }
    else if (TransMode == 1)
    {
      scrollTimeMode();
    }

    break;
  case 1:
    dateMode();
    break;
  case 2:
    rotateMode();
    break;
  }
}

void settingsMenu()
{
  int settingsMode = 0;
  boolean exitSettings = false;
  while (!exitSettings)
  {
    delay(10);
    settingsMode = readRotEncoder(settingsMode);
    writeToNixie(settingsMode, 255, 255, 0);
    if (readRotButton())
    {
      switch (settingsMode)
      {
      case 0:
        exitSettings = true; //exit back to display mode
        break;
      case 1:
        setTime();
        break;
      case 2:
        setDate();
        break;
      case 3:
        setRotationSpeed();
        break;
      case 4:
        setHourDisplay();
        break;
      case 5:
        setTransitionMode();
        break;
      }
    }
    if (settingsMode > 5)
    {
      settingsMode = 0;
    }
    if (settingsMode < 0)
    {
      settingsMode = 5;
    }
  }
}
void normalTimeMode()
{
  getDateTime();
  if (second % 2 == 0) //too slow, change to millis timer for 500 milliseconds
  {
    colons = 15;
  }
  else
  {
    colons = 0;
  }
  if (twelveHourMode)
  {
    if (hour > 12)
    {
      writeToNixie(hour - 12, minute, second, colons);
    }
    else if (hour == 00)
    {
      writeToNixie(12, minute, second, colons);
    }

    else
    {
      writeToNixie(hour, minute, second, colons);
    }
  }
  else
  {
    writeToNixieScroll(hour, minute, second, colons);
  }
}

void scrollTimeMode()
{
  getDateTime();
  if (second % 2 == 0) //too slow, change to millis timer for 500 milliseconds
  {
    colons = 15;
  }
  else
  {
    colons = 0;
  }
  if (twelveHourMode)
  {
    if (hour > 12)
    {
      writeToNixie(hour - 12, minute, second, colons);
    }
    else if (hour == 00)
    {
      writeToNixie(12, minute, second, colons);
    }

    else
    {
      writeToNixie(hour, minute, second, colons);
    }
  }
  else
  {
    writeToNixieScroll(hour, minute, second, colons); //////debug change
  }
}

void dateMode()
{
  getDateTime();

  //printtime(); //debug
  if (second % 2 == 0) //too slow, change to millis timer for 500 milliseconds
  {
    colons = 6;
  }
  else
  {
    colons = 0;
  }
  writeToNixie(month, day, year, colons);
}
void rotateMode()
{
  if (second == nextSwitch)
  {
    nextSwitch = ((second + rotatespeed) % 60);
    dateOrTime = !dateOrTime;
  }
  if (dateOrTime)
  {
    normalTimeMode(); /////////////////maybe change
  }
  else
  {
    dateMode();
  }
}
void setTime()
{
  int lasthour, lastminute, lastsecond;
  int digitMod = 0;
  lastBlinkTime = millis();
  while (digitMod < 3)
  {
    lasthour = hour;
    lastminute = minute;
    lastsecond = second;
    delay(5);
    if (readRotButton())
    {
      lastBlinkTime = millis(); //reset timer
      digitMod++;
    }
    if ((millis() - lastBlinkTime) < blinkTime)
    {

      writeToNixie(hour, minute, second, 15);
    }
    else if (((millis() - lastBlinkTime) > blinkTime * 2))
    {
      writeToNixie(hour, minute, second, 15);
      lastBlinkTime = millis(); //reset timer
    }
    else
    {
      //switch off
      switch (digitMod)
      {
      case 0:
        writeToNixie(255, minute, second, 15); //255 will blank the display
        break;
      case 1:
        writeToNixie(hour, 255, second, 15);
        break;
      case 2:
        writeToNixie(hour, minute, 255, 15);
        break;
      }
    }

    switch (digitMod)
    {
    case 0:
      hour = readRotEncoder(hour);
      if (hour != lasthour)
      {
        lastBlinkTime = millis(); //reset timer
        if (hour > 23)
        {
          hour = 0;
        }
        else if (hour < 0)
        {
          hour = 23;
        }
      }
      break;
    case 1:
      minute = readRotEncoder(minute);
      if (minute != lastminute)
      {
        lastBlinkTime = millis(); //reset timer
        if (minute > 59)
        {
          minute = 0;
        }
        else if (minute < 0)
        {
          minute = 59;
        }
      }
      break;
    case 2:
      second = readRotEncoder(second);
      if (second != lastsecond)
      {
        lastBlinkTime = millis(); //reset timer
        if (second > 59)
        {
          second = 0;
        }
        else if (second < 0)
        {
          second = 59;
        }
      }
      break;
    }
  }
  rtc.adjust(DateTime(year, month, day, hour, minute, second));
}
void setDate()
{
  int lastyear, lastmonth, lastday;
  int digitMod = 0;
  lastBlinkTime = millis();
  while (digitMod < 3)
  {
    lastyear = year;
    lastmonth = month;
    lastday = day;
    delay(5);
    if (readRotButton())
    {
      lastBlinkTime = millis(); //reset timer
      digitMod++;
    }
    if (digitMod == 2)
    {
      writeToNixie(255, year / 100, year % 100, 6);
    }
    else
    {
      if ((millis() - lastBlinkTime) < blinkTime)
      {

        writeToNixie(month, day, year, 6);
      }
      else if (((millis() - lastBlinkTime) > blinkTime * 2))
      {
        writeToNixie(month, day, year, 6);
        lastBlinkTime = millis(); //reset timer
      }
      else
      {
        //switch off
        switch (digitMod)
        {
        case 0:
          writeToNixie(255, day, year, 6); //255 will blank the display
          break;
        case 1:
          writeToNixie(month, 255, year, 6);
          break;
        }
      }
    }

    switch (digitMod)
    {
    case 0:
      month = readRotEncoder(month);
      if (month != lastmonth)
      {
        lastBlinkTime = millis(); //reset timer
        if (month > 12)
        {
          month = 1;
        }
        else if (month < 1)
        {
          month = 12;
        }
      }
      break;
    case 1:
      day = readRotEncoder(day);
      if (day != lastday)
      {
        lastBlinkTime = millis(); //reset timer
        if (day > 31)
        {
          day = 1;
        }
        else if (day < 1)
        {
          day = 31;
        }
      }
      break;
    case 2:
      year = readRotEncoder(year);
      if (year != lastyear)
      {
        lastBlinkTime = millis(); //reset timer
        if (year > 2100)
        {
          year = 2100;
        }
        else if (year < 2020)
        {
          year = 2020;
        }
      }
      break;
    }
  }
  DateTime now = rtc.now();
  hour = now.hour();
  minute = now.minute();
  second = now.second();
  rtc.adjust(DateTime(year, month, day, hour, minute, second));
}
void setHourDisplay()
{
  boolean origBool = twelveHourMode;
  int counter = 0;
  if (twelveHourMode)
  {
    counter = 1;
  }
  while (!readRotButton())
  {
    delay(10);
    counter = readRotEncoder(counter);
    if (counter % 2 == 0)
    {
      twelveHourMode = false;
    }
    else
    {
      twelveHourMode = true;
    }
    if (twelveHourMode)
    {
      writeToNixie(12, 255, 255, 0);
    }
    else
    {
      writeToNixie(24, 255, 255, 0);
    }
  }
  if (origBool != twelveHourMode) //thi sis not working
  {
    EEPROM.write(twelveHourModeADDRESS, (boolean)twelveHourMode); //commmit change to EEPROM
  }
}
void setRotationSpeed()
{
  int origRotateSpeed = rotatespeed;
  while (!readRotButton())
  {
    delay(10);
    rotatespeed = readRotEncoder(rotatespeed);
    if (rotatespeed > 59)
    {
      rotatespeed = 59;
    }
    writeToNixie(255, 255, rotatespeed, 0);
  }
  if (origRotateSpeed != rotatespeed)
  {
    EEPROM.write(ROTATETIMEADDRESS, rotatespeed); //commit change to EEPROM
  }
}

void setTransitionMode()
{
  int origTransMode = TransMode;
  while (!readRotButton())
  {
    delay(10);
    TransMode = readRotEncoder(TransMode);
    if (TransMode > 1)
    {
      TransMode = 0;
    }
    if (TransMode < 0)
    {
      TransMode = 1;
    }
    writeToNixie(255, 255, TransMode, 0);
  }
  if (origTransMode != TransMode)
  {
    EEPROM.write(TRANSMODEADDRESS, TransMode); //commit change to EEPROM
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
  printNixieToSerial(hours, minutes, seconds, divider); //degug
  digitalWrite(LATCHPIN, LOW);
  shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, divider);
  shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, convertToNixe(seconds));
  shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, convertToNixe(minutes));
  shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, convertToNixe(hours));
  digitalWrite(LATCHPIN, HIGH);
}

void writeToNixieRAW(int hours, int minutes, int seconds, int divider) //write data to nixie tubes RAW
{
  //printNixieToSerial(hours, minutes, seconds, divider);
  digitalWrite(LATCHPIN, LOW);
  shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, divider);
  shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, seconds);
  shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, minutes);
  shiftOut(DATAPIN, CLOCKPIN, MSBFIRST, hours);
  digitalWrite(LATCHPIN, HIGH);
}

void writeToNixieScroll(int hours, int minutes, int seconds, int divider) //this will be annoying i need an efficiant way to do this. or brute force it
{ //this is so messy I hate it

  HourTen = (hours / 10) % 10;
  HourOne = hours % 10;
  MinuteTen = (minutes / 10) % 10;
  MinuteOne = minutes % 10;
  SecondTen = (seconds / 10) % 10;
  SecondOne = seconds % 10;
  ///////

  boolean scrollDigitBool[6] = {SecondOne != lastSecondOne, SecondTen != lastSecondTen, MinuteOne != lastMinuteOne, MinuteTen != lastMinuteTen, HourOne != lastHourOne, HourTen != lastHourTen};
  if (scrollDigitBool[0])
  {
    for (int i = 0; i < 11; i++)
    {
      lastSecondOne++;
      if (scrollDigitBool[1])
      {
        lastSecondTen++;
      }
      if (scrollDigitBool[2])
      {
        lastMinuteOne++;
      }
      if (scrollDigitBool[3])
      {
        lastMinuteTen++;
      }
      if (scrollDigitBool[4])
      {
        lastHourOne++;
      }
      if (scrollDigitBool[5])
      {
        lastHourTen++;
      }
      secondsCombined = (lastSecondTen % 10) | ((lastSecondOne % 10) << 4);
      minutesCombined = (lastMinuteTen % 10) | ((lastMinuteOne % 10) << 4);
      hoursCombined = (lastHourTen % 10) | ((lastHourOne % 10) << 4);
      writeToNixieRAW(hoursCombined, minutesCombined, secondsCombined, divider);
      delay(75);
    }
    if (lastSecondTen == 16)
    {
      lastSecondTen = 0;
      if (lastMinuteTen == 16)
      {
        lastMinuteTen = 0;
        if (twelveHourMode)
        {
          if (lastHourOne == 13)
          {
            lastHourOne = 1;
            lastHourTen = 0;
          }

        } else {
          if (lastHourOne == 14)
          {
            lastHourOne = 0;
            lastHourTen = 0;
          }
        }
      }
      secondsCombined = (lastSecondTen % 10) | ((lastSecondOne % 10) << 4);
      minutesCombined = (lastMinuteTen % 10) | ((lastMinuteOne % 10) << 4);
      hoursCombined = (lastHourTen % 10) | ((lastHourOne % 10) << 4);
      writeToNixieRAW(hoursCombined, minutesCombined, secondsCombined, divider);
    }
  }

  lastHourTen = (hours / 10) % 10;
  lastHourOne = hours % 10;
  lastMinuteTen = (minutes / 10) % 10;
  lastMinuteOne = minutes % 10;
  lastSecondTen = (seconds / 10) % 10;
  lastSecondOne = seconds % 10;
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
  Serial.print(displayMode, DEC);
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
