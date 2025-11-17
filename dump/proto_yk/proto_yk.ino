#include <Wire.h>
#include <Adafruit_ADS1X15.h> // Include ADS1115 library
#include <ZMPT101B.h>

const int FIRING_DELAY = 3000;

const int CURR_ADC_P = 0;
const int CURR_ADC_N = 1;

const int ZCD_PIN = 2;
const int TRIAC_PIN = 7;

double VOLTAGE_CAL = 304.60;
double CURRENT_CAL = 1.0;

Adafruit_ADS1115 ads;

const float ADS_VOLTS_PER_BIT = 0.000125F;
const int ADS1115_MID_POINT = 0;

volatile bool zcdflag = false;
unsigned long lastPrintTime = 0;
const int sampleCount = 500;

void onZeroCross() {
  zcdflag = true;
}

ZMPT101B voltageSensor(A0, 50.0);

void setup() {
  Serial.begin(9600);
  Serial.println("TEST");
  Serial.print("Firing delay: "); Serial.print(FIRING_DELAY);

  if (!ads.begin()) {
    Serial.println("Failed to initialize");
    while (1);
  }

    ads.setGain(GAIN_ONE);
    Serial.println("ADS1115 Init");

    pinMode(TRIAC_PIN, OUTPUT);
    pinMode(ZCD_PIN, INPUT);
    digitalWrite(TRIAC_PIN, LOW);

    attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, RISING);

    voltageSensor.setSensitivity(304.60);
}

void loop() {
  if (zcdflag == true) {
    delayMicroseconds(FIRING_DELAY);
    digitalWrite(TRIAC_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIAC_PIN, LOW);
    zcdflag = false;
  }

  if (millis() - lastPrintTime >= 1000) { // Calculate and print every 1 second
    
    long sumVoltageSq = 0;
    long sumCurrentSq = 0;
    int16_t voltageSample;
    int16_t currentSample;

    // --- Take Samples ---
    for (int i = 0; i < sampleCount; i++) {

      voltageSample = voltageSensor.getRmsVoltage();
      currentSample = ads.readADC_Differential_0_1();

      long filteredVoltage = voltageSample;
      long filteredCurrent = currentSample;

      sumVoltageSq += filteredVoltage * filteredVoltage;
      sumCurrentSq += filteredCurrent * filteredCurrent;
    }

    // --- Calculate RMS ---
    // Voltage
    double meanVoltageSq = (double)sumVoltageSq / sampleCount;
    double rmsVoltageRaw = sqrt(meanVoltageSq);
    double Vrms = rmsVoltageRaw;

    // Current
    double meanCurrentSq = (double)sumCurrentSq / sampleCount;
    double rmsCurrentRaw = sqrt(meanCurrentSq);
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
