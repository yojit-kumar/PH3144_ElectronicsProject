#include <Wire.h>
#include <Adafruit_ADS1X15.h> // Include ADS1115 library

/*
 * AC POWER CONTROLLER - FIXED DIMMER & MONITOR TEST
 * DANGER: Controls 230V AC. Use extreme caution.
 *
 * Reads Voltage (ZMPT101B) and Current (SCT-013-030) via ADS1115.
 * Uses PC814 ZCD for timing.
 * Fires Triac after a fixed delay.
 * Prints Vrms, Irms, and Power to Serial Monitor.
 */

// --- USER SETTING: Firing Delay ---
// Set the firing delay in microseconds (µs). 50Hz = 10,000µs half-cycle.
// Longer delay = dimmer. Shorter delay = brighter.
const int USER_FIRING_DELAY = 5000; // 5000µs = ~50% brightness

// --- Pin Definitions ---
// ADC Channels (ADS1115)
const int VOLTAGE_ADC_PIN = 0;   // ZMPT101B connected to A0 (Single-Ended)
const int CURRENT_ADC_DIFF_P = 1;// SCT-013 Biased Signal connected to A1
const int CURRENT_ADC_DIFF_N = 2;// Stable 2.5V Reference connected to A2
// Control Pins (Arduino)
const int ZCD_PIN = 2;       // PC814 output (MUST be D2 or D3)
const int TRIAC_PIN = 7;     // MOC3023M trigger (D7)

// --- Calibration Constants ---
// YOU MUST TUNE THESE LATER FOR ACCURACY
// Start by adjusting ZMPT Potentiometer while watching Vrms
double VOLTAGE_CAL = 155.0; // Initial guess, adjust based on multimeter
// For SCT-013-030 (30A/1V) -> 30.0. Needs scaling for ADS1115 gain & bits.
// We will calculate RMS directly from ADS readings, calibration integrated later.
double CURRENT_CAL = 1.0; // Placeholder

// --- ADS1115 Setup ---
Adafruit_ADS1115 ads;
// Voltage multiplier to convert ADS reading to Volts, depends on Gain
// For GAIN_TWOTHIRDS (+/- 6.144V), 1 bit = 0.1875mV = 0.0001875V
// For GAIN_ONE (+/- 4.096V), 1 bit = 0.125mV = 0.000125V
// For GAIN_TWO (+/- 2.048V), 1 bit = 0.0625mV = 0.0000625V
const float ADS_VOLTS_PER_BIT = 0.000125F; // Set based on GAIN_ONE initially
const int ADS1115_MID_POINT = 0; // Differential reading should be centered near 0

// --- Global Variables ---
volatile bool zcdFlag = false; // ZCD Interrupt flag
unsigned long lastPrintTime = 0; // For printing stats every second
const int sampleCount = 500;   // Number of samples for RMS calculation

// --- Interrupt Service Routine (ISR) ---
void onZeroCross() {
  zcdFlag = true;
}

// --- SETUP ---
void setup() {
  Serial.begin(9600); // Use a faster baud rate for more data
  Serial.println("AC Power Controller - Fixed Dimmer & Monitor Test");
  Serial.print("Firing delay: "); Serial.print(USER_FIRING_DELAY); Serial.println(" us");

  // Initialize ADS1115
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1); // Stop execution
  }
  // Set Gain - GAIN_ONE (+/- 4.096V) is a good starting point for signals centered at 2.5V
  ads.setGain(GAIN_ONE);
  Serial.println("ADS1115 Initialized.");

  // Setup Arduino Pins
  pinMode(TRIAC_PIN, OUTPUT);
  pinMode(ZCD_PIN, INPUT);
  digitalWrite(TRIAC_PIN, LOW); // Start Triac OFF

  // Attach Interrupt for ZCD
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, RISING);
}

