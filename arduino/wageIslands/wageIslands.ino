// FOR USE ON ARDUINO UNO R3

/* Connections
 *  D7 to pushbutton with a pulldown resistor, this controls the movement of the motors
 *  A4 to SDA, A5 to SDL of an I2C LCD
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h> // from https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home

// LEVELS
#define levelCount 19 // Total number of levelCount that will be moving between
float wages[levelCount] = { 8.25, 12.00, 15.75, 19.75, 23.50, 27.25, 31.75, 35.00, 38.75, 42.50, 46.50, 50.25, 54.00, 58.00, 61.75, 65.50, 69.25, 73.25 , 77 }; //Display values for hourly income
String areas[levelCount] = { "5.36", "11.10", "17.88", "29.38", "46.00", "64.80", "92.09", "126.37", "145.59", "171.57", "191.16", "217.93", "226.10", "238.64", "254.13", "259.22", "264.43", "271.85", "285.10" }; // Display values for area

int level = 0;
int previous_level = 0;

// LCD
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7); // SDA to A4, SDL to A5
#define BACKLIGHT_PIN     3                       // LCD Backlight pin (on LCD, do no change)

// PINS
const int buttonPin = 7;  // Button Pin - Connect between this and +5V, Pull down resistor between this and GND.

const int MS1pin = 12;    // 00 is 1, 10 is 1/2
const int MS2pin = 11;    // 01 is 1/4, 11 is 1/16
const int ENpin = 8;      // 0 is on, 1 is off
const int STEPpin = 9;    // Low-to-high is step
const int DIRpin = 10;    // 0 is CCW, 1 is CW

// STEPPING
bool stepping = true;
int stepDelay = 1000; // milliseconds to stop each time

int microstep = 16;     // Choices are full-step, half-step, quarter-step, sixteenth-step
int spinSpeed = 27;//4;      // Desired speed in mm/s

int currentStep = 0;
int targetStep = 0;
const int maxStep = 755;//753; //total number of steps travel

// TIMING
int pulseWidthMicros = 20; // microseconds
int millisbetweenSteps = 25; // milliseconds

const float usteps_per_mm1 = 96.2752018f; // Sixteenth stepping
const float usteps_per_mm2 = 95.9125834f; // Sixteenth stepping - measured
const unsigned int timer_speed = (unsigned int) ((1.f / ((float) spinSpeed * (float) microstep * 0.0625f * usteps_per_mm2)) * 3000000.f); //24V, full-step

// STATES
int buttonState = 0;
int dir = 0;

void setup()
{
  lcd.begin(20, 4);                  //Setup LCD dims
  pinMode(buttonPin, INPUT);         //Change button pin to an input

  pinMode(ENpin, OUTPUT);
  digitalWrite(ENpin, HIGH);         // Turn driver off

  pinMode(MS1pin, OUTPUT);
  pinMode(MS2pin, OUTPUT);

  if (microstep == 2)
  {
    digitalWrite(MS1pin, HIGH);
    digitalWrite(MS2pin, LOW);
  }
  else if (microstep == 4)
  {
    digitalWrite(MS1pin, LOW);
    digitalWrite(MS2pin, HIGH);
  }
  else if (microstep == 16)
  {
    digitalWrite(MS1pin, HIGH);
    digitalWrite(MS2pin, HIGH);
  }
  else
  {
    digitalWrite(MS1pin, LOW);
    digitalWrite(MS2pin, LOW);
  }

  pinMode(STEPpin, OUTPUT);
  digitalWrite(STEPpin, LOW);

  pinMode(DIRpin, OUTPUT);
  digitalWrite(DIRpin, HIGH);

  noInterrupts(); // Turn off interrupt when registers are being set
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  TIMSK1 = 0;

  TCCR1A |= (1 << COM1A0);  // Enable OCR1A pin
  TCCR1B |= (1 << WGM12);   // Mode 4, CTC on OCR1A
  TCCR1B |= (1 << CS10);    // No prescaler
  OCR1A = timer_speed; //24V, full-step
  interrupts();  // Resume interrupt

  digitalWrite(ENpin, LOW);

  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);                        // Turn Backlight on

  Serial.begin(9600);                            // Start Serial
}

//Display data to LCD
void displayData() {
  lcd.home();

  if (level >= 19) {  // Stops display but going over the correct level and displaying garbage because it's written out of order
  }
  else {

    lcd.setCursor(0, 0);
    lcd.print((level + 1));
    lcd.print(" ");  // formatting and whatnot to avoid extra characters

    lcd.setCursor(0, 1);
    lcd.print("HOURLY WAGE: $");
    lcd.print(wages[level]);
    lcd.print(" ");  // formatting and whatnot to avoid extra characters
    //    lcd.print(" ");

    lcd.setCursor(0, 2);
    lcd.print("AREA: ");
    lcd.print(areas[level]);
    lcd.print(" SQ MI ");
  }

  lcd.setCursor(0, 3);
  lcd.print("             "); //formatting and whatnot to avoid extra characters - this probably isn't needed, nothingis written to this line
}


void loop() {
  targetStep = floor(maxStep);    // Get total steps per level
  level = levelCount * currentStep / targetStep;// Math for level height

  //Printing to serial for debug purpuses
  Serial.print("level: ");
  Serial.print(level);
  Serial.print("   ");
  Serial.print("currentStep: ");
  Serial.print(currentStep);
  Serial.println("");
  Serial.print("TargetStep: ");
  Serial.print(targetStep);
  Serial.println("");

  if (stepping && level != previous_level) {
    noInterrupts();
    TCCR1A &= ~(1 << COM1A0);  // Enable OCR1A pin
    interrupts();
    delay(stepDelay);
  } else {
    buttonState = digitalRead(buttonPin); //Check state of button

    dir = (2 * buttonState) - 1;
    displayData();

    if ( (dir > 0 && currentStep < targetStep) ||
         (dir < 0 && currentStep > 0 ) ) {
      digitalWrite(DIRpin, buttonState);

      noInterrupts();
      TCCR1A |= (1 << COM1A0);  // Enable OCR1A pin
      interrupts();

      currentStep = constrain(currentStep + dir, 0, targetStep);

    } else {
      noInterrupts();
      TCCR1A &= ~(1 << COM1A0);  // Disable OCR1A pin
      interrupts();
    }
  }

  previous_level = level;
}



