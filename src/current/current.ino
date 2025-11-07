// EmonLibrary examples openenergymonitor.org, Licence GNU GPL V3

#include "EmonLib.h"                   // Include Emon Library
EnergyMonitor emon1;                   // Create an instance

void setup()
{  
  Serial.begin(9600);
  
  emon1.current(1, 30.0);             // Current: input pin, calibration.
}

void loop()
{
  double Irms = emon1.calcIrms(1480);  // Calculate Irms only
//  if (Irms < 0.15) {
//    Irms = 0;
//  }
  
  Serial.print(Irms*230.0);	       // Apparent power
  Serial.print(" ");
  Serial.println(Irms);		       // Irms

  delay(1000);
}
