/*
 * DIAGNOSTIC TEST 1: THE "MUSCLE"
 * This code tests only the MOC and Triac.
 */

const int TRIAC_PIN = 7; // MOC3023M trigger (D7)

void setup() {
  Serial.begin(9600);
  pinMode(TRIAC_PIN, OUTPUT);
  digitalWrite(TRIAC_PIN, LOW); // Start with the Triac off
}

void loop() {
  // --- Brute-force ON for 2 seconds ---
  Serial.println("Setting Muscle: ON");
  unsigned long startTime = millis();
  while (millis() - startTime < 2000) {
    // Send a rapid, repeating pulse to "shotgun" the Triac on
    digitalWrite(TRIAC_PIN, HIGH);
    delayMicroseconds(1000); // 1ms pulse
    digitalWrite(TRIAC_PIN, LOW);
    delay(3); // 3ms wait (guarantees a hit)
  }

  // --- Force OFF for 2 seconds ---
  Serial.println("Setting Muscle: OFF");
  digitalWrite(TRIAC_PIN, LOW);
  delay(2000);
}
