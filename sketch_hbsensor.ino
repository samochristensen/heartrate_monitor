#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor1;
MAX30105 particleSensor2;

LiquidCrystal_I2C lcd1(0x27, 16, 2);
LiquidCrystal_I2C lcd2(0x26, 16, 2);

const byte RATE_SIZE = 4;

byte rates1[RATE_SIZE];
byte rateSpot1 = 0;
long lastBeat1 = 0;
long delta1;
float beatsPerMinute1;
int beatAvg1 = 0;

byte rates2[RATE_SIZE];
byte rateSpot2 = 0;
long lastBeat2 = 0;
long delta2;
float beatsPerMinute2;
int beatAvg2 = 0;

int lastDisplayedBPM1 = 0, lastDisplayedBPM2 = 0;

void TCA9548A(uint8_t bus) {
  Wire.beginTransmission(0x70);
  Wire.write(1 << bus);
  Wire.endTransmission();
  delay(10);
}

void initializeSensor(MAX30105 &particleSensor) { 
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 was not found. Please check wiring/power.");
    while (1);
  }
  Serial.println("sensor ready");

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeGreen(0);
}

void updateLCD(LiquidCrystal_I2C &lcd, int beatAvg, int &lastDisplayedBPM) {
  if (beatAvg != lastDisplayedBPM) {
    lcd.clear();
    lcd.print("BPM: ");
    lcd.print(beatAvg);
    lastDisplayedBPM = beatAvg;
  }
}

void getHeartRate(MAX30105 &particleSensor, long &delta, long &lastBeat, 
          float &beatsPerMinute, byte rates[], byte &rateSpot, int &beatAvg, 
          LiquidCrystal_I2C &lcd, int &lastDisplayedBPM) {
  
  long irValue = particleSensor.getIR();
  if (irValue < 50000) return;

  if (checkForBeat(irValue)) {
    delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;

      Serial.print("Avg BPM: ");
      Serial.println(beatAvg);

      updateLCD(lcd, beatAvg, lastDisplayedBPM);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  TCA9548A(1);
  initializeSensor(particleSensor1);
  delay(100);

  TCA9548A(2);
  initializeSensor(particleSensor2);
  delay(100);

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