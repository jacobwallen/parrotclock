// Parrot Clock Version 4.0
// Aircombat timing device
//
// Constructed and Coded by Jacob Wallen 2014-2018
/*##############################################
Includes
##############################################*/
#include <Servo.h>
#include <AButton.h>
#include <ATimer.h>
#include <LedControl.h>
/*##############################################
Variable Declaration
##############################################*/
// Creates a timer with resolution 50ms
ATimer timer(50);
// Heat Times
const unsigned long stdTime = 420000; // 420000 = 7 minutes in milliseconds
const unsigned long elacTime = 300000; // 300000 = 5 minutes
const word thirtysecs = 30000; // 30000 = 30 seconds
unsigned long heatTime = 0; // Holds the heat time used
// Servo
Servo servo;
byte angle = 10; // Servo angle value valid: 0-179
byte prevAngle = angle - 1; // Init prev angle to something impossible to force update on next iteration
const byte servoMin = 7;
const byte servoMax = 145;
// IO pins
// Servo attaches to 5 during setup()
const byte siren = 6;
const byte horn = 8;
// Segment display: Pin 12 to DIN, 11 to Clk, 10 to LOAD, no.of devices is 1
LedControl lc = LedControl(12, 11, 10, 1); // LED-Display pins and 1 for amount
const byte elacSwitch = 7;
const byte parrotSwitch = 14;
AButton redButton(15, 1); // Pin number 15(Analog 1), 1 for INPUT_PULLUP
AButton blueButton(16, 1);
AButton greenButton(17, 1);
AButton yellowButton(18, 1);
// Siren parameters
byte siren_speed = 40;
bool hit_max = false;
int siren_pulses = 2;
short ramp_direction = 1;
unsigned long updateSirenSpeed = 0;
const byte siren_max = 145;
const byte siren_min = 25;
bool startSirenStart = false;
// Mode Variables
String systemState = "idle"; // The systems state
bool pause = false; // System pause. Unneccessary?
bool parrotMode;
bool softparrotSwitch = false; // Debugging only
bool elacMode;
bool softelacSwitch = false; // Debugging only
// Rolling Text parameters
int L = 0; // Leading Character integer (Rolling text)
int P = 0; // Position integer (Rolling text)
int rtPause = 200; // Rolling text pause in ms
int lastSecond = 61; // used for ledDisplay, uses 61 as faux second.
unsigned long currentMillis; // reference time
unsigned long targetMillis; // time in future when to allow update of ledTimer
/*##############################################
Program Start
##############################################*/
void setup()
{
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);
  pinMode(horn, OUTPUT);
  pinMode(siren, OUTPUT);
  pinMode(parrotSwitch, INPUT_PULLUP);
  pinMode(elacSwitch, INPUT_PULLUP);
  targetMillis = millis() + rtPause; // Used for rolling text
  servo.attach(5); // attaches the servo on pin 5
  servo.write(angle);
  lc.shutdown(0, false); // Enable display
  lc.setIntensity(0, 15); // Set brightness level (0 is min, 15 is max)
  lc.clearDisplay(0); // Clear display register
}

// Read serial input for soft control and out of the box testing
void checkSerialrx()
{
  int incoming;
  if (Serial.available() > 1)
  {
    incoming = Serial.parseInt();
  }
  if (incoming == 10)
  {
    Serial.print("Soft green button released\n");
    greenButton.force_released();
  }
  if (incoming == 20)
  {
    Serial.print("Soft yellow button released\n");
    yellowButton.force_released();
  }
  if (incoming == 60)
  {
    // Set softelacSwitch true
    if (!softelacSwitch)
    {
      softelacSwitch = true;
      Serial.print("softelacSwitch: On\n");
    }
    else
    {
      softelacSwitch = false;
      Serial.print("softelacSwitch: Off\n");
    }
  }
  if (incoming == 61)
  {
    // Get state of elacMode
    if (elacMode)
    {
      Serial.print("elacMode: On\n");
    }
    else
    {
      Serial.print("elacMode: Off\n");
    }
  }
  if (incoming == 70)
  {
    // Set softparrotSwitch true
    if (!softparrotSwitch)
    {
      softparrotSwitch = true;
      Serial.print("softparrotSwitch: On\n");
    }
    else
    {
      softparrotSwitch = false;
      Serial.print("softparrotSwitch: Off\n");
    }
  }
  if (incoming == 71)
  {
    // Get state of parrotMode
    if (parrotMode)
    {
      Serial.print("parrotMode: On\n");
    }
    else
    {
      Serial.print("parrotMode: Off\n");
    }
  }
  if (incoming == 99)
  {
    Serial.print("Soft red button released\n");
    redButton.force_released();
  }
  return;
}

