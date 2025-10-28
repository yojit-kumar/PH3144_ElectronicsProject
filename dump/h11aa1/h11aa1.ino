/*
 * H11AA1 "Bench Test"
 *
 * This test checks if the H11AA1's output circuit is working.
 * We will manually apply 5V to the input to force it to change.
 */

const int ZCD_PIN = 2; // H11AA1 output (Pin 5)

void setup() {
  Serial.begin(9600);
  Serial.println("--- H11AA1 Bench Test ---");
  Serial.println("Now reading D2. Expecting '1'...");
  
  // We set D2 as an input. The pull-up resistor
  // in your circuit should hold it HIGH (1).
  pinMode(ZCD_PIN, INPUT); 
}

void loop() {
  int pinState = digitalRead(ZCD_PIN);
  
  Serial.print("D2 is: ");
  Serial.println(pinState);
  
  delay(250); // A short delay so we can read it
}
