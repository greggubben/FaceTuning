/*
 * Tuning operations for the Face
 *
 * The servo under test must be attached to PIN 11
 *
 * List of tests:
 *     Set the Servo to 90 degrees dddd
 */
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LCD_Helper.h>

/******************
 * Set up the Servos
 ******************/
 
// Test selection pins
boolean testRange = false;

// Pot input for the Range Test
const int POT_PIN = 1;  // Use a pot to see the range of the servo

// Range of Servos available for testing
const int MIN_SERVO_PIN = 2;
const int MAX_SERVO_PIN = 10;

// Last position of the servo.  Used to minimize communication
int last_servoVal;
int min_servoVal[MAX_SERVO_PIN+1];
int max_servoVal[MAX_SERVO_PIN+1];

// Pin of the servo being tested
Servo testServo[14];
int servoPin = 0;

// LED to show we are running a test
const int RUNNING_TEST_PIN = 13;

/******************
 * Set up the LCD
 ******************/
 
// set the LCD address to 0x27 for a 20 chars and 4 line display
LiquidCrystal_I2C lcdisplay(0x27,20,4);

// Display settings
const uint8_t DISPLAY_LENGTH = 9;
display_t display_positions[DISPLAY_LENGTH] = {
    { 0, 0,"BL:", 3, 0, 3},
    {10, 0,"BR:",13, 0, 3},
    { 0, 1,"EL:", 3, 1, 3},
    { 6, 1,  ":", 7, 1, 3},
    {10, 1,"ER:",13, 1, 3},
    {16, 1,  ":",17, 1, 3},
    { 0, 2, "M:", 2, 2, 3},
    { 0, 3,"NU:", 3, 3, 3},
    {10, 3,"NS:",13, 3, 3},
};

/******************
 * Setup()
 ******************/
 
void setup() {
  // For writing the results
  Serial.begin(9600);
  
  // Pin used to show Test is being performed
  pinMode(RUNNING_TEST_PIN, OUTPUT);

  quitTesting();
  detachAllServos();
  clearAllValues();
  servoPin = 0;

  // initialize the lcd 
  lcdisplay.init();

  // Print the labels
  displayLabels(&lcdisplay, DISPLAY_LENGTH, display_positions);

  printHelp();
}

void loop() {
  // Get what to do
  if (Serial.available() > 0) {
    int inByte = Serial.read();
    int pin = 0;
    
    switch (inByte) {
      case 'h':
      case '?':                // Print the help
        printHelp();
        break;

      case 'R':                // Reset everything
        Serial.println("Resetting... (calling setup)");
        setup();
        break;
        
    case 's':                  // Select the servo pin to test
      pin = Serial.parseInt();
      if (pin < MIN_SERVO_PIN || pin > MAX_SERVO_PIN) {
        Serial.println("Invalid Pin");
        Serial.println();
      }
      else {
        quitTesting();
        printValues();
        Serial.println();
        setPin(pin);
      }
        break;

    case 't':                  // Test Range
      if (servoPin < MIN_SERVO_PIN || servoPin > MAX_SERVO_PIN) {
        Serial.println("Invalid Pin");
      }
      else {
        startRangeTest();
      }
      break;

    case 'n':                  // Next Servo to Test Range
      if (servoPin < MIN_SERVO_PIN) {
        // Have not started yet - init
        servoPin = MIN_SERVO_PIN;
      }
      else {
        // proceed to next servo
        quitTesting();
        servoPin++;
        Serial.println();
      }

      if (servoPin > MAX_SERVO_PIN) {
        // Reached the end - done
        printAllValues();
      }
      else {
        // start testing next servo
        setPin(servoPin);
        startRangeTest();
      }
      break;

    case 'q':                  // Quit testing
      quitTesting();
      printValues();
      break;

    case '9':                  // Set the servo back to 90
      quitTesting();
      setServoTo90();
      break;

    case 'I':                  // Set All servos back to 90
      quitTesting();
      setAllServosTo90();
      servoPin = 0;
      break;

    case 'd':                  // Detach servo
      quitTesting();
      detachServo();
      servoPin = 0;
      Serial.println();
      break;

    case 'D':                  // Detach All servos
      quitTesting();
      detachAllServos();
      servoPin = 0;
      Serial.println();
      break;
      
    case 'c':                  // Clear the key servo values
      clearValues();
      break;
      
    case 'C':                  // Clear All servos key values
      clearAllValues();
      break;
      
    case 'p':                  // Print the key servo values
      printValues();
      break;
      
    case 'P':                  // Print the key servo values
      quitTesting();         // Do it for all servos
      Serial.println();
      printAllValues();
      Serial.println();
      break;
      
    default:
      break;
    }
  }

  // Perform the tests
  if (testRange) {
    int potVal = analogRead(POT_PIN);  // reads the value of the potentiometer
    // Pot range is typically between 0 and 1023.
    // My pot range is only 114 to 900
    // Servo range is typically between 0 and 180.
    // The widest range for the servos is 30 to 150 for this installation
    int servoVal = map(potVal, 114, 900, 30, 150);    // scale pot to servo
    
    if (servoVal != last_servoVal) {
      // Print the Servo position back to the serial monitor
      //Serial.print("Servo Pos: ");
      //Serial.print(servoVal);
      //Serial.println();
      // Display a single value as left justified
      displayValueLeft(&lcdisplay, display_positions, servoPin-MIN_SERVO_PIN, servoVal);

      last_servoVal = servoVal;
      testServo[servoPin].write(servoVal);
      if (min_servoVal[servoPin] > servoVal) {
        min_servoVal[servoPin] = servoVal;
      }
      if (max_servoVal[servoPin] < servoVal) {
        max_servoVal[servoPin] = servoVal;
      }
      delay(15);                             // waits for the servo to get there
    }
  }
}

