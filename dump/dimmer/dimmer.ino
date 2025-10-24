/*
 * AC PHASE CONTROL (DIMMER) - TEST SKETCH
 * DANGER: This code controls 230V AC. Use with extreme caution.
 *
 * This code uses the ZCD (H11AA1) to fire the Triac (MOC3023M)
 * after a fixed delay, allowing you to set a specific brightness.
 *
 * --- HARDWARE CONNECTIONS ---
 * ZMPT101B OUT -> Arduino A0
 * ZMPT101B OUT -> H11AA1 Input
 * H11AA1 OUT   -> Arduino D2 (CRITICAL: MUST be D2 or D3 for interrupt)
 * Arduino D7     -> MOC3023M Input
 */

// --- USER SETTING: CHANGE THIS VALUE ---
// Set the firing delay in microseconds (µs).
// A 50Hz AC half-cycle is 10,000µs.
//
// 8000 = Very Dim
// 5000 = Medium Brightness
// 1000 = Very Bright
const int USER_FIRING_DELAY = 5000; // 5000µs = ~50% brightness

// --- Pin Definitions (Based on your setup) ---
const int SENSE_PIN = A0;    // ZMPT101B signal (for EmonLib later)
const int ZCD_PIN = 2;       // H11AA1 output. MUST be Pin 2 or 3.
const int TRIAC_PIN = 7;     // MOC3023M trigger. (This is D7)

// --- Global Variables ---
// This flag is set by the interrupt. "volatile" is required.
volatile bool zcdFlag = false;

// --- Interrupt Service Routine (ISR) ---
// This function is called by the hardware every time the ZCD
// detects a zero-crossing. It must be as fast as possible.
void onZeroCross() {
  // Set our flag to true to signal the main loop
  zcdFlag = true;
}

// --- SETUP ---
void setup() {
  Serial.begin(9600);
  Serial.println("AC Dimmer Test");
  Serial.print("Firing delay set to: ");
  Serial.print(USER_FIRING_DELAY);
  Serial.println(" us");

  // Set pin modes
  pinMode(TRIAC_PIN, OUTPUT);
  pinMode(ZCD_PIN, INPUT);      // ZCD pin is an input
  pinMode(SENSE_PIN, INPUT);  // Set up the sense pin (though unused in this test)

  // Start with the Triac off
  digitalWrite(TRIAC_PIN, LOW);

  // Attach the interrupt to the ZCD pin
  // We trigger on the RISING edge, as designed by our H11AA1 circuit
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, RISING);
}

// --- MAIN LOOP ---
void loop() {
  
  // Check if the ZCD flag has been set by the interrupt
  if (zcdFlag == true) {
    
    // 1. Wait for our set delay time
    delayMicroseconds(USER_FIRING_DELAY);
    
    // 2. Fire the Triac by sending a brief pulse
    digitalWrite(TRIAC_PIN, HIGH);
    delayMicroseconds(10); // A 10us pulse is all it needs
    digitalWrite(TRIAC_PIN, LOW);
    
    // 3. Reset the flag to wait for the next zero-cross
    zcdFlag = false;
  }
}
