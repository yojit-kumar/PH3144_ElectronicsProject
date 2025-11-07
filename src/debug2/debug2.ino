/*
 * DIAGNOSTIC TEST 2: THE "VOLTAGE SENSE"
 * This code tests the ZMPT101B module.
 */
const int VOLTAGE_SENSE_PIN = A0; // ZMPT101B OUT pin

void setup() {
  Serial.begin(9600);
  Serial.println("--- ZMPT101B Liveliness Test ---");
  pinMode(VOLTAGE_SENSE_PIN, INPUT);
}

void loop() {
  int sensorValue = analogRead(VOLTAGE_SENSE_PIN);
  Serial.println(sensorValue);
  delay(20);
}
