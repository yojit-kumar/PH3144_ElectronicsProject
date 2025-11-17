/*
 * AC DIMMER WITH POWER MONITORING - FINAL VERSION (FIXED)
 * 
 * Features:
 * - Fixed delay dimming control
 * - Real-time Voltage, Current, and Power measurement
 * - 16x2 I2C LCD Display
 * 
 * Hardware:
 * - ZMPT101B → ADS1115 A3 (Single-Ended)
 * - SCT-013-030 → ADS1115 A0 (+) and A1 (-) (Differential)
 * - ZCD (PC814) → Pin D2
 * - Triac (MOC3023M) → Pin D7
 * - 16x2 I2C LCD Display
 */

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <LiquidCrystal_I2C.h>

// ====================================
// USER SETTINGS - ADJUST THESE!
// ====================================

// Set firing delay (microseconds)
// 100 = Full Brightness | 5000 = Medium | 8500 = Very Dim
const int FIRING_DELAY = 1000;

// Calibration constants from calibration procedure
const double VOLTAGE_CAL = 158.39;    // Replace with your calibrated value
const double CURRENT_CAL = 0.000378;  // Replace with your calibrated value

// ====================================

// --- Pin Definitions ---
const int VOLTAGE_ADC_PIN = 3;      // ZMPT101B on A3
const int CURRENT_ADC_DIFF_P = 0;   // SCT-013 on A0
const int CURRENT_ADC_DIFF_N = 1;   // Reference on A1
const int ZCD_PIN = 2;              // PC814 Zero-Cross Detector
const int TRIAC_PIN = 7;            // Triac trigger

// --- Hardware Objects ---
Adafruit_ADS1115 ads;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD address 0x27 (change to 0x3F if needed)

// --- ADS1115 Constants ---
const float ADS_VOLTS_PER_BIT = 0.000125F; // For GAIN_ONE (+/- 4.096V)

// --- Measurement Variables ---
const int SAMPLE_COUNT = 500;           // Samples for RMS calculation
unsigned long lastMeasurement = 0;
const unsigned long MEASUREMENT_INTERVAL = 1000; // Update every 1 second

double Vrms = 0.0;
double Irms = 0.0;
double Power = 0.0;

// Voltage offset - calculated once at startup
int16_t voltageOffset = 0;

// --- Triac Control ---
volatile bool zcdFlag = false;

// --- Interrupt Service Routine ---
void onZeroCross() {
  zcdFlag = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("AC Dimmer with Power Monitoring");
  Serial.print("Firing Delay: "); Serial.print(FIRING_DELAY); Serial.println(" us");
  
  // Initialize ADS1115
  if (!ads.begin()) {
    Serial.println("ERROR: Failed to initialize ADS1115!");
    while (1);
  }
  ads.setGain(GAIN_ONE);
  Serial.println("ADS1115 Initialized");
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AC Power Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Calibrating...");
  
  // Calculate voltage offset once at startup
  delay(1000);
  voltageOffset = findVoltageOffset();
  Serial.print("Voltage Offset: "); Serial.println(voltageOffset);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  delay(1000);
  lcd.clear();
  
  // Setup pins
  pinMode(TRIAC_PIN, OUTPUT);
  pinMode(ZCD_PIN, INPUT);
  digitalWrite(TRIAC_PIN, LOW);
  
  // Attach interrupt
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, RISING);
  
  Serial.println("System Ready!");
  Serial.println("V(V)\tI(A)\tP(W)");
  Serial.println("----------------------------");
}