// Output correct text on the segment display
void ledText(String ledtext, bool clean)
{
  if (clean)
  {
    lc.clearDisplay(0);
  }
  if (ledtext == "heat")
  {
    lc.setChar(0, 7, 'H', false);
    lc.setChar(0, 6, 'e', false);
    lc.setChar(0, 5, 'a', false);
    lc.setRow(0, 4, 15); // t
    return;
  }
  if (ledtext == "prep")
  {
    lc.setChar(0, 7, 'P', false);
    lc.setRow(0, 6, 5); // r
    lc.setChar(0, 5, 'E', false);
    lc.setChar(0, 4, 'P', false);
    return;
  }
  if (ledtext == "pause")
  {
    lc.setChar(0, 7, 'P', false);
    lc.setChar(0, 6, 'A', false);
    lc.setRow(0, 5, 62); // U
    lc.setRow(0, 4, 91); // S
    return;
  }
  if (ledtext == "stop")
  {
    lc.setRow(0, 7, 91); // S
    lc.setRow(0, 6, 15); // t
    lc.setRow(0, 5, 126); // O
    lc.setChar(0, 4, 'P', false);
    return;
  }
  if (ledtext == "end")
  {
    lc.setChar(0, 7, 'E', false);
    lc.setRow(0, 6, 21); // n
    lc.setChar(0, 5, 'd', false);
    return;
  }
  if (ledtext == "horn")
  {
    lc.setChar(0, 7, 'H', false);
    lc.setRow(0, 6, 29); // o
    lc.setRow(0, 5, 5); // r
    lc.setRow(0, 4, 21); // n
    return;
  }
  if (ledtext == "siren")
  {
    lc.setRow(0, 7, 91); // S
    lc.setRow(0, 6, 16); // i
    lc.setRow(0, 5, 5); // r
    lc.setRow(0, 4, 21); // n
    return;
  }
}

// Output correct time on the segment display
void updateLedClock()
{
  // Debugging
  if (timer.get_s() != lastSecond)
  {
    Serial.print(timer.get_m());
    Serial.print(":");
    Serial.print(timer.get_s());
    Serial.print("\n");
  }
  // Print the time
  byte seconds = timer.get_s();
  if (seconds != lastSecond)
  {
    lastSecond = seconds;
    byte ones = seconds % 10;
    byte tens = (seconds/10) % 10;
    lc.setChar(0, 3, ' ', false);
    lc.setDigit(0, 2, timer.get_m(), true); // Minutes
    lc.setDigit(0, 1, tens, false); // Seconds
    lc.setDigit(0, 0, ones, false); // Seconds
    ledText(systemState, false);
  }
}

// Update Servo position in relation to heat time
void updateServo()
{
  // Map servo position to timers total in ms 420000ms = 7 minutes
  angle = map(timer.get_time(), 0, heatTime, servoMin, servoMax);
  if (angle != prevAngle)
  {
    servo.write(angle); // Write Servo
    prevAngle = angle; // Set angle of this iteration
  }
}

// Read state of buttons and switches
void checkButtons()
{
  if (!softparrotSwitch)
  {
    parrotMode = !digitalRead(parrotSwitch);
  }
  else
  {
    parrotMode = true;
  }
  if (!softelacSwitch)
  {
    elacMode = !digitalRead(elacSwitch);
  }
  else
  {
    elacMode = true;
  }
  greenButton.read();
  yellowButton.read();
  redButton.read();
  blueButton.read();
}