///////////////////////
//
// Supporting Functions
//
///////////////////////


// Print the list of commands available
void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  h  = Help (this menu)");
  Serial.println("  ?  = Help (this menu)");
  Serial.println();
  Serial.println("  R  = Reset everything");
  Serial.println();
  Serial.println("  s# = Servo where # is the number of the pin (2-13) the servo is attached");
  Serial.println("  t  = Test range");
  Serial.println("  n  = Next servo to test range");
  Serial.println("  q  = Quit testing");
  Serial.println();
  Serial.println("  9  = set the servo to 90 degrees");
  Serial.println("  I  = initialize All servos to 90 degrees");
  Serial.println();
  Serial.println("  d  = Detach servo");
  Serial.println("  D  = Detach All servos to detached state");
  Serial.println();
  Serial.println("  c  = Clear the key servo testing values");
  Serial.println("  C  = Clear All the key servo testing values");
  Serial.println("  p  = Print the key servo testing values");
  Serial.println("  P  = print All the key servo testing values");
  Serial.println();
}

// Set a new Servo Pin to test with
void setPin(int pin) {
  servoPin = pin;
  if (!testServo[servoPin].attached()) {
    testServo[servoPin].attach(servoPin);
  }
  
  Serial.print("Servo Pin = ");
  Serial.print(servoPin);
  Serial.println();
  setServoTo90();
}

// Start the range testing
void startRangeTest() {
  if (testRange) {
    Serial.println("Already Testing");
  }
  else {
    testRange = true;
    digitalWrite(RUNNING_TEST_PIN,HIGH);
    Serial.println("Testing Started");
  }
}

// Quit running the Range testing
void quitTesting() {
  if (testRange) {
    testRange = false;
    digitalWrite(RUNNING_TEST_PIN,LOW);
    Serial.println("Testing Stopped");
  }
}

// Initialize all Servos to 90 degrees
void setAllServosTo90 () {
  Serial.println("Initializing All Servos to 90 degrees");
  for (servoPin = MIN_SERVO_PIN; servoPin<=MAX_SERVO_PIN; servoPin++) {
    setPin(servoPin);
  }
  Serial.println();
}

// Set the Servo to 90 degrees
void setServoTo90 () {
  testServo[servoPin].write(90);
  Serial.print("Servo on Pin = ");
  Serial.print(servoPin);
  Serial.print(" set to 90");
  Serial.println();
  delay(15);
}

// Detach Servos
void detachServo () {
  if (testServo[servoPin].attached()) {
    testServo[servoPin].detach();
    Serial.print("Detached Servo on Pin = ");
    Serial.print(servoPin);
    Serial.println();
  }
}

// Reset all Servos to detached
void detachAllServos () {
  Serial.println("Detaching All Servos");
  for (servoPin = MIN_SERVO_PIN; servoPin<=MAX_SERVO_PIN; servoPin++) {
    detachServo();
  }
}

// Clear the servo key values
void clearValues() {
  min_servoVal[servoPin] = 255;
  max_servoVal[servoPin] = 0;
  Serial.print("Values Cleared for Servo on Pin = ");
  Serial.print(servoPin);
  Serial.println();
}

// Clear all Servo key values
void clearAllValues () {
  Serial.println("Clearing All Servos");
  for (servoPin = MIN_SERVO_PIN; servoPin<=MAX_SERVO_PIN; servoPin++) {
    clearValues();
  }
}

// Print the servo key values
void printValues() {
  Serial.print("Servo Pin = ");
  Serial.print(servoPin);
//  Serial.println();
  Serial.print("; Min Pos = ");
  Serial.print(min_servoVal[servoPin]);
//  Serial.println();
  Serial.print("; Max Pos = ");
  Serial.print(max_servoVal[servoPin]);
  Serial.println();
}

// Print the all the servo key values
void printAllValues() {
  int remember_servoPin = servoPin;
  Serial.println("Printing All Servo Values");
  for (servoPin = MIN_SERVO_PIN; servoPin<=MAX_SERVO_PIN; servoPin++) {
    printValues();
  }
  servoPin = remember_servoPin;
}
