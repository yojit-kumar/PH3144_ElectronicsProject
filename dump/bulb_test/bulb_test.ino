/*
 * "DUMB" BLINKER TEST - NO ZCD REQUIRED
 * DANGER: This code controls 230V AC. Use with extreme caution.
 *
 * This test does not use a ZCD and may cause the bulb to flicker.
 * It is only to confirm that the MOC3023M and Triac are wired correctly.
 */

// --- Pin Definition ---
const int TRIAC_PIN = 7;   // Triac trigger (MOC3023M)

// --- Global Variables ---
bool bulbState = true;         // Current state of the bulb (true=ON, false=OFF)
unsigned long lastToggleTime = 0; // For the blink timer

// --- SETUP ---
void setup() {
  Serial.begin(9600); // For debug messages
  pinMode(TRIAC_PIN, OUTPUT);
  digitalWrite(TRIAC_PIN, LOW); // Start with the Triac off
}

// --- MAIN LOOP ---
void loop() {
  
  // --- Part 1: Blink Timer ---
  // This block checks if 2 seconds have passed and toggles the bulb's state.
  if (millis() - lastToggleTime > 2000) {
    bulbState = !bulbState; // Flip the state (ON to OFF, or OFF to ON)
    lastToggleTime = millis(); // Reset the timer
    
    if (bulbState == true) {
      Serial.println("Bulb ON");
    } else {
      Serial.println("Bulb OFF");
    }
  }

  // --- Part 2: Triac Firing Logic ---
  if (bulbState == true) {
    // If the bulb should be ON, we send a rapid, repeating
    // pulse to the Triac gate to "brute force" it on.
    // We do this by sending a 1ms HIGH pulse every 10ms.
    digitalWrite(TRIAC_PIN, HIGH);
    delayMicroseconds(1000); // Pulse for 1ms
    digitalWrite(TRIAC_PIN, LOW);
    delay(1); // Wait 9ms
    
  } else {
    // If the bulb should be OFF, we do nothing.
    // We just keep the pin LOW.
    digitalWrite(TRIAC_PIN, LOW);
  }
}