// Roll some nice text during idle state
void textRoll()
{
  currentMillis = millis();
  if (targetMillis < currentMillis)
  {
    lc.clearDisplay(0);
    P = L - 1;
    if (L >= 20)
    {
      L = 0;
      P = 0;
    }
    if (parrotMode)
    {
      // Parrot
      lc.setLed(0, P, 1, true);
      lc.setLed(0, P, 2, true); // P
      lc.setLed(0, P, 5, true);
      lc.setLed(0, P, 6, true);
      lc.setLed(0, P, 7, true);
      P--;
      lc.setLed(0, P, 1, true);
      lc.setLed(0, P, 2, true);
      lc.setLed(0, P, 3, true); // a
      lc.setLed(0, P, 5, true);
      lc.setLed(0, P, 6, true);
      lc.setLed(0, P, 7, true);
      P--;
      lc.setLed(0, P, 5, true); // r
      lc.setLed(0, P, 7, true);
      P--;
      lc.setLed(0, P, 5, true); // r
      lc.setLed(0, P, 7, true);
      P--;
      lc.setLed(0, P, 3, true); // o
      lc.setLed(0, P, 4, true);
      lc.setLed(0, P, 5, true);
      lc.setLed(0, P, 7, true);
      P--;
      lc.setLed(0, P, 4, true); // t
      lc.setLed(0, P, 5, true);
      lc.setLed(0, P, 6, true);
      lc.setLed(0, P, 7, true);
      P--;
    }
    else
    {
      // ACES
      lc.setChar(0, P, 'A', false);
      P--;
      lc.setLed(0, P, 1, true);
      lc.setLed(0, P, 4, true); // C
      lc.setLed(0, P, 5, true);
      lc.setLed(0, P, 6, true);
      P--;
      lc.setChar(0, P, 'E', false);
      P--;
      lc.setRow(0, P, 91); // S
      P--;
    }
    lc.setLed(0, P, 0, false);
    lc.setLed(0, P, 1, false);
    lc.setLed(0, P, 2, false);
    lc.setLed(0, P, 3, false); // SPACE
    lc.setLed(0, P, 4, false);
    lc.setLed(0, P, 5, false);
    lc.setLed(0, P, 6, false);
    lc.setLed(0, P, 7, false);
    P--;
    if (!elacMode)
    {
      // Std (Standard) with preceding and trailing space
      lc.setRow(0, P, 91); // S
      P--;
      lc.setLed(0, P, 4, true); // t
      lc.setLed(0, P, 5, true);
      lc.setLed(0, P, 6, true);
      lc.setLed(0, P, 7, true);
      P--;
      lc.setChar(0, P, 'd', false); // d
      P--;
    }
    else
      if (elacMode)
      {
        // ELAC
        lc.setChar(0, P, 'E', false);
        P--;
        lc.setChar(0, P, 'L', false);
        P--;
        lc.setChar(0, P, 'A', false);
        P--;
        lc.setChar(0, P, 'C', false);
        P--;
      }
    lc.setLed(0, P, 0, false);
    lc.setLed(0, P, 1, false);
    lc.setLed(0, P, 2, false);
    lc.setLed(0, P, 3, false); // SPACE
    lc.setLed(0, P, 4, false);
    lc.setLed(0, P, 5, false);
    lc.setLed(0, P, 6, false);
    lc.setLed(0, P, 7, false);
    P--;
    // Update Clock display
    targetMillis = currentMillis + rtPause;
    L++;
  }
}

