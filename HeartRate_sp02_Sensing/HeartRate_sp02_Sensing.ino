/*
  Nano_MAX30102_HR_LED_Plotter_Fixed.ino

  Arduino Nano + MAX30102
  Using SparkFun MAX3010X library: https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library

  Wiring:
    VIN  -> 5V
    GND  -> GND
    SDA  -> A4
    SCL  -> A5
    LED  -> Pin 5 (Anode) -> 220 ohm resistor -> GND
*/

#include <Wire.h>
#include "MAX30105.h"   // SparkFun MAX3010X library
#include "heartRate.h"  // SparkFun beat-detection algorithm

MAX30105 particleSensor;

#define MAX30105 

#define LED_PIN 5             // LED on Digital Pin 5
#define LED_BLINK_MS 80       // Flash duration per heartbeat (ms)

bool ledIsOn = false;
unsigned long ledOnAt = 0;

// HR paramaters
long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg = 0;
#define RATE_SIZE 4           // Moving Average Filter for smoothing
byte rateSpot = 0;
long rates[RATE_SIZE];

// sp02 paramaters
double avered = 0; 
double aveir = 0;
double sumirrms = 0;
double sumredrms = 0;
int i = 0;
int Num = 100;                // Calculate SpO2 every 100 samples

double ESpO2 = 95.0;          // Initial estimated SpO2
double FSpO2 = 0.7;           // Low-pass filter factor for SpO2
double frate = 0.95;          // Low-pass filter factor for DC component

#define TIMETOBOOT 3000
#define SAMPLING 5

#define FINGER_ON 15000 
#define MINIMUM_SPO2 80.0 

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing MAX30102...");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD))
  {
    Serial.println("MAX30102 not found. Check wiring: VIN=5V, GND=GND, SDA=A4 SCL=A5, LED=D5.");
    while (1); // forever until fixed
  }
  Serial.println("Sensor found, configuration starting...");

  byte ledBrightness = 0x3F; 
  byte sampleAverage = 1;
  byte ledMode = 2;  // 2 = Red + IR
  int sampleRate = 200;    
  int pulseWidth = 411;      
  int adcRange = 16384;    

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
}

void loop()
{
  uint32_t ir, red;
  double fred, fir;
  double SpO2 = 0;

  particleSensor.check(); // Read data from sensor FIFO buffer

  while (particleSensor.available())
  {
    #ifdef MAX30105
        red = particleSensor.getFIFORed(); 
        ir  = particleSensor.getFIFOIR();  
    #else
        red = particleSensor.getFIFOIR();  
        ir  = particleSensor.getFIFORed(); 
    #endif

    i++;
    fred = (double)red;
    fir = (double)ir;

    // Filter DC offsets
    avered = avered * frate + fred * (1.0 - frate);
    aveir  = aveir  * frate + fir  * (1.0 - frate);
    
    // RMS AC component calculations
    sumredrms += (fred - avered) * (fred - avered);
    sumirrms  += (fir  - aveir)  * (fir  - aveir);

    if (checkForBeat((long)ir))
    {
      long delta = millis() - lastBeat;
      lastBeat = millis();

      beatsPerMinute = 60.0 / (delta / 1000.0);

      if (beatsPerMinute < 255.0 && beatsPerMinute > 20.0)
      {
        rates[rateSpot++] = (long)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }

      // Flash external LED
      digitalWrite(LED_PIN, HIGH);
      ledIsOn = true;
      ledOnAt = millis();

      // Output to Serial Monitor on each detected heartbeat
      if (millis() > TIMETOBOOT && ir >= FINGER_ON)
      {
        Serial.print("BPM: ");
        Serial.print(beatAvg);
        Serial.print(" | SpO2: ");
        Serial.print(ESpO2, 1);
        Serial.println("%");
      }
    }

    // Turn off LED after specified blink duration
    if (ledIsOn && (millis() - ledOnAt > LED_BLINK_MS))
    {
      digitalWrite(LED_PIN, LOW);
      ledIsOn = false;
    }

    if ((i % SAMPLING) == 0)
    {
      if (millis() > TIMETOBOOT)
      {
        // Finger detection check
        if (ir < FINGER_ON)
        {
          ESpO2 = MINIMUM_SPO2;
          beatAvg = 0; // Reset HR display when no finger is detected
        }
      }
    }

    if ((i % Num) == 0)
    {
      if (avered > 0 && aveir > 0 && sumirrms > 0)
      {
        double R = (sqrt(sumredrms) / avered) / (sqrt(sumirrms) / aveir);
        SpO2 = -23.3 * (R - 0.4) + 100.0;
        
        // Clamp calculated SpO2 values to realistic range
        if (SpO2 > 100.0) SpO2 = 100.0;
        if (SpO2 < 80.0)  SpO2 = 80.0;

        ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2; // Apply low-pass filter
      }

      // Reset accumulation variables
      sumredrms = 0.0; 
      sumirrms = 0.0; 
      i = 0;
    }

    particleSensor.nextSample(); // Advance to next sample in FIFO
  }
}