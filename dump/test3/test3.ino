/*
 * ZCD Liveliness Test
 *
 * This test checks if the H11AA1 circuit is successfully
 * reading the ZMPT101B signal and firing the interrupt on D2.
 */

// --- Pin Definitions ---
const int ZCD_PIN = 2;     // H11AA1 output. MUST be Pin 2 or 3.
const int SENSE_PIN = A0;  // ZMPT101B OUT (just to set the pin mode)

// This flag is set by the interrupt
volatile bool zcdFlag = false;

// --- Interrupt Service Routine (ISR) ---
// This function runs when the H11AA1 triggers Pin D2
void onZeroCross() {
  zcdFlag = true;
}

// --- SETUP ---
void setup() {
  Serial.begin(9600);
  Serial.println("--- ZCD Test ---");
  Serial.println("Waiting for ZCD interrupts on Pin D2...");

  pinMode(ZCD_PIN, INPUT);
  pinMode(SENSE_PIN, INPUT);
  
  // Attach the interrupt to the ZCD pin
  // We'll check for a RISING edge first.
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
