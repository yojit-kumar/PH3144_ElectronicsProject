/*
 * ZCD Liveliness Test (Using PC814)
 *
 * This test checks if the PC814 circuit is successfully
 * reading the ZMPT101B signal and firing the interrupt on D2.
 */

// --- Pin Definitions ---
const int ZCD_PIN = 2;     // PC814 output (Pin 4). MUST be Pin 2 or 3.
const int SENSE_PIN = A0;  // ZMPT101B OUT (just to set the pin mode)

// This flag is set by the interrupt
volatile bool zcdFlag = false;

// --- Interrupt Service Routine (ISR) ---
// This function runs when the PC814 triggers Pin D2
void onZeroCross() {
  zcdFlag = true;
}

// --- SETUP ---
void setup() {
  Serial.begin(9600);
  Serial.println("--- ZCD Test (PC814) ---");
  Serial.println("Waiting for ZCD interrupts on Pin D2...");

  pinMode(ZCD_PIN, INPUT);
  pinMode(SENSE_PIN, INPUT);
  
  // Attach the interrupt to the ZCD pin
  // Trigger on the RISING edge (when the PC814 output goes HIGH)
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, RISING);
}

// --- MAIN LOOP ---
void loop() {
  
  if (zcdFlag == true) {
    // The interrupt has fired!
    Serial.println("ZCD Interrupt Fired!");
    
    // Reset the flag to wait for the next one
    zcdFlag = false; 
    
    delay(10); // A small delay to make the print readable
  }
}
