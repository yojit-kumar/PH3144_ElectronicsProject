#include <Wire.h>
#include <Adafruit_ADS1X15.h> // For the ADC
#include <PID_v1.h>           // For the PID Controller

// --- USER SETTINGS ---
// 1. SET YOUR TARGET POWER
const double setpointPower = 50.0; // Your target power in Watts (e.g., 50.0W)

// 2. MUST CALIBRATE THESE OFFSETS
//    Run the "Liveliness Test" with 230V AC OFF.
//    The stable number you see on A0 is your currentOffset.
//    The stable number you see on A3 is your voltageOffset.
const int voltageOffset = 20000; // Placeholder, MUST CALIBRATE
const int currentOffset = 20000; // Placeholder, MUST CALIBRATE

// 3. MUST CALIBRATE THESE CONSTANTS
//    Use a multimeter to measure Vrms and a known load to find Irms.
//    Adjust these values until the Serial.print matches your meter.
double VOLTAGE_CAL = 304.60;
double CURRENT_CAL = 1.0; 

// 4. PID TUNING (Start with these, then tune)
double Kp = 0.5;  // Proportional - How hard to react to error
double Ki = 0.1;  // Integral - How much to correct for past error
double Kd = 0.05; // Derivative - How much to predict future error

// --- HARDWARE & ADC DEFINITIONS ---
const int ZCD_PIN = 2;       // D2
const int TRIAC_PIN = 7;     // D7
const int CURR_ADC_S = 0;    // Current Signal (SCT-013) on AIN0
const int VOLT_ADC_S = 3;    // Voltage Signal (ZMPT101B) on AIN3
const float ADS_VOLTS_PER_BIT = 0.000125F; // For GAIN_ONE
const int sampleCount = 400; // Number of samples for RMS

// --- GLOBAL VARIABLES ---
volatile bool zcdFlag = false;
unsigned long lastCalcTime = 0;
unsigned long lastPrintTime = 0;

// --- PID VARIABLES ---
// These are the inputs and outputs for the PID controller
double measuredPower = 0.0; // PID Input (What we measure)
double pidOutput = 0.0;     // PID Output (0-255)
int firingDelay = 5000;     // Mapped from pidOutput

// Create the PID Controller Object
// We use REVERSE because: As measuredPower (Input) goes UP,
// we need pidOutput (Output) to go DOWN, which increases delay.
PID myPID(&measuredPower, &pidOutput, &setpointPower, Kp, Ki, Kd, REVERSE);

// Create the ADS Object
Adafruit_ADS1115 ads;

// --- INTERRUPT SERVICE ROUTINE ---
void onZeroCross() {
  zcdFlag = true;
}

// --- SETUP ---
void setup() {
  Serial.begin(115200); // Use a fast baud rate
  Serial.println("Closed-Loop PID Power Controller");

  // Initialize ADS1115
  if (!ads.begin()) {
    Serial.println("Failed to Initialize ADS");
    while (1);
  }
  ads.setGain(GAIN_ONE);
  Serial.println("ADS Initialized.");

  // Setup Arduino Pins
  pinMode(TRIAC_PIN, OUTPUT);
  pinMode(ZCD_PIN, INPUT);
  digitalWrite(TRIAC_PIN, LOW); // Start Triac OFF

  // Initialize PID Controller
  myPID.SetMode(AUTOMATIC);           // Turn the PID on
  myPID.SetOutputLimits(0, 255);      // PID output will be 0-255
  myPID.SetSampleTime(100);           // Compute PID every 100ms
  Serial.println("PID Initialized.");

  // Attach Interrupt
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, CHANGE);
}

// --- MAIN LOOP ---
void loop() {
  
  // --- Part 1: High-Priority Firing Logic ---
  if (zcdFlag == true) {
    // Wait for the delay calculated by the PID
    delayMicroseconds(firingDelay);
    
    // Fire the Triac
    digitalWrite(TRIAC_PIN, HIGH);
    delayMicroseconds(20); // Short pulse
    digitalWrite(TRIAC_PIN, LOW);
    
    zcdFlag = false; // Reset the flag
  }

  // --- Part 2: Low-Priority Calculation Logic ---
  // Run the PID calculation and RMS sampling every 100ms
  if (millis() - lastCalcTime >= 100) {
    
    // 1. Calculate the current power
    calculatePower(); 
    
    // 2. Run one cycle of the PID algorithm
    myPID.Compute(); 
    
    // 3. Map the PID's output (0-255) to a firing delay
    //    Output 0 = Min Power (long delay)
    //    Output 255 = Max Power (short delay)
    firingDelay = map(pidOutput, 0, 255, 9500, 1000); 
    
    lastCalcTime = millis();
  }

  // --- Part 3: Serial Printing (Runs every 1 second) ---
  if (millis() - lastPrintTime >= 1000) {
    Serial.print("Target: "); Serial.print(setpointPower, 2); Serial.print(" W | ");
    Serial.print("Measured: "); Serial.print(measuredPower, 2); Serial.print(" VA | ");
    Serial.print("PID Out: "); Serial.print(pidOutput, 0); Serial.print(" | ");
    Serial.print("Delay: "); Serial.print(firingDelay); Serial.println(" us");
    
    lastPrintTime = millis();
  }
}

// --- FUNCTION TO CALCULATE POWER ---
void calculatePower() {
  // Use 64-bit integers to prevent overflow (the 'nan' bug)
  unsigned long long sumVoltageSq = 0;
  unsigned long long sumCurrentSq = 0;
  
  int16_t voltageSample;
  int16_t currentSample;

  for (int i = 0; i < sampleCount; i++) {
    voltageSample = ads.readADC_SingleEnded(VOLT_ADC_S);
    currentSample = ads.readADC_SingleEnded(CURR_ADC_S);

    long filteredVoltage = voltageSample - voltageOffset;
    long filteredCurrent = currentSample - currentOffset;

    sumVoltageSq += (unsigned long long)filteredVoltage * filteredVoltage;
    sumCurrentSq += (unsigned long long)filteredCurrent * filteredCurrent;
  }

  // Calculate RMS Voltage
  double meanVoltageSq = (double)sumVoltageSq / sampleCount;
  double rmsVoltageRaw = sqrt(meanVoltageSq);
  double Vrms = rmsVoltageRaw * ADS_VOLTS_PER_BIT * VOLTAGE_CAL;

  // Calculate RMS Current
  double meanCurrentSq = (double)sumCurrentSq / sampleCount;
  double rmsCurrentRaw = sqrt(meanCurrentSq);
  double Irms = rmsCurrentRaw * ADS_VOLTS_PER_BIT * CURRENT_CAL;

  // Update the global measuredPower variable
  measuredPower = Vrms * Irms;
}
