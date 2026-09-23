# Heart Rate & SpO2 Sensing (Arduino Nano + MAX30102)

Reads heart rate (BPM) and blood oxygen saturation (SpO2) from a MAX30102 pulse oximeter sensor using an Arduino Nano, with results printed to the Serial Monitor and an LED that flashes on each detected heartbeat.

**Hardware**

Arduino Nano (or compatible)
MAX30102 pulse oximeter / heart-rate sensor module
LED + 220 Ω resistor (optional, for a visual heartbeat indicator)
Wiring
MAX30102 Pin	Arduino Nano Pin
VIN	5V
GND	GND
SDA	A4
SCL	A5

External LED: anode → Pin 5 → 220 Ω resistor → GND.

Dependencies

Install via the Arduino Library Manager, or from source:

SparkFun MAX3010x Pulse and Proximity Sensor Library — provides MAX30105.h and heartRate.h
Wire.h (built into the Arduino IDE)

**How It Works**

The MAX30102's FIFO buffer is continuously read for red and IR light samples.
A low-pass filter tracks the DC (average) component of each signal, and the AC (pulsatile) component is accumulated as an RMS sum.
Heart rate is detected using the SparkFun checkForBeat() algorithm on the IR signal; BPM is averaged over the last 4 beats (RATE_SIZE) to smooth the reading.
SpO2 is estimated every 100 samples (Num) from the ratio of the red and IR AC/DC components (the "R ratio"), converted to a percentage via a linear approximation, then smoothed with a low-pass filter.
If the IR signal drops below a threshold (FINGER_ON), the sensor assumes no finger is present and resets the readings.
Usage
Wire the sensor as described above.
Install the SparkFun MAX3010x library.
Upload the sketch to the Arduino Nano.
Open the Serial Monitor at 115200 baud.
Place a fingertip gently on the sensor. After a few seconds, BPM and SpO2 readings will print on each detected heartbeat, and the LED will flash in time with the pulse.
Key Configuration Constants
Constant	Default	Purpose
LED_BLINK_MS	80	LED flash duration per heartbeat (ms)
RATE_SIZE	4	Number of beats averaged for BPM
Num	100	Samples between SpO2 recalculations
FSpO2	0.7	Low-pass filter strength for SpO2
frate	0.95	Low-pass filter strength for DC signal
FINGER_ON	15000	IR threshold below which "no finger" is assumed
MINIMUM_SPO2	80.0	SpO2 value shown when no finger is detected
Notes
SpO2 values are clamped to a realistic 80–100% range and are an estimate, not a calibrated medical reading.
Sensor settings (LED brightness, sample rate, pulse width, ADC range) can be tuned in particleSensor.setup(...) inside setup().

