/*
 * AC SENSOR CALIBRATION TOOL - FIXED
 * 
 * Step 1: Calibrate ZMPT101B Voltage Sensor (bulb OFF)
 * Step 2: Calibrate SCT-013-030 Current Sensor (bulb ON at full brightness)
 * 
 * Hardware:
 * - ZMPT101B → ADS1115 A3 (Single-Ended)
 * - SCT-013-030 → ADS1115 A0 (+) and A1 (-) (Differential)
 * - ZCD (PC814) → Pin D2
 * - Triac (MOC3023M) → Pin D7
 */

#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// --- Pin Definitions ---
const int VOLTAGE_ADC_PIN = 3;      // ZMPT101B on A3
const int CURRENT_ADC_DIFF_P = 0;   // SCT-013 on A0
const int CURRENT_ADC_DIFF_N = 1;   // Reference on A1
const int ZCD_PIN = 2;              // PC814 Zero-Cross Detector
const int TRIAC_PIN = 7;            // Triac trigger

// --- ADS1115 Setup ---
Adafruit_ADS1115 ads;
const float ADS_VOLTS_PER_BIT = 0.000125F; // For GAIN_ONE (+/- 4.096V)

// --- Calibration Variables ---
const int SAMPLE_COUNT = 1000;      // Number of samples for RMS
const int CALIBRATION_CYCLES = 5;  // Number of measurement cycles

// --- Triac Control ---
volatile bool zcdFlag = false;
bool triacEnabled = false;
const int ZERO_DELAY = 100;         // Minimal delay for full brightness (microseconds)

// --- Interrupt Service Routine ---
void onZeroCross() {
  zcdFlag = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("====================================");
  Serial.println("  AC SENSOR CALIBRATION TOOL");
  Serial.println("====================================");
  Serial.println();
  
  // Initialize ADS1115
  if (!ads.begin()) {
    Serial.println("ERROR: Failed to initialize ADS1115!");
    while (1);
  }
  
  ads.setGain(GAIN_ONE); // +/- 4.096V range
  Serial.println("ADS1115 Initialized (GAIN_ONE: +/- 4.096V)");
  
  // Setup pins
  pinMode(TRIAC_PIN, OUTPUT);
  pinMode(ZCD_PIN, INPUT);
  digitalWrite(TRIAC_PIN, LOW);
  
  // Attach interrupt for zero-cross detection
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, RISING);
  
  Serial.println("Hardware Initialized");
  Serial.println();
  delay(2000);
  
  // Start calibration sequence
  runCalibration();
}

void loop() {
  // Handle triac firing if enabled
  if (triacEnabled && zcdFlag) {
    delayMicroseconds(ZERO_DELAY);
    digitalWrite(TRIAC_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIAC_PIN, LOW);
    zcdFlag = false;
  }
}

void runCalibration() {
//  Serial.println("====================================");
//  Serial.println("STEP 1: VOLTAGE CALIBRATION");
//  Serial.println("====================================");
//  Serial.println("Instructions:");
//  Serial.println("1. Ensure bulb is OFF (Triac disabled)");
//  Serial.println("2. Connect ZMPT101B to mains voltage");
//  Serial.println("3. Measure actual voltage with multimeter");
//  Serial.println("4. Note the voltage (should be ~230V)");
//  Serial.println();
//  Serial.println("Starting voltage measurement in 5 seconds...");
//  delay(5000);
  
  // Measure voltage with bulb OFF
  calibrateVoltage();
  
  Serial.println();
  Serial.println("====================================");
  Serial.println("STEP 2: CURRENT CALIBRATION");
  Serial.println("====================================");
  Serial.println("Instructions:");
  Serial.println("1. Turning bulb ON at FULL BRIGHTNESS");
  Serial.println("2. Measure actual current with multimeter");
  Serial.println("3. Note the current reading");
  Serial.println();
  Serial.println("Turning bulb ON in 5 seconds...");
  Serial.println("Make sure your multimeter is ready!");
  delay(5000);
  
  // Enable triac for full brightness
  triacEnabled = true;
  
  Serial.println();
  Serial.println("*** BULB IS NOW ON! ***");
  Serial.println("Waiting 3 seconds for bulb to stabilize...");
  delay(3000);
  
  // Measure current with bulb ON
  calibrateCurrent();
  
  // Turn off bulb
  triacEnabled = false;
  digitalWrite(TRIAC_PIN, LOW);
  
  Serial.println();
  Serial.println("*** BULB TURNED OFF ***");
  Serial.println();
  Serial.println("====================================");
  Serial.println("CALIBRATION COMPLETE!");
  Serial.println("====================================");
  Serial.println("You can now use the calibration");
  Serial.println("constants in your main code.");
  Serial.println();
  
  while(1); // Stop here
}

