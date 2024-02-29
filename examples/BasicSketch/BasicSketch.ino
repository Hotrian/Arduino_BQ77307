#include <BQ77307.h>

BQ77307 bq77307; // Create an instance of the BQ77307 class

void setup() {
  Serial.begin(9600); // Initialize serial communication at 9600 baud rate
  while (!Serial); // Wait for the serial port to connect - necessary for Leonardo/Micro
  
  // Uncomment the next line if your device setup requires CRC checks
  // bq77307.Enable_CRC(); // Enable CRC checks if your device requires it

  bq77303.Enter_Configuration_Mode();
  bq77303.Enable_REGOUT();
  bq77303.Exit_Configuration_Mode();

  Serial.println("BQ77307 status:");
  bq77307.readAndDecodeAlarmStatus();
  bq77307.readAndDecodeREGOUTControl()
}

void loop() {
  // Since the operations are in setup(), the loop can remain empty or be used for other periodic checks
}
