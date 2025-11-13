/*
 * SIMPLE AC DIMMER - MANUAL DELAY CONTROL
 *
 * ZCD (PC814) connected to Pin D2.
 * Triac (MOC3023M) connected to Pin D7.
 *
 * Change 'FIRING_DELAY' to change brightness.
 * Range: 1000 (Bright) to 9000 (Dim).
 */

// --- USER SETTING ---
// Change this value to dim the bulb!
// 1000 = Bright | 5000 = Medium | 8500 = Dim
const int FIRING_DELAY = 1000; 

// --- Pin Definitions ---
const int ZCD_PIN = 2;     // Must be D2 for interrupt
const int TRIAC_PIN = 7;   // MOC3023M trigger

// --- Global Variables ---
volatile bool zcdFlag = false;

// --- Interrupt Service Routine ---
void onZeroCross() {
  zcdFlag = true;
}

void setup() {
  // Setup pins
  pinMode(TRIAC_PIN, OUTPUT);
  pinMode(ZCD_PIN, INPUT); // Assumes you have an external 10k pull-up
  
  // Start with Triac OFF
  digitalWrite(TRIAC_PIN, LOW);

  // Attach Interrupt
  // PC814 goes HIGH exactly when AC crosses zero
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, RISING);
}

void loop() {
  // If the ZCD told us the cycle just started...
  if (zcdFlag) {
    
    // 1. Wait for your specified delay
    delayMicroseconds(FIRING_DELAY);
    
    // 2. Fire the Triac (short pulse)
    digitalWrite(TRIAC_PIN, HIGH);
    delayMicroseconds(10); 
    digitalWrite(TRIAC_PIN, LOW);
    
    // 3. Reset flag and wait for next cycle
    zcdFlag = false;
  }
}
