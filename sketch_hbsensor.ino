// Include necessary libraries for communication and for the MAX30105 sensor
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "MAX30105.h"
#include "heartRate.h"
 
// Create instances of the MAX30105 class to interact with the sensor
MAX30105 particleSensor1;
MAX30105 particleSensor2;

//Initialize LCDs
LiquidCrystal_I2C lcd1(0x27, 16, 2);
LiquidCrystal_I2C lcd2(0x26, 16, 2);

const byte RATE_SIZE = 4; // Increase this for more averaging. 4 is a good starting point.

byte rates1[RATE_SIZE]; // Array to store heart rate readings for averaging
byte rateSpot1 = 0; // Index for inserting the next heart rate reading into the array
long lastBeat1 = 0; // Timestamp of the last detected beat, used to calculate BPM
long delta1;
float beatsPerMinute1; // Calculated heart rate in beats per minute
int beatAvg1 = 0; // Average heart rate after processing multiple readings

byte rates2[RATE_SIZE]; // Array to store heart rate readings for averaging
byte rateSpot2 = 0; // Index for inserting the next heart rate reading into the array
long lastBeat2 = 0; // Timestamp of the last detected beat, used to calculate BPM
long delta2;
float beatsPerMinute2; // Calculated heart rate in beats per minute
int beatAvg2 = 0; // Average heart rate after processing multiple readings

int lastDisplayedBPM1 = 0, lastDisplayedBPM2 = 0; // Stores last displayed BPM to avoid redundant updates

void TCA9548A(uint8_t bus)
{
  Wire.beginTransmission(0x70); //it's address
  Wire.write(1 << bus);
  Wire.endTransmission();

  delay(10);
}

void initializeSensor(MAX30105 &particleSensor) 
{ 
  // Attempt to initialize the MAX30105 sensor. Check for a successful connection and report.
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) { // Start communication using fast I2C speed
    Serial.println("MAX30102 was not found. Please check wiring/power. ");
    while (1); // Infinite loop to halt further execution if sensor is not found
  }
  Serial.println("sensor ready");
 
  particleSensor.setup(); // Configure sensor with default settings for heart rate monitoring
  particleSensor.setPulseAmplitudeRed(0x1F); // Set the red LED pulse amplitude (intensity) to a low value as an indicator
  particleSensor.setPulseAmplitudeGreen(0); // Turn off the green LED as it's not used here
}

// Function to update LCD only if the BPM value has changed
void updateLCD(LiquidCrystal_I2C &lcd, int beatAvg, int &lastDisplayedBPM)
{
  if (beatAvg != lastDisplayedBPM) { // Update LCD only if BPM has changed
    lcd.clear();
    lcd.print("BPM: ");
    lcd.print(beatAvg);
    lastDisplayedBPM = beatAvg;
  }
}

// Function to get heart rate readings
void getHeartRate(MAX30105 &particleSensor, long &delta, long &lastBeat, 
          float &beatsPerMinute, byte rates[], byte &rateSpot, int &beatAvg, 
          LiquidCrystal_I2C &lcd, int &lastDisplayedBPM)
{
  long irValue = particleSensor.getIR(); // Read the infrared value from the sensor
  //Serial.println(irValue);
  if (irValue < 50000) return; // Skip if no finger detected
 
  if (checkForBeat(irValue) == true) { // Check if a heart beat is detected
    delta = millis() - lastBeat; // Calculate the time between the current and last beat
    lastBeat = millis(); // Update lastBeat to the current time
 
    beatsPerMinute = 60 / (delta / 1000.0); // Calculate BPM
 
    // Ensure BPM is within a reasonable range before updating the rates array
    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute; // Store this reading in the rates array
      rateSpot %= RATE_SIZE; // Wrap the rateSpot index to keep it within the bounds of the rates array
 
      beatAvg = 0; // Compute the average of stored heart rates to smooth out the BPM
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;

      Serial.print("Avg BPM: ");
      Serial.println(beatAvg);

      updateLCD(lcd, beatAvg, lastDisplayedBPM);
    }
  }
}

 
void setup() {
  Serial.begin(115200); // Start serial communication at 115200 baud rate

  //Start Wire Library I2C
  Wire.begin();
 
  //initialize sensors
  TCA9548A(1);
  initializeSensor(particleSensor1);

  delay(100);

  TCA9548A(2);
  initializeSensor(particleSensor2);

  delay(100);

  //start LCD and print message
  lcd1.init();
  lcd1.backlight();
  lcd1.setCursor(0, 0);
  lcd1.print("Place finger");

  lcd2.init();
  lcd2.backlight();
  lcd2.setCursor(0, 0);
  lcd2.print("Place finger");

  delay(100);
}
 
void loop() {

  TCA9548A(1);
  delay(5);
  for (int i = 0; i < 10; i++) {  
    getHeartRate(particleSensor1, delta1, lastBeat1, beatsPerMinute1, rates1, rateSpot1, beatAvg1, lcd1, lastDisplayedBPM1);
  }

  TCA9548A(2);
  delay(5);
  for (int i = 0; i < 10; i++) {  
    getHeartRate(particleSensor2, delta2, lastBeat2, beatsPerMinute2, rates2, rateSpot2, beatAvg2, lcd2, lastDisplayedBPM2);
  }

  delay(10);
}