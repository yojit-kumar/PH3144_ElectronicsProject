/*
 * AC POWER MONITOR - CALIBRATION & DISPLAY
 * 
 * Hardware:
 * - ADS1115 (I2C: 0x48)
 *   - A0: SCT-013-030 Current Sensor
 *   - A1: 2.5V Reference
 *   - A3: ZMPT101B Voltage Sensor
 * - 16x2 I2C LCD (I2C: 0x27 or 0x3F)
 * 
 * Commands via Serial Monitor:
 * - 'c' = Start calibration mode
 * - 'm' = Normal measurement mode
 * - 'v' = Adjust voltage calibration factor
 * - 'i' = Adjust current calibration factor
 */

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <LiquidCrystal_I2C.h>

// --- I2C Addresses ---
// Try 0x27 first, if LCD doesn't work, change to 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- ADS1115 Setup ---
Adafruit_ADS1115 ads;

// --- ADS Channels ---
const int CURRENT_CHANNEL = 0;  // A0 - SCT-013-030
const int VREF_CHANNEL = 1;     // A1 - 2.5V Reference
const int VOLTAGE_CHANNEL = 3;  // A3 - ZMPT101B

// --- Calibration Factors ---
// Adjust these values during calibration
float voltageCalFactor = 230.0;  // Expected mains voltage
float currentCalFactor = 0.0;   // SCT-013-030 rated current

// --- Measurement Variables ---
float voltage_rms = 0;
float current_rms = 0;
float power = 0;
float vref_voltage = 0;

// --- Sampling ---
const int SAMPLES = 1000;
const int SAMPLE_DELAY = 1; // microseconds

// --- Mode Control ---
enum Mode { CALIBRATION, MEASUREMENT };
Mode currentMode = MEASUREMENT;

void setup() {
  Serial.begin(115200);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Power Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  // Initialize ADS1115
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1115!");
    lcd.clear();
    lcd.print("ADS Error!");
    while (1);
  }
  
  // Set gain to ±4.096V (best for AC measurements around 2.5V center)
  ads.setGain(GAIN_ONE);
  
  delay(2000);
  lcd.clear();
  
  printMenu();
}

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }
  
  if (currentMode == CALIBRATION) {
    runCalibration();
  } else {
    runMeasurement();
  }
  
  delay(500);
}

void handleCommand(char cmd) {
  switch(cmd) {
    case 'c':
    case 'C':
      currentMode = CALIBRATION;
      Serial.println("\n=== CALIBRATION MODE ===");
      Serial.println("Ensure known loads are connected");
      break;
      
    case 'm':
    case 'M':
      currentMode = MEASUREMENT;
      Serial.println("\n=== MEASUREMENT MODE ===");
      break;
      
    case 'v':
    case 'V':
      adjustVoltageCal();
      break;
      
    case 'i':
    case 'I':
      adjustCurrentCal();
      break;
      
    case 'h':
    case 'H':
      printMenu();
      break;
  }
}

void printMenu() {
  Serial.println("\n========================================");
  Serial.println("    AC POWER MONITOR - MENU");
  Serial.println("========================================");
  Serial.println("Commands:");
  Serial.println("  c - Calibration Mode");
  Serial.println("  m - Measurement Mode");
  Serial.println("  v - Adjust Voltage Calibration");
  Serial.println("  i - Adjust Current Calibration");
  Serial.println("  h - Show this menu");
  Serial.println("========================================");
  Serial.print("Current Voltage Cal: "); Serial.println(voltageCalFactor);
  Serial.print("Current Current Cal: "); Serial.println(currentCalFactor);
  Serial.println("========================================\n");
}

void adjustVoltageCal() {
  Serial.println("\n--- Voltage Calibration ---");
  Serial.print("Current factor: "); Serial.println(voltageCalFactor);
  Serial.println("Enter new voltage calibration factor (e.g., 230):");
  
  while (!Serial.available()) {}
  voltageCalFactor = Serial.parseFloat();
  
  Serial.print("New voltage calibration: "); Serial.println(voltageCalFactor);
}