// Controls ramping of the air raid siren
void airRaidSiren(byte pin)
{
  currentMillis = millis();
  if (currentMillis > updateSirenSpeed)
  {
    siren_speed = siren_speed + ramp_direction;
    if (siren_speed <= siren_max && siren_speed >= siren_min)
    {
      analogWrite(siren, siren_speed);
    }
    if (siren_speed >= (siren_max + 20))
    {
      // +10 to hold at 100 for 10x50ms
      ramp_direction = -1;
      hit_max = true;
      lc.shutdown(0, true);
      lc.shutdown(0, false);
    }
    if (siren_speed <= (siren_min + 15) && hit_max)
    {
      hit_max = false;
      ramp_direction = 1;
      if (systemState == "heat")
      {
        siren_pulses--;
      }
    }
    updateSirenSpeed = currentMillis + 40;
  }
}

void heatHornAndSiren()
{
  if (!pause)
  {
    if (siren_pulses > 0 && timer.get_time() > heatTime - 60000)
    {
      airRaidSiren(siren);
    }
    else
    {
      analogWrite(siren, 0);
    }
    if (timer.get_time() < heatTime && timer.get_time() > heatTime - 1000)
    {
      digitalWrite(horn, HIGH);
      if (!startSirenStart)
      {
        Serial.print("Start Horn On\n");
        startSirenStart = true;
      }
    }
    // Parrot rule: In Air within 30 seconds
    else
      if (parrotMode && timer.get_time() < heatTime - 30000 && timer.get_time() > heatTime - 30500)
      {
        digitalWrite(horn, HIGH);
        Serial.print("30 second mark\n");
      }
    else
    {
      digitalWrite(horn, LOW);
    }
  }
}

// System Idle state
void idle()
{
  checkButtons();
  checkSerialrx();
  if (servo.read() != servoMin)
  {
    // Make sure that servo is parked at 0
    angle = servoMin; // Set servo angle at parking (0 mins)
    servo.write(angle); // Send signal to servo
  }
  if (redButton.pressed())
  {
    digitalWrite(horn, HIGH);
    ledText("horn", true);
    return;
  }
  else
  {
    digitalWrite(horn, LOW);
  }
  if (redButton.released())
  {
    lc.shutdown(0, true);
    lc.shutdown(0, false);
  }
  if (blueButton.pressed())
  {
    airRaidSiren(siren);
    ledText("siren", true);
  }
  else
    if (blueButton.released())
    {
      siren_speed = siren_min;
      ramp_direction = 1;
    }
  else
  {
    digitalWrite(siren, LOW);
  }
  if (greenButton.released())
  {
    systemState = "heat";
    ledText(systemState, true);
    Serial.print("System State => heat\n");
    if (elacMode)
    {
      timer.set_time(elacTime);
      heatTime = elacTime;
    }
    else
    {
      timer.set_time(stdTime);
      heatTime = stdTime;
    }
    Serial.print("Timer set: ");
    Serial.print(timer.get_time());
    Serial.print("ms\n");
    siren_pulses = 2;
    timer.start();
    return;
  }
  if (yellowButton.released())
  {
    systemState = "prep";
    ledText(systemState, true);
    Serial.print("System State => prep\n");
    if (elacMode)
    {
      timer.set_time(elacTime);
      heatTime = elacTime;
    }
    else
    {
      timer.set_time(stdTime);
      heatTime = stdTime;
    }
    Serial.print("Timer set: ");
    Serial.print(timer.get_time());
    Serial.print("ms\n");
    timer.start();
    return;
  }
  if (!blueButton.pressed())
  {
    textRoll();
  }
  lc.setIntensity(0, 11);
}

