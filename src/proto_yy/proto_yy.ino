#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// Set the FIRING_DELAY according to the power needed.
// For a 50Hz half-cycle the time period is 1/100Hz = 10,000 microseconds. [cite: 2]
const int FIRING_DELAY = 1000;

// ADC1115 Pins
const int CURR_ADC_S = 0; // Current Signal (SCT-013) on AIN0
const int CURR_ADC_R = 1; // Current 2.5 V Reference
const int VOLT_ADC_S = 3; // Voltage Signal (ZMPT101B) on AIN3
// Note: CURR_ADC_R = 1 is no longer used in this logic

// Control Pins
const int ZCD_PIN = 2; //D2 [cite: 4]
const int TRIAC_PIN = 7; //D7 [cite: 4]

// Calibiration Constants
double VOLTAGE_CAL = 304.60; // Use voltage_calibirate code [cite: 5]
double CURRENT_CAL = 1.0; // Use current code [cite: 5]

// ADS1115 Setup
Adafruit_ADS1115 ads;
// Voltage Multiplier
const float ADS_VOLTS_PER_BIT = 0.000125F; // For GAIN_ONE [cite: 6]

// Global Variables
volatile bool zcdFlag = false;
unsigned long lastPrintTime = 0;
const int sampleCount = 500;

void onZeroCross() {
  zcdFlag = true;
}

void setup() {
  Serial.begin(9600);
  Serial.println("AC POWER CONTROLLER");
  Serial.print("Firing Delay: "); Serial.print(FIRING_DELAY); Serial.println(" microseconds");

  // Initialize ADS1115
  if (!ads.begin()) {
    Serial.println("Failed to Initialize ADS");
    while (1);
  }
  // Set Gain - GAIN_ONE gives +/- 4.096V range, good for 2.5V biased signals
  ads.setGain(GAIN_ONE);
  Serial.println("ADS Initialized");

  // Setup Arduino Pins
  pinMode(TRIAC_PIN, OUTPUT);
  pinMode(ZCD_PIN, INPUT);
  digitalWrite(TRIAC_PIN, LOW); // Start with Triac OFF 

  // Attach Interrupt
  // Use CHANGE to catch both rising and falling edges for a 100Hz cycle
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, CHANGE);
}

void loop() {
  // FIRING LOGIC
  if (zcdFlag == true) {
    delayMicroseconds(FIRING_DELAY);
    digitalWrite(TRIAC_PIN, HIGH); // 
    delayMicroseconds(20); // Short pulse to latch Triac
    digitalWrite(TRIAC_PIN, LOW);
    zcdFlag = false; // Reset the flag.
  }

  // Measurement
  if (millis() - lastPrintTime >= 1000) { // Prints every second
    
    // variable initializing
    long sumVoltageSq = 0;
    long sumCurrentSq = 0;
    int16_t voltageSample;
    int16_t currentSample;
    int16_t currentReference;

    // --- MUST CALIBRATE THESE OFFSETS ---
    // Measure the signal on A0 and A3 with 230V AC OFF.
    // The stable number you see is your offset.
    int voltageOffset = 20000; // Placeholder, MUST CALIBRATE 
    int currentOffset = 20000; // Placeholder, MUST CALIBRATE

    for (int i = 0; i < sampleCount; i++) {

      // Read both channels as single-ended
      voltageSample = ads.readADC_SingleEnded(VOLT_ADC_S);
      currentSample = ads.readADC_SingleEnded(CURR_ADC_S);
      currentReference = ads.readADC_SingleEnded(CURR_ADC_R); 

      // Remove the 2.5V DC offset in software
      long filteredVoltage = voltageSample - voltageOffset;
      long filteredCurrent = currentSample - currentReference;

      sumVoltageSq += filteredVoltage * filteredVoltage;
      sumCurrentSq += filteredCurrent * filteredCurrent;
    }

    // RMS Value
    double meanVoltageSq = (double)sumVoltageSq / sampleCount;
    double rmsVoltageRaw = sqrt(meanVoltageSq);
    double Vrms = rmsVoltageRaw * ADS_VOLTS_PER_BIT * VOLTAGE_CAL;

    double meanCurrentSq = (double)sumCurrentSq / sampleCount;
    double rmsCurrentRaw = sqrt(meanCurrentSq);
    double Irms = rmsCurrentRaw * ADS_VOLTS_PER_BIT * CURRENT_CAL; // <--FIXED: Must also multiply by ADS_VOLTS_PER_BIT

    // Power Calculation (Apparent Power)
    double power = Vrms * Irms;

    Serial.print("Vrms: ");
    Serial.print(Vrms, 2); Serial.println(" V"); // [cite: 20]
    Serial.print("Irms: "); Serial.print(Irms, 3); Serial.println(" A");
    Serial.print("Power: "); Serial.print(power, 2); Serial.println(" VA");

    lastPrintTime = millis();
  }
}