// --- MAIN LOOP ---
void loop() {

  // --- Part 1: Triac Firing Logic (Interrupt Driven) ---
  if (zcdFlag == true) {
    delayMicroseconds(USER_FIRING_DELAY); // Wait for the set delay
    // Fire the Triac
    digitalWrite(TRIAC_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIAC_PIN, LOW);
    zcdFlag = false; // Reset the flag
  }

  // --- Part 2: Measurement Sampling and Calculation (Runs periodically) ---
  if (millis() - lastPrintTime >= 1000) { // Calculate and print every 1 second
    
    long sumVoltageSq = 0;
    long sumCurrentSq = 0;
    int16_t voltageSample;
    int16_t currentSample;

    // --- Take Samples ---
    for (int i = 0; i < sampleCount; i++) {
      // Read voltage (single-ended, relative to GND)
      // Assuming ZMPT101B output is centered at 2.5V, needs offset removal later
      voltageSample = ads.readADC_SingleEnded(VOLTAGE_ADC_PIN); 

      // Read current (differential A0 - A1)
      // This automatically removes the 2.5V DC offset if A1 is stable 2.5V
      currentSample = ads.readADC_Differential_0_1();

      // Placeholder for offset removal for voltage - find midpoint dynamically later
      // For now, assume ideal 2.5V midpoint needs calculation based on ADC range/gain
      // Example: If 0V=-32768, 5V=+32767, then 2.5V is near 0? No, single ended is 0-5V.
      // Need to find the actual DC offset for voltage. Let's sample and average later.
      // Quick Fix: Assume midpoint reading is around half the max positive range for 5V
      // e.g. for GAIN_ONE (+/-4.096V), 0V=0, 4.096V=32767 -> 2.5V ~ (2.5/4.096)*32767 = 19938
      // THIS NEEDS PROPER CALIBRATION - run a test just reading A0 with AC off.
      int voltageOffset = 20000; // MUST CALIBRATE THIS VALUE

      long filteredVoltage = voltageSample - voltageOffset;
      long filteredCurrent = currentSample; // Differential reading is already offset-removed

      sumVoltageSq += filteredVoltage * filteredVoltage;
      sumCurrentSq += filteredCurrent * filteredCurrent;
    }

    // --- Calculate RMS ---
    // Voltage
    double meanVoltageSq = (double)sumVoltageSq / sampleCount;
    double rmsVoltageRaw = sqrt(meanVoltageSq);
    double Vrms = rmsVoltageRaw * ADS_VOLTS_PER_BIT * VOLTAGE_CAL; // Apply scaling and calibration

    // Current
    double meanCurrentSq = (double)sumCurrentSq / sampleCount;
    double rmsCurrentRaw = sqrt(meanCurrentSq);
    // Need to calibrate current based on sensor ratio (30A/1V) and ADC scaling
    // 1V signal swing maps to ADC range. If GAIN_ONE, 1V = 1V/0.000125V = 8000 counts peak?
    // RMS of sine wave = Peak / sqrt(2). So 1V RMS ~ 8000 / sqrt(2) counts RMS.
    // Irms = (rmsCurrentRaw / (8000/sqrt(2))) * 1V_sensor_output * 30A/1V ???
    // Simpler: Find V/A experimentally. V = I*R -> A = V/R -> V = counts*ADS_VOLTS_PER_BIT
    // Example: If 1A RMS causes a signal of 0.033V RMS (1V/30A)
    // 0.033V RMS = 0.033 / ADS_VOLTS_PER_BIT = 264 counts RMS (approx)
    // So, CURRENT_CAL should be approx 1A / 264 counts = 0.00378
    CURRENT_CAL = 0.00378; // MUST CALIBRATE THIS VALUE with a known load!
    double Irms = rmsCurrentRaw * CURRENT_CAL;


    // --- Calculate Apparent Power ---
    double apparentPower = Vrms * Irms;

    // --- Print Results ---
    Serial.print("Vrms: "); Serial.print(Vrms, 2); Serial.print(" V | ");
    Serial.print("Irms: "); Serial.print(Irms, 3); Serial.print(" A | ");
    Serial.print("Apparent Power: "); Serial.print(apparentPower, 2); Serial.println(" VA");

    lastPrintTime = millis(); // Reset print timer
  }
}