void calibrateVoltage() {
  Serial.println("--- Voltage Calibration Measurements ---");
  Serial.println();
  
  double totalRawRMS = 0;
  
  for (int cycle = 0; cycle < CALIBRATION_CYCLES; cycle++) {
    long sumVoltageSq = 0;
    int16_t voltageOffset = findVoltageOffset();
    
    // Sample voltage
    for (int i = 0; i < SAMPLE_COUNT; i++) {
      int16_t voltageSample = ads.readADC_SingleEnded(VOLTAGE_ADC_PIN);
      unsigned long long filteredVoltage = voltageSample - voltageOffset;
      sumVoltageSq += filteredVoltage * filteredVoltage;
      delayMicroseconds(100);
    }
    
    // Calculate RMS
    double meanVoltageSq = (double)sumVoltageSq / SAMPLE_COUNT;
    double rmsVoltageRaw = sqrt(meanVoltageSq);
    totalRawRMS += rmsVoltageRaw;
    
    double rmsVoltageVolts = rmsVoltageRaw * ADS_VOLTS_PER_BIT;
    
    Serial.print("Cycle "); Serial.print(cycle + 1);
    Serial.print(": Raw RMS = "); Serial.print(rmsVoltageRaw, 2);
    Serial.print(" counts, Voltage = "); Serial.print(rmsVoltageVolts, 4);
    Serial.println(" V");
    
    delay(500);
  }
  
  double avgRawRMS = totalRawRMS / CALIBRATION_CYCLES;
  double avgVoltageVolts = avgRawRMS * ADS_VOLTS_PER_BIT;
  
  Serial.println();
  Serial.println("--- Voltage Calibration Results ---");
  Serial.print("Average Raw RMS: "); Serial.print(avgRawRMS, 2); Serial.println(" counts");
  Serial.print("Average Voltage: "); Serial.print(avgVoltageVolts, 4); Serial.println(" V");
  Serial.println();
  
//  Serial.println("TO CALIBRATE:");
//  Serial.println("1. Measure actual mains voltage with multimeter (e.g., 230V)");
//  Serial.println("2. Calculate: VOLTAGE_CAL = (Your_Multimeter_Reading) / " + String(avgVoltageVolts, 4));
//  Serial.println();
//  Serial.print("Example: If multimeter shows 230V, then VOLTAGE_CAL = 230 / ");
//  Serial.print(avgVoltageVolts, 4);
//  Serial.print(" = ");
//  Serial.println(230.0 / avgVoltageVolts, 2);
//  Serial.println();
//  
  // Wait before next step
  delay(3000);
}

void calibrateCurrent() {
  Serial.println("--- Current Calibration Measurements ---");
  Serial.println("Bulb should be ON and bright now!");
  Serial.println();
  
  double totalRawRMS = 0;
  
  for (int cycle = 0; cycle < CALIBRATION_CYCLES; cycle++) {
    long sumCurrentSq = 0;
    
    // Sample current (differential reading) - with small delays to allow triac firing
    for (int i = 0; i < SAMPLE_COUNT; i++) {
      int16_t currentSample = ads.readADC_Differential_0_1();
      sumCurrentSq += (long)currentSample * (long)currentSample;
      
      // Very short delay to allow loop() to keep firing triac
      delayMicroseconds(50);
    }
    
    // Calculate RMS
    double meanCurrentSq = (double)sumCurrentSq / SAMPLE_COUNT;
    double rmsCurrentRaw = sqrt(meanCurrentSq);
    totalRawRMS += rmsCurrentRaw;
    
    double rmsCurrentVolts = rmsCurrentRaw * ADS_VOLTS_PER_BIT;
    
    Serial.print("Cycle "); Serial.print(cycle + 1);
    Serial.print(": Raw RMS = "); Serial.print(rmsCurrentRaw, 2);
    Serial.print(" counts, Signal = "); Serial.print(rmsCurrentVolts, 4);
    Serial.println(" V");
    
    // Short delay between cycles
    delay(200);
  }
  
  double avgRawRMS = totalRawRMS / CALIBRATION_CYCLES;
  double avgCurrentVolts = avgRawRMS * ADS_VOLTS_PER_BIT;
  
  Serial.println();
  Serial.println("--- Current Calibration Results ---");
  Serial.print("Average Raw RMS: "); Serial.print(avgRawRMS, 2); Serial.println(" counts");
  Serial.print("Average Signal: "); Serial.print(avgCurrentVolts, 4); Serial.println(" V");
  Serial.println();
  
  Serial.println("TO CALIBRATE:");
  Serial.println("1. Measure actual current with multimeter (e.g., 0.26A for 60W bulb)");
  Serial.println("2. Calculate: CURRENT_CAL = (Your_Multimeter_Reading) / " + String(avgRawRMS, 2));
  Serial.println();
  Serial.print("Example: If multimeter shows 0.26A, then CURRENT_CAL = 0.26 / ");
  Serial.print(avgRawRMS, 2);
  Serial.print(" = ");
  
  if (avgRawRMS > 0) {
    Serial.println(0.26 / avgRawRMS, 6);
  } else {
    Serial.println("ERROR - No current detected!");
    Serial.println();
    Serial.println("TROUBLESHOOTING:");
    Serial.println("- Check if bulb is actually ON (visually confirm)");
    Serial.println("- Check ZCD signal on Pin D2 (should pulse)");
    Serial.println("- Check Triac connection to Pin D7");
    Serial.println("- Verify SCT-013-030 is clamped around the wire");
    Serial.println("- Check ADS1115 connections (A0 and A1)");
  }
  Serial.println();
}

int16_t findVoltageOffset() {
  // Find DC offset by averaging multiple samples
  long sum = 0;
  const int offsetSamples = 100;
  
  for (int i = 0; i < offsetSamples; i++) {
    sum += ads.readADC_SingleEnded(VOLTAGE_ADC_PIN);
    delayMicroseconds(100);
  }
  
  return sum / offsetSamples;
}
