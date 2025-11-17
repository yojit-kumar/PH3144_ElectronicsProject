#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <LiquidCrystal_I2C.h>

// --- I2C Addresses ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- ADS1115 Setup ---
Adafruit_ADS1115 ads;

// --- Pin Definitions ---
const int ZCD_PIN = 2;       // PC814 ZCD
const int TRIAC_PIN = 7;     // MOC3023M trigger

// --- ADS Channels (ALL SINGLE-ENDED) ---
const int CURRENT_ADC_PIN = 0;     // A0 - SCT-013-030
const int VREF_ADC_PIN = 1;        // A1 - 2.5V Reference
const int VOLTAGE_ADC_PIN = 3;     // A3 - ZMPT101B

// --- ADS Settings ---
const float ADS_VOLTS_PER_BIT = 0.000125F; // For GAIN_ONE (+/- 4.096V)

// --- Calibration Values ---
int voltageOffset = 0;        // DC offset for voltage
int currentOffset = 0;        // DC offset for current
int vrefOffset = 0;           // Reference offset
double VOLTAGE_CAL = 155.0;   // Voltage scaling factor
double CURRENT_CAL = 30.0;    // Current scaling factor (30A/1V for SCT-013-030)

// --- Bulb Control ---
const int BULB_FIRING_DELAY = 1000; // Low delay = bright bulb for current testing
volatile bool zcdFlag = false;
bool bulbEnabled = false;

// --- Measurement Variables ---
const int sampleCount = 500;
float voltage_rms = 0;
float current_rms = 0;
float power = 0;
float vref_voltage = 0;

// --- Timing ---
unsigned long lastMeasurement = 0;
const int MEASUREMENT_INTERVAL = 1000;

// --- Calibration State Machine ---
enum CalState { 
  IDLE,
  OFFSET_CAL_WAIT,
  OFFSET_CAL_RUNNING,
  VOLTAGE_CAL_WAIT,
  VOLTAGE_CAL_RUNNING,
  CURRENT_CAL_WAIT,
  CURRENT_CAL_RUNNING,
  MEASUREMENT_MODE
};
CalState calState = IDLE;

// --- ZCD Interrupt ---
void onZeroCross() {
  zcdFlag = true;
}

void setup() {
  Serial.begin(9600);
  
  // Initialize Triac control
  pinMode(TRIAC_PIN, OUTPUT);
  pinMode(ZCD_PIN, INPUT);
  digitalWrite(TRIAC_PIN, LOW);
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, RISING);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Power Monitor"));
  lcd.setCursor(0, 1);
  lcd.print(F("Initializing..."));
  
  // Initialize ADS1115
  if (!ads.begin()) {
    Serial.println(F("Failed to initialize ADS1115!"));
    lcd.clear();
    lcd.print(F("ADS Error!"));
    while (1);
  }
  
  // Set gain to GAIN_ONE (+/- 4.096V) - SINGLE-ENDED MODE ONLY
  ads.setGain(GAIN_ONE);
  
  delay(2000);
  lcd.clear();
  
  Serial.println(F("   AC POWER MONITOR - CALIBRATION"));
  Serial.println(F("\nType 'START' to begin calibration"));
  Serial.println(F("Type 'MEASURE' to skip to measurement mode"));
}

void loop() {
  // Handle bulb control (if enabled)
  if (bulbEnabled && zcdFlag) {
    delayMicroseconds(BULB_FIRING_DELAY);
    digitalWrite(TRIAC_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIAC_PIN, LOW);
    zcdFlag = false;
  }
  
  // Check for serial commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();
    handleCommand(cmd);
  }
  
  // Run calibration state machine
  if (millis() - lastMeasurement >= MEASUREMENT_INTERVAL) {
    runCalibrationStateMachine();
    lastMeasurement = millis();
  }
}

