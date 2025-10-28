// Define the pin number we want to use
int outputPin = 12;

// The setup() function runs once when you press reset or power the board
void setup() {
  // Set Pin 12 as an OUTPUT
  pinMode(outputPin, OUTPUT);
}

// The loop() function runs over and over again forever
void loop() {
  // Turn the pin ON (set it to 5V)
  digitalWrite(outputPin, HIGH);
  
  // Wait for 1000 milliseconds (1 second)
  delay(1000);
  
  // Turn the pin OFF (set it to 0V)
  digitalWrite(outputPin, LOW);
  
  // Wait for 1000 milliseconds (1 second)
  delay(1000);
}
