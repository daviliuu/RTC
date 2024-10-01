#include <LiquidCrystal.h>

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Arduino Inputs:
// Pins
int temperatureSensor = A5;
int yellowButton = 6;
int greenButton = 5;
int redButton = 4;
int blueButton = 3;
int speakerPin = 13;

// Button debouncing
long lastYellowPress = 0;
long lastGreenPress = 0;
long lastRedPress = 0;
long lastBluePress = 0;

// Program Settings:
// Initial Time Parameters:
int hour = 0;
int minute = 0;
int second = 0;
bool use12Hour = true;
bool showTemp = true;
long lastRecordedTime = 0;

// Alarm Parameters
bool soundAlarm = false;
bool setAfterTime = false;
int alarmHour = -1;
int alarmMin = -1;
int pomodoroFocusMin = 25;

// Temp Parameters:
int celcius = 0;

// LCD Parameters
int lcdCols = 16;
int lcdRows = 2;

// Cursor Positioning & Options
// {0: "main", 1: "settings", 2: "time", 3: "options"}
int displayMode = 0;

String settingsOptions[5] = {"Options", "Set Time", "Set Alarm", "Set Timer", "Set Pomodoro"};
int settingsCursor = 0;

int timerTime[4] = {0, 0, 0, 0};
int timerCursor = 0;

String options[2] = {"12 Hour", "Show Temp"};
int optionsCursor = 0;

void setup() {
  pinMode(temperatureSensor, INPUT);
  pinMode(yellowButton, INPUT_PULLUP);
  pinMode(greenButton, INPUT_PULLUP);
  pinMode(redButton, INPUT_PULLUP);
  pinMode(blueButton, INPUT_PULLUP);
  pinMode(speakerPin, OUTPUT);
  Serial.begin(9600);
  lcd.begin(lcdCols, lcdRows);
  getTemperature();
  displayMain();
}

void loop() {
  updateTimeTemp();
  handleButtonInput();
  checkAlarm();
}


// Updates the time every minute, adding a minute or setting the next hour
// Updates the temperature reading
// If the screen is on the main menu, updates the menu.
// Otherwise, does this in the background
void updateTimeTemp() {
  long currentTime = millis();
  
  if (currentTime - lastRecordedTime >= 60000) {
    if (minute == 59) {
      minute = 0;
      hour += 1;
    }
    else {
      minute += 1;
    }

    hour %= 24;
    minute %= 60;

    lastRecordedTime = currentTime;
    getTemperature();
    // Ensures alarms set before the current time do not ring until it reaches that time
    if (setAfterTime) {
      setAfterTime = !(hour == 23 && minute == 59);
    }
    if (displayMode == 0) {
      displayMain();
    }
  }
}

// Reads the temperature from the TMP36 Temperature Sensor
void getTemperature() {
  double voltage = (analogRead(temperatureSensor) / 1023.0);
  double temp = ((voltage * 5) - 0.5) * 100;
  celcius = int(round(temp));
}

// Makes tone to speaker when its time for the alarm
void checkAlarm() {
  soundAlarm = hour >= alarmHour && minute >= alarmMin && alarmHour != -1 && alarmMin != -1 && !setAfterTime;

  if (soundAlarm && millis() % 2000 == 0) {
    tone(speakerPin, 432, 1000);
  }
}

// Input Handlers
// Reads button input and delegates the correct function based on the input
void handleButtonInput() {
  if (digitalRead(yellowButton) == LOW && millis() - lastYellowPress > 500) {
    lastYellowPress = millis();
    handleYellowButton();
  }
  else if (digitalRead(greenButton) == LOW && millis() - lastGreenPress > 500) {
    lastGreenPress = millis();
    handleGreenButton();
  }
  else if (digitalRead(redButton) == LOW && millis() - lastRedPress > 500) {
    lastRedPress = millis();
    handleRedButton();
  }
  else if (digitalRead(blueButton) == LOW && millis() - lastBluePress > 500) {
    lastBluePress = millis();
    handleBlueButton();
  }
}

