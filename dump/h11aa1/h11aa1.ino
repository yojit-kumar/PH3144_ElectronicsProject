/*
 * H11AA1 "Bench Test"
 *
 * This test checks if the H11AA1's output transistor is working.
 * We will manually apply 5V to the input to see if the output changes.
 */

const int ZCD_PIN = 2; // H11AA1 output (Pin 5)

void setup() {
  Serial.begin(9600);
  Serial.println("--- H11AA1 Bench Test ---");
  Serial.println("Reading state of D2. Should be 1...");
  
  // We set D2 as an input. The pull-up resistor
  // in the circuit should hold it HIGH (1).
  pinMode(ZCD_PIN, INPUT); 
}

void loop() {
  int pinState = digitalRead(ZCD_PIN);
  Serial.println(pinState);
  delay(100);
}
