/*
 * ZMPT101B Liveliness Test
 *
 * This sketch just reads the analog value from the ZMPT101B (on pin A0)
 * and prints it. This will tell us if the sensor is actually seeing
 * the 230V AC sine wave.
 */

const int VOLTAGE_SENSE_PIN = A0; // ZMPT101B OUT pin

void setup() {
  Serial.begin(9600);
  Serial.println("--- ZMPT101B Liveliness Test ---");
  pinMode(VOLTAGE_SENSE_PIN, INPUT);
}

void loop() {
  // Read the raw analog value (0-1023)
  int sensorValue = analogRead(VOLTAGE_SENSE_PIN);
  
  // Print the value to the Serial Monitor
  Serial.println(sensorValue);
  
  // A small delay to keep the Serial Monitor from
  // becoming an unreadable blur
  delay(20); 
}
