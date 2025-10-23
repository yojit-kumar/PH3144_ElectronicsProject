/**
 * This program reads the ZMPT101B voltage sensor and prints
 * the RMS voltage to the Serial Monitor.
*/
#include <ZMPT101B.h>
 
#define SENSITIVITY 485.0f
 
// ZMPT101B sensor output connected to analog pin A0
// and the voltage source frequency is 50 Hz.
ZMPT101B voltageSensor(A0, 50.0);
 
int buzzer = 4; // Buzzer pin
 
void setup() {
  Serial.begin(9600);
  pinMode(buzzer, OUTPUT);
  
  // Change the sensitivity value based on the value you got 
  // from the calibration example.
  voltageSensor.setSensitivity(SENSITIVITY);
}
 
void loop() {
  // Read the RMS voltage
  float voltage = voltageSensor.getRmsVoltage();
 
  // Print the voltage value to the Serial Monitor
  Serial.print("RMS Voltage: ");
  Serial.println(voltage);
 
  delay(1000); // Wait for a second before the next reading
}
