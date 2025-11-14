/*
 * AC DIMMER - SWEEPING DELAY CONTROL
 *
 * ZCD (PC814) connected to Pin D2.
 * Triac (MOC3023M) connected to Pin D7.
 *
 * Automatically sweeps through firing delays to show brightness levels.
 */

// --- USER SETTINGS ---
const int MIN_DELAY = 0;      // Brightest (microseconds)
const int MAX_DELAY = 10000;      // Dimmest (microseconds)
const int DELAY_STEP = 500;      // How much to change each step
const int CYCLES_PER_STEP = 250; // How many AC cycles to stay at each delay level

// --- Pin Definitions ---
const int ZCD_PIN = 2;     // Must be D2 for interrupt
const int TRIAC_PIN = 7;   // MOC3023M trigger

// --- Global Variables ---
volatile bool zcdFlag = false;
int currentDelay = MIN_DELAY;
int cycleCounter = 0;
bool sweepingUp = true;  // Direction of sweep

// --- Interrupt Service Routine ---
void onZeroCross() {
  zcdFlag = true;
}

void setup() {
  // Setup pins
  pinMode(TRIAC_PIN, OUTPUT);
  pinMode(ZCD_PIN, INPUT);
  
  // Start with Triac OFF
  digitalWrite(TRIAC_PIN, LOW);
  
  // Optional: Serial monitor to see current delay value
  Serial.begin(9600);
  Serial.println("AC Dimmer - Sweep Mode");
  Serial.print("Current Delay: ");
  Serial.println(currentDelay);
  
  // Attach Interrupt
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), onZeroCross, RISING);
}

void loop() {
  // If the ZCD told us the cycle just started...
  if (zcdFlag) {
    
    // 1. Wait for the current firing delay
    delayMicroseconds(currentDelay);
    
    // 2. Fire the Triac (short pulse)
    digitalWrite(TRIAC_PIN, HIGH);
    delayMicroseconds(10); 
    digitalWrite(TRIAC_PIN, LOW);
    
    // 3. Reset flag
    zcdFlag = false;
    
    // 4. Update cycle counter and change delay if needed
    cycleCounter++;
    
    if (cycleCounter >= CYCLES_PER_STEP) {
      cycleCounter = 0;
      
      // Change delay based on sweep direction
      if (sweepingUp) {
        currentDelay += DELAY_STEP;
        if (currentDelay >= MAX_DELAY) {
          currentDelay = MAX_DELAY;
          sweepingUp = false;  // Reverse direction
        }
      } else {
        currentDelay -= DELAY_STEP;
        if (currentDelay <= MIN_DELAY) {
          currentDelay = MIN_DELAY;
          sweepingUp = true;  // Reverse direction
        }
      }
      
      // Print new delay value to Serial Monitor
      Serial.print("Current Delay: ");
      Serial.print(currentDelay);
      Serial.print(" us | Brightness: ");
      if (currentDelay < 3000) Serial.println("High");
      else if (currentDelay < 6000) Serial.println("Medium");
      else Serial.println("Low");
    }
  }
}