// Handles the functions of the yellow button
// 0 : Main - Changes to Settings Menu
// 1 : Settings - Changes to Main Menu
// 2 : Time Setting - Moves Cursor Left
// 3 : Options - Changes to Settings Menu
void handleYellowButton() {
  if (displayMode == 1) {
    displayMain();
  }
  else if (displayMode == 2) {
    if (timerCursor > 0) {
      incrementSkip(timerCursor, 2, -1);
      displayTimeSet();
    }
    else {
      resetTimer();
      displaySettings();
    }
  }
  else {
    displaySettings();
  }
}

// Green Button Functions
// 0 : Main - Snooze Alarm for 5 Minutes
// 1 : Settings - Moves Cursor Downward
// 2 : Time Setting - Increases Time
// 3 : Options - Moves Cursor Downward
void handleGreenButton() {
  if (displayMode == 0 && soundAlarm) {
    soundAlarm = false;
    addToAlarm(0, 5);
    displayMain();
  }
  else if (displayMode == 1 && settingsCursor > 0) {
    settingsCursor -= 1;
    displaySettings(); // todo if time allows: optimize displaying
  }
  else if (displayMode == 2) {
    int actualCursor = timerCursor;
    // To adjust past the invalid ":" in the time set menu
    if (actualCursor > 1) {
      actualCursor -= 1;
    }
    
    timerTime[actualCursor] += 1; // Increment the time
    correctTimer();
    displayTimeSet();
  }
  else if (displayMode == 3 && optionsCursor < 1) {
    optionsCursor += 1;
    displayOptions();
  }
}

// Red Button Functions
// 0 : Main - N/A
// 1 : Settings - Moves Cursor Upward
// 2 : Time Setting - Decreases Time
// 3 : Options - Moves Cursor Upward
void handleRedButton() {
  if (displayMode == 1 && settingsCursor < 4) {
    settingsCursor += 1;
    displaySettings(); // todo if time allows: optimize displaying
  }
  else if (displayMode == 2) {
    // To adjust past the invalid ":" in the time set menu
    int actualCursor = timerCursor;
    if (actualCursor > 1) {
      actualCursor -= 1;
    }
    
    timerTime[actualCursor] -= 1; // Decrement the time
    correctTimer();
    displayTimeSet();
  }
  else if (displayMode == 3 && optionsCursor > 0) {
    optionsCursor -= 1;
    displayOptions();
  }
}

// Blue Button Functions
// 0 : Main - Removes Alarm
// 1 : Settings - Selects Highlighted Option
// 2 : Time Setting - Step Next or Finish
// 3 : Options - Enable or Disable
void handleBlueButton() {
  if (displayMode == 0) {
    soundAlarm = false;
    alarmHour = -1;
    alarmMin = -1;
    displayMain();
  }
  else if (displayMode == 1) {
    if (settingsCursor == 0) {
      displayOptions();
    }
    else if (settingsCursor == 4) {
      setAlarm(0, pomodoroFocusMin, false);
      displayMain();
    }
    else {
      displayTimeSet();
    }
  }
  else if (displayMode == 2) {
    if (timerCursor == 4) {
      finalizeTimeSet();
    }
    else {
      incrementSkip(timerCursor, 2, 1);
      displayTimeSet();
    }
  }
  else if (displayMode == 3) {
    if (optionsCursor == 0) {
      use12Hour = !use12Hour;
    }
    else {
      showTemp = !showTemp;
    }
    
    displayOptions();
  }
}

// Display Functions
// Main
// Displays the Main Menu with Time, Temperature, and the Alarm (if any)
void displayMain() {
  displayMode = 0;
  lcd.clear();
  lcd.setCursor(0, 0);

  String timeOutput = formatTime(hour, minute);
  String formattedStr = fillGap(timeOutput, String(celcius) + "C");

  if (showTemp) {
    lcd.print(formattedStr);
  }
  else {
    lcd.print(timeOutput);
  }
  
  if (alarmHour != -1 && alarmMin != -1) {
    String alarmOutput = fillGap("Alarm:", formatTime(alarmHour, alarmMin));
    lcd.setCursor(0, 1);
    lcd.print(alarmOutput);
  }
}

