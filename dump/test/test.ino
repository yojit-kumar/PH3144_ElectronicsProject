// --- Pin Definitions ---
const int ZCD_PIN = 2;     // H11AA1 output. MUST be Pin 2 or 3.
const int TRIAC_PIN = 7;   // MOC3023M trigger. (This is D7)

volatile bool zcdFlag = false;

void onZeroCross() {
  zcdFlag = true;
}

void setup() {
  Serial.begin(9600);
  Serial.println("--- ZCD Test ---");
  Serial.println("Watching for ZCD interrupts...");

  pinMode(TRIAC_PIN, OUTPUT);
  pinMode(ZCD_PIN, INPUT);
  digitalWrite(TRIAC_PIN, LOW);
  
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, CHANGE);
}

void loop() {
  if (zcdFlag == true) {
    // The interrupt has fired!
    Serial.println("ZCD Fired! Firing Triac.");
    
    // Now we do the firing logic
    delayMicroseconds(5000); // 5ms delay
    digitalWrite(TRIAC_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIAC_PIN, LOW);
    
    zcdFlag = false; // Reset the flag
  }
}