void adjustCurrentCal() {
  Serial.println("\n--- Current Calibration ---");
  Serial.print("Current factor: "); Serial.println(currentCalFactor);
  Serial.println("Enter new current calibration factor (e.g., 30):");
  
  while (!Serial.available()) {}
  currentCalFactor = Serial.parseFloat();
  
  Serial.print("New current calibration: "); Serial.println(currentCalFactor);
}

void runCalibration() {
  // Read reference voltage
  vref_voltage = readVref();
  
  // Read voltage and current
  voltage_rms = readVoltageRMS();
  current_rms = readCurrentRMS();
  power = voltage_rms * current_rms;
  
  // Print detailed calibration info
  Serial.println("\n--- CALIBRATION DATA ---");
  Serial.print("Vref: "); Serial.print(vref_voltage, 3); Serial.println(" V");
  Serial.print("Voltage RMS: "); Serial.print(voltage_rms, 2); Serial.println(" V");
  Serial.print("Current RMS: "); Serial.print(current_rms, 3); Serial.println(" A");
  Serial.print("Power: "); Serial.print(power, 2); Serial.println(" W");
  Serial.println("------------------------");
  
  // Display on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CAL V:");
  lcd.print(voltage_rms, 0);
  lcd.print("V I:");
  lcd.print(current_rms, 2);
  lcd.setCursor(0, 1);
  lcd.print("P:");
  lcd.print(power, 1);
  lcd.print("W");
}

void runMeasurement() {
  // Read measurements
  vref_voltage = readVref();
  voltage_rms = readVoltageRMS();
  current_rms = readCurrentRMS();
  power = voltage_rms * current_rms;
  
  // Print to serial
  Serial.print("V: "); Serial.print(voltage_rms, 1); Serial.print("V | ");
  Serial.print("I: "); Serial.print(current_rms, 3); Serial.print("A | ");
  Serial.print("P: "); Serial.print(power, 2); Serial.println("W");
  
  // Display on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("V:");
  lcd.print(voltage_rms, 1);
  lcd.print("V I:");
  lcd.print(current_rms, 2);
  lcd.print("A");
  
  lcd.setCursor(0, 1);
  lcd.print("Power: ");
  lcd.print(power, 1);
  lcd.print("W");
}

float readVref() {
  int16_t adc = ads.readADC_SingleEnded(VREF_CHANNEL);
  float voltage = ads.computeVolts(adc);
  return voltage;
}

float readVoltageRMS() {
  long sum = 0;
  
  for (int i = 0; i < SAMPLES; i++) {
    int16_t adc = ads.readADC_SingleEnded(VOLTAGE_CHANNEL);
    float voltage = ads.computeVolts(adc);
    
    // Remove DC offset (should be around Vref)
    float ac_voltage = voltage - vref_voltage;
    
    sum += (ac_voltage * ac_voltage);
    delayMicroseconds(SAMPLE_DELAY);
  }
  
  float mean_square = sum / (float)SAMPLES;
  float rms = sqrt(mean_square);
  
  // Scale to actual voltage using calibration factor
  // The ZMPT101B has a built-in voltage divider
  // Adjust this multiplier based on your module's specs
  float voltage_rms = rms * voltageCalFactor;
  
  return voltage_rms;
}

float readCurrentRMS() {
  long sum = 0;
  
  for (int i = 0; i < SAMPLES; i++) {
    int16_t adc = ads.readADC_SingleEnded(CURRENT_CHANNEL);
    float voltage = ads.computeVolts(adc);
    
    // Remove DC offset
    float ac_voltage = voltage - vref_voltage;
    
    sum += (ac_voltage * ac_voltage);
    delayMicroseconds(SAMPLE_DELAY);
  }
  
  float mean_square = sum / (float)SAMPLES;
  float rms_voltage = sqrt(mean_square);
  
  // Convert voltage to current using SCT-013-030 specs
  // SCT-013-030: 30A/1V (burden resistor creates 1V at 30A)
  float current_rms = (rms_voltage / 1.0) * currentCalFactor;
  
  return current_rms;
}
