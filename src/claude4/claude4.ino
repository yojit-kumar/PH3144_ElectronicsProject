/*
 * AC DIMMER WITH POWER MONITORING - FINAL VERSION
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
const double VOLTAGE_CAL = 1300.00;    // Replace with your calibrated value
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
  lcd.print("Starting...");
  delay(2000);
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
  int16_t voltageOffset = findVoltageOffset();
  
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    int16_t voltageSample = ads.readADC_SingleEnded(VOLTAGE_ADC_PIN);
    long filteredVoltage = voltageSample - voltageOffset;
    sumVoltageSq += filteredVoltage * filteredVoltage;
    delayMicroseconds(100);
  }
  
  double meanVoltageSq = (double)sumVoltageSq / SAMPLE_COUNT;
  double rmsVoltageRaw = sqrt(meanVoltageSq);
  double rmsVoltageVolts = rmsVoltageRaw * ADS_VOLTS_PER_BIT;
  Vrms = rmsVoltageVolts * VOLTAGE_CAL;
  
  // --- Measure Current ---
  long sumCurrentSq = 0;
  
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    int16_t currentSample = ads.readADC_Differential_0_1();
    sumCurrentSq += (long)currentSample * (long)currentSample;
    delayMicroseconds(50);
  }
  
  double meanCurrentSq = (double)sumCurrentSq / SAMPLE_COUNT;
  double rmsCurrentRaw = sqrt(meanCurrentSq);
  Irms = rmsCurrentRaw * CURRENT_CAL;
  
  // --- Calculate Power ---
  Power = Vrms * Irms;
  
  // Sanity checks
  if (Vrms < 0 || Vrms > 300) Vrms = 0;
  if (Irms < 0 || Irms > 30) Irms = 0;
  if (Power < 0 || Power > 7000) Power = 0;
}

int16_t findVoltageOffset() {
  // Find DC offset by averaging samples
  long sum = 0;
  const int offsetSamples = 50;
  
  for (int i = 0; i < offsetSamples; i++) {
    sum += ads.readADC_SingleEnded(VOLTAGE_ADC_PIN);
    delayMicroseconds(50);
  }
  
  return sum / offsetSamples;
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