// Heat state
void heat()
{
  checkButtons();
  checkSerialrx();
  // Check if timer has reached 0 and transition back to idle
  if (timer.get_time() == 0)
  {
    // Timer is done
    digitalWrite(horn, HIGH);
    Serial.print("Heat Over!\n");
    ledText("end", true);
    delay(2000);
    digitalWrite(horn, LOW);
    digitalWrite(siren, LOW);
    lc.clearDisplay(0);
    siren_pulses = 2;
    siren_speed = siren_min;
    systemState = "idle";
    startSirenStart = false;
    Serial.print("System State => idle\n");
    return;
  }
  if (redButton.released())
  {
    // If red button has ben released
    timer.stop(); // IF SOMETHING FISHY IS GOING ON DELETE THIS ROW
    analogWrite(siren, 0);
    digitalWrite(horn, LOW);
    lc.clearDisplay(0);
    ledText("stop", true);
    delay(1500);
    siren_pulses = 2;
    siren_speed = siren_min;
    systemState = "idle";
    startSirenStart = false;
    Serial.print("System State => idle\n");
    return;
  }
  if (blueButton.released() && !pause)
  {
    timer.pause();
    pause = true;
    digitalWrite(siren, LOW);
    digitalWrite(horn, LOW);
    ledText("pause", false);
    return;
  }
  if (blueButton.released() && pause)
  {
    timer.start();
    pause = false;
    digitalWrite(siren, LOW);
    digitalWrite(horn, LOW);
    ledText(systemState, false);
    return;
  }
  timer.tick();
  updateLedClock();
  updateServo();
  heatHornAndSiren(); // Horn and Siren sounds
}

// Horn and Siren sounds for preparation state
void prepHornAndSiren()
{
  // Start Horn
  if (timer.get_time() < heatTime && timer.get_time() > heatTime - 200)
  {
    digitalWrite(horn, HIGH);
  }
  else
    if (timer.get_time() < heatTime - 400 && timer.get_time() > heatTime - 600)
    {
      digitalWrite(horn, HIGH);
    }
  else
    if (timer.get_time() < heatTime - 800 && timer.get_time() > heatTime - 1000)
    {
      digitalWrite(horn, HIGH);
    }
  // 30 seconds left horn, sound the alarm 0.5s, pause 0.5s and sound 0.5s
  else
    if (timer.get_time() < thirtysecs && timer.get_time() > thirtysecs - 500)
    {
      digitalWrite(horn, HIGH);
    }
  else
    if (timer.get_time() < thirtysecs - 1000 && timer.get_time() > thirtysecs - 1500)
    {
      digitalWrite(horn, HIGH);
    }
  else
  {
    digitalWrite(horn, LOW);
  }
}

// Add or Subtract 10 seconds from timer
void addSubTime()
{
  // Subtract 10 seconds, check that 40 seconds remain
  if (timer.get_time() > 40000 && yellowButton.pushed())
  {
    timer.set_time(timer.get_time() - 10000);
  }
  // Add 10 seconds, check that 40 seconds has passed
  if (timer.get_time() < 380000 && greenButton.pushed())
  {
    timer.set_time(timer.get_time() + 10000);
  }
}

// Preparation State
void prep()
{
  checkButtons();
  checkSerialrx();
  // Check if timer has reached 0 and transition back to idle
  if (timer.get_time() == 0)
  {
    Serial.print("Preparation Over!\n");
    ledText("end", true);
    delay(100);
    digitalWrite(horn, LOW);
    digitalWrite(siren, LOW);
    lc.clearDisplay(0);
    systemState = "idle";
    Serial.print("System State => idle\n");
    return;
  }
  // addSubTime();
  // Abort
  if (redButton.released())
  {
    timer.stop(); // IF SOMETHING FISHY IS GOING ON DELETE THIS ROW
    digitalWrite(siren, LOW);
    digitalWrite(horn, LOW);
    lc.clearDisplay(0);
    ledText("stop", true);
    delay(1500);
    systemState = "idle";
    Serial.print("System State => idle\n");
    return;
  }
  // Pause
  if (blueButton.released() && !pause)
  {
    timer.pause();
    pause = true;
    digitalWrite(siren, LOW);
    digitalWrite(horn, LOW);
    ledText("pause", false);
    return;
  }
  // Unpause
  if (blueButton.released() && pause)
  {
    timer.start();
    pause = false;
    digitalWrite(siren, LOW);
    digitalWrite(horn, LOW);
    ledText(systemState, false);
    return;
  }
  prepHornAndSiren();
  timer.tick();
  updateLedClock();
  updateServo();
}

void loop()
{
  if (systemState == "idle")
  {
    idle();
  }
  while (systemState == "heat")
  {
    heat();
  }
  while (systemState == "prep")
  {
    prep();
  }
}