void handleCommand(String cmd) {
  if (cmd == "START" || cmd == "S") {
    calState = OFFSET_CAL_WAIT;
    bulbEnabled = false;
    digitalWrite(TRIAC_PIN, LOW);
  } else if (cmd == "MEASURE" || cmd == "M") {
    calState = MEASUREMENT_MODE;
    Serial.println(F("\n=== MEASUREMENT MODE ==="));
    Serial.println(F("Type 'BULB ON' or 'BULB OFF' to control bulb"));
  } else if (cmd == "BULB ON") {
    bulbEnabled = true;
    Serial.println(F("Bulb: ON"));
  } else if (cmd == "BULB OFF") {
    bulbEnabled = false;
    digitalWrite(TRIAC_PIN, LOW);
    Serial.println(F("Bulb: OFF"));
  } else if (cmd == "NEXT" || cmd == "N") {
    // Advance to next calibration step
    if (calState == OFFSET_CAL_RUNNING) {
      calState = VOLTAGE_CAL_WAIT;
    } else if (calState == VOLTAGE_CAL_RUNNING) {
      calState = CURRENT_CAL_WAIT;
    } else if (calState == CURRENT_CAL_RUNNING) {
      calState = MEASUREMENT_MODE;
    }
  } else if (cmd.startsWith(F("VCAL="))) {
    // Manual voltage calibration: VCAL=230
    double actualVoltage = cmd.substring(5).toDouble();
    if (actualVoltage > 0 && voltage_rms > 0) {
      VOLTAGE_CAL = VOLTAGE_CAL * (actualVoltage / voltage_rms);
      Serial.print(F("New VOLTAGE_CAL: ")); Serial.println(VOLTAGE_CAL, 2);
    }
  } else if (cmd.startsWith(F("ICAL="))) {
    // Manual current calibration: ICAL=2.5
    double actualCurrent = cmd.substring(5).toDouble();
    if (actualCurrent > 0 && current_rms > 0) {
      CURRENT_CAL = CURRENT_CAL * (actualCurrent / current_rms);
      Serial.print(F("New CURRENT_CAL: ")); Serial.println(CURRENT_CAL, 5);
    }
  }
}

void runCalibrationStateMachine() {
  switch(calState) {
    case IDLE:
      // Waiting for user to start
      break;
      
    case OFFSET_CAL_WAIT:
      Serial.println(F("\n========================================"));
      Serial.println(F("STEP 1: OFFSET CALIBRATION"));
      Serial.println(F("1. Turn OFF all AC loads"));
      Serial.println(F("2. Disconnect AC from ZMPT101B and SCT-013"));
      Serial.println(F("3. Type 'NEXT' when ready"));
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("STEP 1: Offset"));
      lcd.setCursor(0, 1);
      lcd.print(F("Turn AC OFF"));
      
      calState = OFFSET_CAL_RUNNING;
      break;
      
    case OFFSET_CAL_RUNNING:
      runOffsetCalibration();
      break;
      
    case VOLTAGE_CAL_WAIT:
      Serial.println(F("\n========================================"));
      Serial.println(F("STEP 2: VOLTAGE CALIBRATION"));
      Serial.println(F("1. Turn ON AC power to ZMPT101B"));
      Serial.println(F("2. Keep SCT-013 disconnected or no load"));
      Serial.println(F("3. Measure actual voltage with multimeter"));
      Serial.println(F("4. Type 'NEXT' when ready"));
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("STEP 2: Voltage"));
      lcd.setCursor(0, 1);
      lcd.print(F("AC ON, no load"));
      
      calState = VOLTAGE_CAL_RUNNING;
      break;
      
    case VOLTAGE_CAL_RUNNING:
      runVoltageCalibration();
      break;
      
    case CURRENT_CAL_WAIT:
      Serial.println(F("\n========================================"));
      Serial.println(F("STEP 3: CURRENT CALIBRATION"));
      Serial.println(F("1. Connect SCT-013 around bulb wire"));
      Serial.println(F("2. BULB WILL TURN ON AUTOMATICALLY"));
      Serial.println(F("3. Measure actual current with clamp meter"));
      Serial.println(F("4. Type 'NEXT' when measurement done"));
      Serial.println(F("Starting bulb in 3 seconds..."));
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("STEP 3: Current"));
      lcd.setCursor(0, 1);
      lcd.print(F("Bulb turning ON"));
      
      delay(3000);
      bulbEnabled = true; // Turn on bulb automatically
      Serial.println(F(">>> BULB IS NOW ON <<<"));
      
      calState = CURRENT_CAL_RUNNING;
      break;
      
    case CURRENT_CAL_RUNNING:
      runCurrentCalibration();
      break;
      
    case MEASUREMENT_MODE:
      runMeasurement();
      break;
  }
}

