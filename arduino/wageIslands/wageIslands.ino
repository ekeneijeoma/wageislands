

// FOR USE ON ARDUINO UNO R3

/* Connections
 *  D7 to pushbutton with a pulldown resistor, this controls the movement of the motors
 *  A0 to potentiometer, this constrains the maximum travel length
 *  A4 to SDA, A5 to SDL of an I2C LCD
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h> // from https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home

// BEGIN SETTINGS
#define LAYERS 19
float wages[LAYERS] = { 8.25, 12.00, 15.75, 19.75, 23.50, 27.25, 31.75, 35.00, 38.75, 42.50, 46.50, 50.25, 54.00, 58.00, 61.75, 65.50, 69.25, 73.25 , 77 };
String areas[LAYERS] = { "5.36", "11.10", "17.88", "29.38", "46.00", "64.80", "92.09", "126.37", "145.59", "171.57", "191.16", "217.93", "226.10", "238.64", "254.13", "259.22", "264.43", "271.85", "285.10" };
int microstep = 16;     // Choices are full-step, half-step, quarter-step, sixteenth-step
int spinSpeed = 4;      // Desired speed in mm/s
bool debug = true;
bool stepping = true;
int stepDelay = 1000; // milliseconds to stop each time
// END SETTINGS

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7); // SDA to A4, SDL to A5
#define BACKLIGHT_PIN     3

// PINS
const int sensorPin = A0; // select the input pin for the potentiometer
const int buttonPin = 7;

const int MS1pin = 12;    // 00 is 1, 10 is 1/2
const int MS2pin = 11;    // 01 is 1/4, 11 is 1/16
const int ENpin = 8;      // 0 is on, 1 is off
const int STEPpin = 9;    // Low-to-high is step
const int DIRpin = 10;    // 0 is CCW, 1 is CW

// TIMING
int pulseWidthMicros = 20; // microseconds
int millisbetweenSteps = 25; // milliseconds

const float usteps_per_mm1 = 96.2752018f; // Sixteenth stepping
const float usteps_per_mm2 = 95.9125834f; // Sixteenth stepping - measured
const unsigned int timer_speed = (unsigned int) ((1.f / ((float) spinSpeed * (float) microstep * 0.0625f * usteps_per_mm2)) * 3000000.f); //24V, full-step

// STATE
int buttonState = 0;
int dir = 0;

int layer = 0;
//int pop = 0;
int area = 0;
int section = 0;
int previous_layer = 0;
float percent = 0.0;

int currentStep = 0;
int targetStep = 0;
const int maxStep = 299; //total number of steps travel


// automated version variables
boolean autoState = false;
int autoPin = 3;
int autoVal = 1;
int upState = true;         
// print out dir and confirm which is which

void setup()
{
  lcd.begin(20, 4);
  pinMode(buttonPin, INPUT);

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

  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  TIMSK1 = 0;

  TCCR1A |= (1 << COM1A0);  // Enable OCR1A pin
  TCCR1B |= (1 << WGM12);   // Mode 4, CTC on OCR1A
  TCCR1B |= (1 << CS10);    // No prescaler
  OCR1A = timer_speed; //24V, full-step
  interrupts();

  digitalWrite(ENpin, LOW);

  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);

  Serial.begin(9600);

// automatic state setup()
pinMode(autoPin, INPUT_PULLUP);
}

void displayData() {
  lcd.home();

  if (layer >= 19) {
  }
  else {
    //    lcd.setCursor(9, 0);
    //    lcd.print(sec[section]);
    //    lcd.print(" /19");

    lcd.setCursor(0, 0);
    lcd.print((layer + 1));
    lcd.print(" ");

    lcd.setCursor(0, 1);
    lcd.print("HOURLY WAGE: $");
    lcd.print(wages[layer]);
    lcd.print(" ");
    //    lcd.print(" ");

    lcd.setCursor(0, 2);
    lcd.print("AREA: ");
    lcd.print(areas[layer]);
    lcd.print(" SQ MI ");
  }
  //    lcd.print("     ");

  //    if (debug) {
  //
  //      lcd.setCursor(2, 3);
  //      //    lcd.print("POPULATION: ");
  //      //    lcd.print(popden[pop]);
  //      lcd.print("     ");
  //      lcd.print("     ");
  //      lcd.print("     ");
  //
    lcd.setCursor(0, 3);
    lcd.print("             ");
  //    }
  //  }

}

void loop() {

 autoVal = digitalRead(autoPin);
  if(autoVal>0){
      autoState = false;
}else{
      autoState = true;
}

  percent = analogRead(sensorPin) / 1024.;
  targetStep = floor(percent * maxStep);
  layer = LAYERS * currentStep / targetStep;
  //  if(currentStep % targetStep == 0 && layer < LAYERS){
  //    layer++;
  //}

  Serial.print("layer: ");
  Serial.print(layer);
  Serial.print("   ");
  Serial.print("currentStep: ");
  Serial.print(currentStep);
  Serial.println("");
  Serial.print("TargetStep: ");
  Serial.print(targetStep);
  Serial.println("");
  //  pop = POPS*currentStep/targetStep;
  //  area = AREAS * currentStep / targetStep;
  //  section = SECTIONS * currentStep / targetStep;

  if (stepping && layer != previous_layer) {
    noInterrupts();
    TCCR1A &= ~(1 << COM1A0);  // Enable OCR1A pin
    interrupts();
    delay(stepDelay);
  } else {

      
if(autoState){
	if(currentStep <= 0 || currentStep >= targetStep){
	    upState = !upState;
	}
	if(upState){
	buttonState = 1;
              }else{
	buttonState = 0;
              }
	
}else{

    buttonState = digitalRead(buttonPin);
}
    dir = (2 * buttonState) - 1;
    displayData();
    if ( (dir > 0 && currentStep < targetStep) ||
         (dir < 0 && currentStep > 0 ) ) {
      digitalWrite(DIRpin, !buttonState);

      noInterrupts();
      TCCR1A |= (1 << COM1A0);  // Enable OCR1A pin
      interrupts();

      currentStep = constrain(currentStep + dir, 0, targetStep);
      if (debug) {
        lcd.setCursor(0, 3);
        lcd.print("      ");
        lcd.print("  ");
      }
    } else {
      noInterrupts();
      TCCR1A &= ~(1 << COM1A0);  // Disable OCR1A pin
      interrupts();
    }
  }
  previous_layer = layer;
}



