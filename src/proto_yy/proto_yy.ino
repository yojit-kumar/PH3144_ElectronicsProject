#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// Set the FIRING_DELAY according to the power needed. For a half-cycle the time period is 1/100Hz = 1e4 microseconds. Choose accordingly !!
const int FIRING_DELAY = 3000;

// ADC1115 Pins
const int CURR_ADC_S = 0;
const int CURR_ADC_R = 1;
const int VOLT_ADC_S = 3;

// Control Pins
const int ZCD_PIN = 2; //D2 
const int TRIAC_PIN = 7; //D7

// Calibiration Constants
double VOLTAGE_CAL = 304.60; // Use voltage_calibirate code 
double CURRENT_CAL = 1.0; // Use current code

// ADS1115 Setup
Adafruit_ADS1115 ads;
// Voltage Multiplier
const float ADS_VOLTS_PER_BIT = 0.000125F; // GAIN_ONE
const int ADS1115_MID_POINT = 0; // Not Sure, this maybe 2.5V

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
  // Set Gain
  ads.setGain(GAIN_ONE);
  Serial.println("ADS Initialized");

  // Setup Arduino Pins
  pinMode(TRIAC_PIN, OUTPUT);
  pinMode(ZCD_PIN, INPUT);
  digitalWrite(TRIAC_PIN, LOW);

  // Attach Interrupt
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, RISING); //RISING for full-cycle CHANGE for half-cycle
}

void loop() {
  // FIRING LOGIC
  if (zcdFlag == true) {
    delayMicroseconds(FIRING_DELAY);
    digitalWrite(TRIAC_PIN, HIGH);
    delayMicroseconds(100); // arbitary wait-time
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

    for (int i = 0; i < sampleCount; i++) {

      voltageSample = ads.readADC_SingleEnded(VOLT_ADC_S);
      currentSample = ads.readADC_Differential_0_1();

      int voltageOffset = 20000; // must be calculated with GAIN in microseconds

      long filteredVoltage = voltageSample - voltageOffset;
      long filteredCurrent = currentSample;

      sumVoltageSq += filteredVoltage * filteredVoltage;
      sumCurrentSq += filteredCurrent * filteredCurrent;
    }

    // RMS Value
    double meanVoltageSq = (double)sumVoltageSq / sampleCount;
    double rmsVoltageRaw = sqrt(meanVoltageSq);
    double Vrms = rmsVoltageRaw * ADS_VOLTS_PER_BIT * VOLTAGE_CAL;

    double meanCurrentSq = (double)sumCurrentSq / sampleCount;
    double rmsCurrentRaw = sqrt(meanCurrentSq);
    double Irms = rmsCurrentRaw * CURRENT_CAL;

    // Power Calculation
    double power = Vrms * Irms;

    Serial.print("Vrms: "); Serial.print(Vrms); Serial.println(" V");
    Serial.print("Irms: "); Serial.print(Irms); Serial.println(" A");
    Serial.print("Power: "); Serial.print(power); Serial.println(" W");

    lastPrintTime = millis();
  }
}