void runOffsetCalibration() {
  long sumVoltage = 0;
  long sumCurrent = 0;
  long sumVref = 0;
  
  // Take samples with AC OFF
  for (int i = 0; i < sampleCount; i++) {
    sumVoltage += ads.readADC_SingleEnded(VOLTAGE_ADC_PIN);
    sumCurrent += ads.readADC_SingleEnded(CURRENT_ADC_PIN);
    sumVref += ads.readADC_SingleEnded(VREF_ADC_PIN);
  }
  
  voltageOffset = sumVoltage / sampleCount;
  currentOffset = sumCurrent / sampleCount;
  vrefOffset = sumVref / sampleCount;
  
  Serial.println(F("--- OFFSET CALIBRATION COMPLETE ---"));
  Serial.print(F("Voltage Offset: ")); Serial.println(voltageOffset);
  Serial.print(F("Current Offset: ")); Serial.println(currentOffset);
  Serial.print(F("Vref Offset: ")); Serial.println(vrefOffset);
  Serial.println(F("\nType 'NEXT' to continue to voltage calibration"));
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Offset: Done"));
  lcd.setCursor(0, 1);
  lcd.print(F("V:"));
  lcd.print(voltageOffset);
  lcd.print(F(" I:"));
  lcd.print(currentOffset);
}

void runVoltageCalibration() {
  long sumVoltageSq = 0;
  
  // Sample voltage only
  for (int i = 0; i < sampleCount; i++) {
    int16_t sample = ads.readADC_SingleEnded(VOLTAGE_ADC_PIN);
    long filtered = sample - voltageOffset;
    sumVoltageSq += filtered * filtered;
  }
  
  double meanVoltageSq = (double)sumVoltageSq / sampleCount;
  double rmsVoltageRaw = sqrt(meanVoltageSq);
  voltage_rms = rmsVoltageRaw * ADS_VOLTS_PER_BIT * VOLTAGE_CAL;
  
  Serial.println(F("--- VOLTAGE READING ---"));
  Serial.print(F("Raw RMS: ")); Serial.print(rmsVoltageRaw, 2); Serial.println(" counts");
  Serial.print(F("Calculated Voltage: ")); Serial.print(voltage_rms, 2); Serial.println(" V");
  Serial.println(F("\nMeasure actual voltage with multimeter"));
  Serial.println(F("Then type: VCAL=<actual_voltage>"));
  Serial.println(F("Example: VCAL=230"));
  Serial.println(F("\nOr type 'NEXT' to continue with current calibration"));
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Voltage:"));
  lcd.print(voltage_rms, 1);
  lcd.print(F("V"));
  lcd.setCursor(0, 1);
  lcd.print(F("Measure & type"));
}