// Settings
// Displays the Settings Menu where options can be selected and an alarm/timer can be set
void displaySettings() {
  displayMode = 1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.noCursor();
  String option1, option2;

  // Determining what to show on the lcd
  if (settingsCursor == 4) {
    option1 = settingsOptions[3];
    option2 = ">> " + settingsOptions[4];
  }
  else {
    option1 = ">> " + settingsOptions[settingsCursor];
    option2 = settingsOptions[settingsCursor + 1];
  }

  lcd.print(option1);
  lcd.setCursor(0, 1);
  lcd.print(option2);
}

// Time Setting
// Uses setting cursor to determine which type of menu to open
// Allows the user to set the time, an alarm, or a timer
void displayTimeSet() {  
  displayMode = 2;
  int cursorLine = 1;
  lcd.noCursor();

  String timerTitle;
  switch (settingsCursor) {
    case 1: 
    {
      timerTitle = "Set the time:";
      break;
    }
    case 2: 
    {
      timerTitle = "Set alarm:";
      break;
    }
    case 3: 
    {
      timerTitle = "Set timer:";
      break;
    }
    case 4: 
    {
      timerTitle = "Set pomodoro:";
      break;
    }
  }

  String timerInput = String(timerTime[0]) + String(timerTime[1]) + ":" +  String(timerTime[2]) + String(timerTime[3]);
  if (settingsCursor == 3) {
    timerInput = fillGap(timerInput, "(hh:mm)");
  }
  else if (use12Hour) {
    int timerHour = timerTime[0] * 10 + timerTime[1];
    int timerMin = timerTime[2] * 10 + timerTime[3];
    timerInput = fillGap(timerInput, "(" + formatTime(timerHour, timerMin) + ")");
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(timerTitle);
  lcd.setCursor(0, 1);
  lcd.print(timerInput);
  lcd.setCursor(timerCursor, 1);
  lcd.cursor();
}

// Options
// Additional settings or preferences changable by user
void displayOptions() {
  lcd.clear();
  lcd.setCursor(0, 0);
  displayMode = 3;
  String firstLine = options[0];
  String secondLine = options[1];

  if (optionsCursor == 0) {
    firstLine = ">> " + firstLine;
  }
  else {
    secondLine = ">> " + secondLine;
  }

  if (use12Hour) {
    firstLine = fillGap(firstLine, "[X]");
  }
  else {
    firstLine = fillGap(firstLine, "[ ]");
  }
  
  if (showTemp) {
    secondLine = fillGap(secondLine, "[X]");
  }
  else {
    secondLine = fillGap(secondLine, "[ ]");
  }

  lcd.print(firstLine);
  lcd.setCursor(0, 1);
  lcd.print(secondLine);
}


// Helpers
// Given an hour and minute, displays the time in:
// 12 Hour Format: (XX:XX am/pm)
// 24 Hour Format: (XX:XX)
String formatTime(int hour, int minute) {
  String timeOutput;
  
  if (use12Hour) {
    if (hour > 12) {
      timeOutput = fillTime(hour % 12) + ":" + fillTime(minute) + "pm"; // fix
    }
    else if (hour == 12) {
      timeOutput = fillTime(hour) + ":" + fillTime(minute) + "pm";
    }
    else if (hour == 0) {
      timeOutput = "12:" + fillTime(minute) + "am";
    }
    else {
      timeOutput = fillTime(hour) + ":" + fillTime(minute) + "am";
    }
  }
  else {
    timeOutput = fillTime(hour) + ":" + fillTime(minute);
  }

  return timeOutput;
}

// Fills the tens digit if none is present:
// 7 -> 07
// 8 -> 08
// 10 -> 10 (does nothing)
String fillTime(int num) {
  String time = "";

  if (num / 10 == 0) {
    time += "0" + String(num);
  }
  else {
    time += String(num);
  }

  return time;
}

// Creates a String given two strings such that a gap is between them
// "A", "B" -> "A               B"
// Used specifically for lcd display (replace lcdCols to re-use)
String fillGap(String first, String second) {
  String gap = "";
  
  for (int i = 0; i < lcdCols - first.length() - second.length(); i += 1) {
    gap += " ";  
  }

  return first + gap + second;
}

// Increments inCursor by amount
// If the result is skip, increments again
// 1, 3, 1 => 2
// 1, 2, 1 => 3 (Skips 2)
int incrementSkip(int& inCursor, int skip, int amount) {
  inCursor += amount;
  if (inCursor == skip) {
    inCursor += amount;
  }
}

// Corrects time created by time cursor by setting the invalid value to 0
void correctTimer() {
  // Ensuring time is allowable at timeCursor =
  // 0: Tens place of hour    (2)3:59
  // 1: Ones place of hour    2(3):59
  // 3: Tens place of minutes 23:(5)9
  // 4: Ones place of minutes 23:5(9)

  for (int i = 0; i < 4; i += 1) {
    int actualCursor = i;
    bool overloadTensHour = actualCursor == 0 && timerTime[actualCursor] > 2;
    bool overloadOnesHour = actualCursor == 1 && ((timerTime[actualCursor - 1] == 2 && timerTime[actualCursor] >= 4) || timerTime[actualCursor] > 9);
    bool overloadTensMin = actualCursor == 2 && timerTime[actualCursor] > 5;
    bool overloadOnesMin = actualCursor == 3 && timerTime[actualCursor] > 9;
    
    if (overloadTensHour || overloadOnesHour || overloadTensMin || overloadOnesMin || timerTime[actualCursor] < 0) {
      timerTime[actualCursor] = 0;
    } 
  }
}

// If "Set Time" is chosen, sets the system time to the given time
// Otherwise, creates an alarm given the data inputted in the time setting menu
void finalizeTimeSet() {
  // 1 : Time
  // 2 : Alarm
  // 3 : Timer
  // 4 : Pomodoro
  if (settingsCursor == 1) {
    hour = timerTime[0] * 10 + timerTime[1];
    minute = timerTime[2] * 10 + timerTime[3];
    lastRecordedTime = millis();

    if (alarmHour > hour || alarmHour == hour && alarmMin >= minute) {
      setAfterTime = false;
    }
  }
  else {
    int timeHour = timerTime[0] * 10 + timerTime[1];
    int timeMin = timerTime[2] * 10 + timerTime[3];
    setAlarm(timeHour, timeMin, settingsCursor == 2);
  }

  settingsCursor = 0; // Resetting settingsCusor for when user opens menu next time
  resetTimer();
  displayMain();
}

// Resetting timerCusor: User starts at first input (0)0:00
// Resetting timerTime: User sees 00:00 when menu opens
void resetTimer() {
  timerCursor = 0;
  for (int i = 0; i < 4; i += 1) {
    timerTime[i] = 0;
  }
}

// Sets an alarm given the hour and minute of the alarm
// Sets a timer if boolean alarm is false
void setAlarm(int timeHour, int timeMin, boolean alarm) {
  if (alarm) {
    alarmHour = timeHour;
    alarmMin = timeMin;
  }
  else {
    alarmHour = hour;
    alarmMin = minute;
    addToAlarm(timeHour, timeMin);
  }

  setAfterTime = alarmHour < hour || (alarmHour == hour && alarmMin < minute);
}

// Adds more time to an active alarm
void addToAlarm(int addHour, int addMinute) {
    alarmHour += addHour;
    alarmMin += addMinute;
    if (alarmMin > 59) {
      alarmMin -= 60;
      alarmHour += 1;
    }
    alarmHour %= 24;
}
