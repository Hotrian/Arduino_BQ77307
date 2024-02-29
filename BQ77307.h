#ifndef BQ77307_h
#define BQ77307_h

#include "Arduino.h"
#include <Wire.h> // Include the Arduino Wire library for I2C communication

class BQ77307 {
public:
	BQ77307(uint8_t deviceAddress); // Constructor with the I2C device address
	void begin(TwoWire& wirePort = Wire); // Initialize communication with the BQ77307, defaulting to the standard Wire port
	float readBatteryVoltage(); // Method to read battery voltage
	float readBatteryCurrent(); // Method to read battery current
	int readBatteryTemperature(); // Method to read battery temperature
	bool isCharging(); // Method to check if the battery is charging
	bool isFullyCharged(); // Method to check if the battery is fully charged
	void setChargeCurrentLimit(float current); // Method to set the charge current limit

private:
	TwoWire* _wire; // Pointer to the I2C bus (Wire) instance
	uint8_t _deviceAddress; // I2C device address of the BQ77307
	// Private method to send a read command to the BQ77307
	uint8_t readRegister8(uint8_t reg);
	// Private method to write a value to a register of the BQ77307
	void writeRegister8(uint8_t reg, uint8_t value);
	// Add other private member variables and helper methods as needed for communication and data processing
};

#endif
