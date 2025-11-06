/*
 * DIAGNOSTIC TEST 3: THE "TIMING"
 * This code checks if the PC814 ZCD is firing.
 */
const int ZCD_PIN = 2; // PC814 output (Pin 4). MUST be D2 or D3.
volatile bool zcdFlag = false;

void onZeroCross() {
  zcdFlag = true;
}

void setup() {
  Serial.begin(9600);
  Serial.println("--- ZCD Test (PC814) ---");
  Serial.println("Waiting for ZCD interrupts on Pin D2...");
  pinMode(ZCD_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, RISING);
}

void loop() {
  if (zcdFlag == true) {
    Serial.println("ZCD Interrupt Fired!");
    zcdFlag = false; 
    delay(10);
  }
}