void runCurrentCalibration() {
  long sumCurrentSq = 0;
  long sumVoltageSq = 0;
  
  // Sample both voltage and current
  for (int i = 0; i < sampleCount; i++) {
    int16_t vSample = ads.readADC_SingleEnded(VOLTAGE_ADC_PIN);
    int16_t iSample = ads.readADC_SingleEnded(CURRENT_ADC_PIN);
    
    long filteredV = vSample - voltageOffset;
    long filteredI = iSample - currentOffset;
    
    sumVoltageSq += filteredV * filteredV;
    sumCurrentSq += filteredI * filteredI;
  }
  
  double meanVoltageSq = (double)sumVoltageSq / sampleCount;
  double rmsVoltageRaw = sqrt(meanVoltageSq);
  voltage_rms = rmsVoltageRaw * ADS_VOLTS_PER_BIT * VOLTAGE_CAL;
  
  double meanCurrentSq = (double)sumCurrentSq / sampleCount;
  double rmsCurrentRaw = sqrt(meanCurrentSq);
  
  // Convert to current: SCT-013-030 outputs 1V for 30A
  // So output voltage = (current / 30A) * 1V
  // Current = (output voltage) * 30A
  current_rms = rmsCurrentRaw * ADS_VOLTS_PER_BIT * CURRENT_CAL;
  
  power = voltage_rms * current_rms;
  
  Serial.println(F("--- CURRENT READING ---"));
  Serial.print(F("Raw RMS: ")); Serial.print(rmsCurrentRaw, 2); Serial.println(" counts");
  Serial.print(F("Calculated Current: ")); Serial.print(current_rms, 3); Serial.println(" A");
  Serial.print(F("Voltage: ")); Serial.print(voltage_rms, 1); Serial.println(" V");
  Serial.print(F("Power: ")); Serial.print(power, 2); Serial.println(" W");
  Serial.println(F("\nMeasure actual current with clamp meter"));
  Serial.println(F("Then type: ICAL=<actual_current>"));
  Serial.println(F("Example: ICAL=2.5"));
  Serial.println(F("\nOr type 'NEXT' to finish calibration"));
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("I:"));
  lcd.print(current_rms, 2);
  lcd.print(F("A V:"));
  lcd.print(voltage_rms, 0);
  lcd.print(F("V"));
  lcd.setCursor(0, 1);
  lcd.print(F("P:"));
  lcd.print(power, 1);
  lcd.print(F("W Measure"));
}

void runMeasurement() {
  long sumVoltageSq = 0;
  long sumCurrentSq = 0;
  
  for (int i = 0; i < sampleCount; i++) {
    int16_t vSample = ads.readADC_SingleEnded(VOLTAGE_ADC_PIN);
    int16_t iSample = ads.readADC_SingleEnded(CURRENT_ADC_PIN);
    
    long filteredV = vSample - voltageOffset;
    long filteredI = iSample - currentOffset;
    
    sumVoltageSq += filteredV * filteredV;
    sumCurrentSq += filteredI * filteredI;
  }
  
  double meanVoltageSq = (double)sumVoltageSq / sampleCount;
  double rmsVoltageRaw = sqrt(meanVoltageSq);
  voltage_rms = rmsVoltageRaw * ADS_VOLTS_PER_BIT * VOLTAGE_CAL;
  
  double meanCurrentSq = (double)sumCurrentSq / sampleCount;
  double rmsCurrentRaw = sqrt(meanCurrentSq);
  current_rms = rmsCurrentRaw * ADS_VOLTS_PER_BIT * CURRENT_CAL;
  
  power = voltage_rms * current_rms;
  
  Serial.print(F("V:"));
  Serial.print(voltage_rms, 1);
  Serial.print(F("V | I:"));
  Serial.print(current_rms, 3);
  Serial.print(F("A | P:"));
  Serial.print(power, 2);
  Serial.println(F("W"));
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("V:"));
  lcd.print(voltage_rms, 0);
  lcd.print(F("V I:"));
  lcd.print(current_rms, 2);
  lcd.print(F("A"));
  lcd.setCursor(0, 1);
  lcd.print(F("P:"));
  lcd.print(power, 1);
  lcd.print(F("W"));
  if (bulbEnabled) {
    lcd.print(F(" ON"));
  } else {
    lcd.print(F(" OFF"));
  }
}
