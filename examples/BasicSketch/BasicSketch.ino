#include <BQ77307.h>

BQ77307 bq77307; // Create an instance of the BQ77307 class

void setup() {
  Serial.begin(9600); // Initialize serial communication at 9600 baud rate
  while (!Serial); // Wait for the serial port to connect - necessary for Leonardo/Micro
  
  // Uncomment the next line if your device setup requires CRC checks
  // bq77307.Enable_CRC(); // Enable CRC checks if your device requires it

  Serial.println("BQ77307 status:");
  bq77307.readAndDecodeSafetyAlertA(); // Read and decode Safety Alert A
  bq77307.readAndDecodeSafetyStatusA(); // Read and decode Safety Status A
  bq77307.readAndDecodeSafetyAlertB(); // Read and decode Safety Alert B
  bq77307.readAndDecodeSafetyStatusB(); // Read and decode Safety Status B
}

void loop() {
  // Since the operations are in setup(), the loop can remain empty or be used for other periodic checks
}