void loop() {
  // --- Part 1: Triac Firing (Critical Timing) ---
  if (zcdFlag) {
    delayMicroseconds(FIRING_DELAY);
    
    digitalWrite(TRIAC_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIAC_PIN, LOW);
    
    zcdFlag = false;
  }
  
  // --- Part 2: Periodic Measurements ---
  if (millis() - lastMeasurement >= MEASUREMENT_INTERVAL) {
    lastMeasurement = millis();
    
    // Measure voltage and current
    measurePower();
    
    // Update LCD display
    updateDisplay();
    
    // Print to Serial Monitor
    Serial.print(Vrms, 1); Serial.print("\t");
    Serial.print(Irms, 3); Serial.print("\t");
    Serial.println(Power, 2);
  }
}

void measurePower() {
  // --- Measure Voltage ---
  long sumVoltageSq = 0;
  int validSamples = 0;
  
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    int16_t voltageSample = ads.readADC_SingleEnded(VOLTAGE_ADC_PIN);
    
    // Check for valid reading
    if (voltageSample != -1) {
      long filteredVoltage = voltageSample - voltageOffset;
      sumVoltageSq += filteredVoltage * filteredVoltage;
      validSamples++;
    }
    delayMicroseconds(50);
  }
  
  // Calculate voltage RMS
  if (validSamples > 0) {
    double meanVoltageSq = (double)sumVoltageSq / validSamples;
    double rmsVoltageRaw = sqrt(meanVoltageSq);
    double rmsVoltageVolts = rmsVoltageRaw * ADS_VOLTS_PER_BIT;
    Vrms = rmsVoltageVolts * VOLTAGE_CAL;
  } else {
    Vrms = 0;
    Serial.println("Warning: No valid voltage samples");
  }
  
  // --- Measure Current ---
  long sumCurrentSq = 0;
  validSamples = 0;
  
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    int16_t currentSample = ads.readADC_Differential_0_1();
    
    // Check for valid reading
    if (currentSample != -1) {
      sumCurrentSq += (long)currentSample * (long)currentSample;
      validSamples++;
    }
    delayMicroseconds(50);
  }
  
  // Calculate current RMS
  if (validSamples > 0) {
    double meanCurrentSq = (double)sumCurrentSq / validSamples;
    double rmsCurrentRaw = sqrt(meanCurrentSq);
    Irms = rmsCurrentRaw * CURRENT_CAL;
  } else {
    Irms = 0;
    Serial.println("Warning: No valid current samples");
  }
  
  // --- Calculate Power ---
  Power = Vrms * Irms;
  
  // Sanity checks
  if (Vrms < 0 || Vrms > 300 || isnan(Vrms)) Vrms = 0;
  if (Irms < 0 || Irms > 30 || isnan(Irms)) Irms = 0;
  if (Power < 0 || Power > 7000 || isnan(Power)) Power = 0;
}

int16_t findVoltageOffset() {
  // Find DC offset by averaging samples
  long sum = 0;
  const int offsetSamples = 100;
  int validSamples = 0;
  
  for (int i = 0; i < offsetSamples; i++) {
    int16_t sample = ads.readADC_SingleEnded(VOLTAGE_ADC_PIN);
    if (sample != -1) {
      sum += sample;
      validSamples++;
    }
    delay(10);  // Longer delay during calibration
  }
  
  if (validSamples > 0) {
    return sum / validSamples;
  } else {
    Serial.println("ERROR: Could not find voltage offset!");
    return 20000; // Default fallback value
  }
}

void updateDisplay() {
  lcd.clear();
  
  // Line 1: Voltage and Current
  lcd.setCursor(0, 0);
  lcd.print("V:");
  
  if (Vrms < 100) {
    lcd.print(" ");  // Add space for alignment
  }
  lcd.print((int)Vrms);
  lcd.print("V");
  
  lcd.setCursor(8, 0);
  lcd.print("I:");
  lcd.print(Irms, 2);
  lcd.print("A");
  
  // Line 2: Power
  lcd.setCursor(0, 1);
  lcd.print("Power:");
  
  if (Power < 1000) {
    lcd.print((int)Power);
    lcd.print("W");
  } else {
    lcd.print(Power / 1000.0, 2);
    lcd.print("kW");
  }
}
