#include <Wire.h>

BQ77307::BQ77307() {
	Wire.begin(); // Initialize I2C communication
	CRC_ENABLED = false; // Default CRC check to disabled
}

// Function to read multiple bytes from a register from BQ77307 with timeout
int BQ77307::readRegisterWithoutCRC(byte regAddress, byte numBytes = 1, unsigned long timeout = 1000) {
	if (numBytes == 0 || numBytes > 4) return -1; // Adjust as necessary for the max expected bytes to be read
	byte data[numBytes]; // Buffer for address byte, data bytes, and CRC byte.
	int readRegister = readRegisterWithoutCRC(regAddress, &data, numBytes, timeout);
	if (readRegister == -1) return -1;

	// Accumulate the result here.
	int value = 0;
	for (byte i = 0; i < numBytes; ++i) {
		value <<= 8; // Shift left by 8 bits
		value |= data[i]; // Merge the next byte into the integer
	}

	return value; // Return the accumulated integer value
}

// Function to read bytes from a register without CRC checking from BQ77307
// Returns the number of bytes read or -1 if an error occurs.
int BQ77307::readRegisterWithoutCRC(byte regAddress, byte* buffer, byte numBytes = 1, unsigned long timeout = 1000)
{
	// Check buffer is not null and number of bytes is within bounds
	if (buffer == nullptr || numBytes == 0 || numBytes > I2C_BUFFER_LENGTH) {
		return -1;
	}

	// Begin transmission to write the register address
	Wire.beginTransmission(_bq77307Address);
	Wire.write(regAddress);
	if (Wire.endTransmission(false) != 0) { // End transmission with a repeated start
		return -1; // Error in transmission
	}

	// Request numBytes from the register
	Wire.requestFrom(_bq77307Address, numBytes);
	unsigned long startTime = millis();
	byte index = 0;
	while (index < numBytes)
	{
		if (Wire.available()) {
			buffer[index++] = Wire.read(); // Read bytes into buffer
		}
		if (millis() - startTime >= timeout) {
			return -1; // Timeout waiting for data
		}
	}

	return numBytes; // Return the number of bytes read
}

// Function to write to a register on BQ77307
void BQ77307::writeRegisterWithoutCRC(byte regAddress, byte value)
{
	Wire.beginTransmission(_bq77307Address);
	Wire.write(regAddress);
	Wire.write(value);
	Wire.endTransmission(); // End transmission and release the I2C bus
}

// Function to write to a register on BQ77307
void BQ77307::sendCommand(byte regAddress)
{
	Wire.beginTransmission(_bq77307Address);
	Wire.write(regAddress);
	Wire.endTransmission(); // End transmission and release the I2C bus
}

// Taken from the data sheet: https://www.ti.com/lit/ug/sluucy8/sluucy8.pdf?ts=1709147814970
byte BQ77307::calculateCRC(byte* data, byte length)
{
	byte crc = 0; // Initial value is 0
	for (byte i = 0; i < length; i++) {
		crc ^= data[i]; // XOR byte into least sig. byte of crc

		for (byte j = 8; j > 0; j--) { // Loop over each bit
			if ((crc & 0x80) != 0) { // If the uppermost bit is 1...
				crc = (byte)((crc << 1) ^ 0x07); // ...shift left and XOR with polynomial
			}
			else {
				crc <<= 1; // Just shift left
			}
		}
	}
	return crc; // Final value is the CRC
}

// Function to read multiple bytes from a register with CRC checking from BQ77307
int BQ77307::readRegisterWithCRC(byte regAddress, byte numBytes = 1, unsigned long timeout = 1000) {
	if (numBytes == 0 || numBytes > 4) return -1; // Adjust as necessary for the max expected bytes to be read
	byte data[numBytes+2]; // Buffer for address byte, data bytes, and CRC byte.
	int readRegister = readRegisterWithCRC(regAddress, &data, numBytes, timeout);
	if (readRegister == -1) return -1;

	// Accumulate the result here.
	int value = 0;
	for (byte i = 0; i < numBytes; ++i) {
		value <<= 8;
		value |= data[i + 1]; // The first byte is our own address, used for CRC
	}

	return value; // Return the accumulated integer value
}

// Function to read bytes from a register with CRC checking from BQ77307
// Returns the number of bytes read or -1 if an error occurs.
int BQ77307::readRegisterWithCRC(byte regAddress, byte* buffer, byte numBytes, unsigned long timeout = 1000)
{
	// Check buffer is not null and number of bytes is within bounds
	if (buffer == nullptr || numBytes == 0 || numBytes > I2C_BUFFER_LENGTH) {
		return -1;
	}

	// Calculate the expected number of bytes (numBytes + 1 for CRC)
	byte expectedBytes = numBytes + 1;
	byte receivedCRC;
	byte calculatedCRC;

	// Begin transmission to write the register address
	Wire.beginTransmission(_bq77307Address);
	Wire.write(regAddress);
	if (Wire.endTransmission(false) != 0) { // End transmission with a repeated start
		return -1; // Error in transmission
	}

	// Request numBytes + 1 (for CRC) from the register
	Wire.requestFrom(_bq77307Address, expectedBytes);
	unsigned long startTime = millis();
	byte index = 0;
	while (index < expectedBytes)
	{
		if (Wire.available()) {
			buffer[index++] = Wire.read(); // Read bytes into buffer
		}
		if (millis() - startTime >= timeout) {
			return -1; // Timeout waiting for data
		}
	}

	// Last byte is CRC
	receivedCRC = buffer[numBytes]; // Received CRC byte is the last byte
	buffer[0] = (_bq77307Address << 1) | 1; // Slave address with read bit for CRC calculation
	calculatedCRC = calculateCRC(buffer, numBytes + 1); // Calculate CRC over numBytes + 1 for address

	// Check if the calculated CRC matches the received CRC
	if (receivedCRC != calculatedCRC) {
		return -1; // CRC mismatch
	}

	return numBytes; // Return the number of bytes read, not including CRC
}

void BQ77307::writeRegisterWithCRC(byte regAddress, byte value)
{
	byte data[3];
	// For write operations, the CRC covers the slave address with write bit (0),
	// register address, and data byte.
	data[0] = (_bq77307Address << 1) | 0; // Slave address with write bit
	data[1] = regAddress; // Register address
	data[2] = value; // Data to write

	// Calculate CRC over address, register, and data
	byte crc = calculateCRC(data, 3);

	Wire.beginTransmission(_bq77307Address);
	Wire.write(regAddress);
	Wire.write(value);
	Wire.write(crc); // Send the CRC byte
	Wire.endTransmission(); // End transmission and release the I2C bus
}

int BQ77307::readRegister(byte regAddress, byte numBytes = 1, unsigned long timeout = 1000)
{
	if (CRC_ENABLED)
	{
		readRegisterWithCRC(regAddress, numBytes, timeout);
	}
	else
	{
		readRegisterWithoutCRC(regAddress, numBytes, timeout);
	}
}

int BQ77307::readRegister(byte regAddress, byte* buffer, byte numBytes, unsigned long timeout = 1000)
{
	if (CRC_ENABLED)
	{
		readRegisterWithCRC(regAddress, buffer, numBytes, timeout);
	}
	else
	{
		readRegisterWithoutCRC(regAddress, buffer, numBytes, timeout);
	}
}

// Function to read and decode the Safety Alert A register (command 0x02)
// returns true if all Safety Alert A bits are untripped
bool BQ77307::readAndDecodeSafetyAlertA()
{
	int safetyAlertA = readRegister(0x02); // Read the register with CRC checking
	if (safetyAlertA == -1)
	{
		Serial.println("Safety Alert A: Read Failed.");
		return false;
	}
	else if (safetyAlertA == 0)
	{
		Serial.println("Safety Alert A: OK.");
		return true;
	}

	// Decode Safety Alert A register bits
	Serial.println("Safety Alert A: Tripped!");
	if (safetyAlertA & (1 << 7)) Serial.println(" - COV: Cell Overvoltage Safety Alert");
	if (safetyAlertA & (1 << 6)) Serial.println(" - CUV: Cell Undervoltage Safety Alert");
	if (safetyAlertA & (1 << 5)) Serial.println(" - SCD: Short Circuit in Discharge Safety Alert");
	if (safetyAlertA & (1 << 4)) Serial.println(" - OCD1: Overcurrent in Discharge 1 Safety Alert");
	if (safetyAlertA & (1 << 3)) Serial.println(" - OCD2: Overcurrent in Discharge 2 Safety Alert");
	if (safetyAlertA & (1 << 2)) Serial.println(" - OCC: Overcurrent in Charge Safety Alert");
	// Bits 1 and 0 are reserved
	return false;
}

// Function to read and decode the Safety Status A register (command 0x03)
// returns true if all Safety Status A bits are untripped
bool BQ77307::readAndDecodeSafetyFaultA()
{
	int safetyStatusA = readRegister(0x03);
	if (safetyStatusA == -1)
	{
		Serial.println("Safety Fault A: Read Failed.");
		return false;
	}
	else if (safetyStatusA == 0)
	{
		Serial.println("Safety Fault A: OK.");
		return true;
	}

	// Decode Safety Alert A register bits
	Serial.println("Safety Fault A: Tripped!");
	if (safetyStatusA & (1 << 7)) Serial.println(" - COV: Cell Overvoltage Safety Fault");
	if (safetyStatusA & (1 << 6)) Serial.println(" - CUV: Cell Undervoltage Safety Fault");
	if (safetyStatusA & (1 << 5)) Serial.println(" - SCD: Short Circuit in Discharge Safety Fault");
	if (safetyStatusA & (1 << 4)) Serial.println(" - OCD1: Overcurrent in Discharge 1 Safety Fault");
	if (safetyStatusA & (1 << 3)) Serial.println(" - OCD2: Overcurrent in Discharge 2 Safety Fault");
	if (safetyStatusA & (1 << 2)) Serial.println(" - OCC: Overcurrent in Charge Safety Fault");
	if (safetyStatusA & (1 << 1)) Serial.println(" - CURLATCH: Current Protection Latch Safety Fault");
	if (safetyStatusA & (1 << 0)) Serial.println(" - REGOUT: REGOUT Safety Fault");
	return false;
}

// Function to read and decode the Safety Alert B register (command 0x04)
// returns true if all Safety Alert B bits are untripped
bool BQ77307::readAndDecodeSafetyAlertB()
{
	int safetyAlertB = readRegister(0x04);
	if (safetyAlertB == -1)
	{
		Serial.println("Safety Alert B: Read Failed.");
		return false;
	}
	else if (safetyAlertB == 0)
	{
		Serial.println("Safety Alert B: OK.");
		return true;
	}

	// Decode Safety Alert A register bits
	Serial.println("Safety Alert B: Tripped!");
	if (safetyAlertB & (1 << 7)) Serial.println(" - OTD: Overtemperature in Discharge Safety Alert");
	if (safetyAlertB & (1 << 6)) Serial.println(" - OTC: Overtemperature in Charge Safety Alert");
	if (safetyAlertB & (1 << 5)) Serial.println(" - UTD: Undertemperature in Discharge Safety Alert");
	if (safetyAlertB & (1 << 4)) Serial.println(" - UTC: Undertemperature in Charge Safety Alert");
	if (safetyAlertB & (1 << 3)) Serial.println(" - OTINT: Internal Overtemperature Safety Alert");
	// Bit 2 is reserved
	if (safetyAlertB & (1 << 1)) Serial.println(" - VREF: VREF Diagnostic Alert");
	if (safetyAlertB & (1 << 0)) Serial.println(" - VSS: VSS Diagnostic Alert");
	return false;
}

// Function to read and decode the Safety Status B register (command 0x05)
// returns true if all Safety Status B bits are untripped
bool BQ77307::readAndDecodeSafetyFaultB()
{
	int safetyStatusB = readRegister(0x05);
	if (safetyStatusB == -1)
	{
		Serial.println("Safety Fault B: Read Failed.");
		return false;
	}
	else if (safetyStatusB == 0)
	{
		Serial.println("Safety Fault B: OK.");
		return true;
	}

	// Decode Safety Alert A register bits
	Serial.println("Safety Fault B: Tripped!");
	if (safetyStatusB & (1 << 7)) Serial.println(" - OTD: Overtemperature in Discharge Safety Fault");
	if (safetyStatusB & (1 << 6)) Serial.println(" - OTC: Overtemperature in Charge Safety Fault");
	if (safetyStatusB & (1 << 5)) Serial.println(" - UTD: Undertemperature in Discharge Safety Fault");
	if (safetyStatusB & (1 << 4)) Serial.println(" - UTC: Undertemperature in Charge Safety Fault");
	if (safetyStatusB & (1 << 3)) Serial.println(" - OTINT: Internal Overtemperature Safety Fault");
	// Bit 2 is reserved
	if (safetyStatusB & (1 << 1)) Serial.println(" - VREF: VREF Diagnostic Fault");
	if (safetyStatusB & (1 << 0)) Serial.println(" - VSS: VSS Diagnostic Fault");
	// Bits 1 and 0 are reserved
	return false;
}


// Function to read and decode the Safety Status B register (command 0x05)
// returns true if all Safety Status B bits are untripped
bool BQ77307::readAndDecodeBatteryStatus()
{
	int batteryStatus = readRegister(0x12, 2);
	if (batteryStatus == -1)
	{
		Serial.println("Battery Status: Read Failed.");
		return false;
	}

	bool deviceNormalMode       = (batteryStatus & (1 << 15)) != 0;
	bool deviceSafetyAlert      = (batteryStatus & (1 << 13)) != 0;
	bool deviceSafetyFault      = (batteryStatus & (1 << 12)) != 0;
	int deviceSecurity          = (batteryStatus >> 10) & 0x03; // Extract bits 10 and 11 as a pair
	bool fetControl             = (batteryStatus & (1 << 8)) != 0;
	bool ramReset               = (batteryStatus & (1 << 7)) != 0;
	bool deviceConfigureMode    = (batteryStatus & (1 << 5)) != 0;
	bool deviceAlertPin         = (batteryStatus & (1 << 4)) != 0;
	bool chargeDriverEnabled    = (batteryStatus & (1 << 3)) != 0;
	bool dischargeDriverEnabled = (batteryStatus & (1 << 2)) != 0;
	bool chargeDetectorHigh     = (batteryStatus & (1 << 1)) != 0;

	int deviceMode = 0;
	if (deviceNormalMode)
	{
		if (deviceConfigureMode)
		{
			deviceMode = 2;
		}
		else
		{
			deviceMode = 1;
		}
	} else {
		deviceMode = 3; // I think? The documentation just says "not normal".
	}

	Serial.println("Battery Status:");
	Serial.print(" - Realized Device Mode: ");
	if (deviceMode == 0) Serial.println("Unknown");
	else if (deviceMode == 1) Serial.println("Normal");
	else if (deviceMode == 2) Serial.println("Configure");
	else if (deviceMode == 3) Serial.println("Shutdown");

	Serial.println(" - Device Mode Is Normal?: " + (deviceNormalMode ? "Normal" : "Not Normal!"));
	Serial.println(" - Device Alert: " + (deviceSafetyAlert ? "None" : "Alert!"));
	Serial.println(" - Device Fault: " + (deviceSafetyFault ? "None" : "Fault!"));

	Serial.print(" - Device Security: ");
	if (deviceSecurity == 0) Serial.println("Uninitialized");
	else if (deviceSecurity == 1) Serial.println("Full Access");
	else if (deviceSecurity == 2) Serial.println("Error");
	else if (deviceSecurity == 3) Serial.println("Sealed");

	Serial.println(" - MOSFET Mode: " + (fetControl ? "Manual" : "Automatic"));
	Serial.println(" - RAM Reset: " + (ramReset ? "True" : "False")); // If this is true, the device needs to be field programmed
	Serial.println(" - Device Mode Is Configure?: " + (deviceConfigureMode ? "Configure!" : "Not Configure"));

	Serial.println(" - Alert Pin: " + (deviceAlertPin ? "Active" : "Inactive"));
	Serial.println(" - Charge Driver Status: " + (chargeDriverEnabled ? "Active" : "Inactive"));
	Serial.println(" - Disharge Driver Status: " + (dischargeDriverEnabled ? "Active" : "Inactive"));
	Serial.println(" - Charge Detector: " + (chargeDetectorHigh ? "High" : "Low"));
}

// Function to read and decode the Alarm Status (command 0x62)
bool BQ77307::readAndDecodeAlarmStatus()
{
	int alarmStatus = readRegister(0x62, 2);
	if (alarmStatus == -1)
	{
		Serial.println("Alarm Status: Read Failed.");
		return false;
	}

	// Decode Alarm Status register bits
	bool SSA =      (alarmStatus & (1 << 15)) != 0;
	bool SSB =      (alarmStatus & (1 << 14)) != 0;
	bool SAA =      (alarmStatus & (1 << 13)) != 0;
	bool SAB =      (alarmStatus & (1 << 12)) != 0;
	bool XCHG =     (alarmStatus & (1 << 11)) != 0;
	bool XDSG =     (alarmStatus & (1 << 10)) != 0;
	bool SHUTV =    (alarmStatus & (1 << 9)) != 0;
	bool CHECK1 =   (alarmStatus & (1 << 7)) != 0;
	bool CHECK2 =   (alarmStatus & (1 << 6)) != 0;
	bool INITCOMP = (alarmStatus & (1 << 2)) != 0;
	bool CDTOGGLE = (alarmStatus & (1 << 1)) != 0;
	bool POR =      (alarmStatus & (1 << 0)) != 0;

	Serial.println("Alarm Status:");
	Serial.println(" - Safety Status A: " + (SSA ? "Tripped" : "OK"));
	Serial.println(" - Safety Status B: " + (SSB ? "Tripped" : "OK"));
	Serial.println(" - Safety Alert A: " + (SAA ? "Tripped" : "OK"));
	Serial.println(" - Safety Alert B: " + (SAB ? "Tripped" : "OK"));
	Serial.println(" - Charge Circuit: " + (XCHG ? "Tripped" : "OK"));
	Serial.println(" - Disharge Circuit: " + (XDSG ? "Tripped" : "OK"));
	Serial.println(" - Undervolt Alarm: " + (SHUTV ? "Tripped" : "OK")); // trips when a single cell or the stack drops too low, remains latched through SHUTDOWN mode
	Serial.println(" - Initialization Check 1: " + (CHECK1 ? "High" : "Low")); // This bit is latched when the device completes a CHECK interval while in NORMAL mode, and the bit is included in the mask.
	Serial.println(" - Initialization Check 2: " + (CHECK2 ? "High" : "Low")); // The bit is cleared when written with a "1". A bit set here causes the ALERT pin to be asserted low.
	Serial.println(" - Initialization State: " + (INITCOMP ? "High" : "Low")); // The bit is cleared when written with a "1". A bit set here causes the ALERT pin to be asserted low.
	Serial.println(" - Charge Detector: " + (CDTOGGLE ? "Detected" : "Not Detected")); // This bit is set when the CHG Detector output is set, indicating that the CHG pin has been	detected above a level of approximately 2 V
	Serial.println(" - RAM State: " + (POR ? "Uninitialized" : "Programmed")); // This bit is set when the device fully resets.It is cleared upon exit of CONFIG_UPDATE mode.It can be used by the host to determine if any RAM configuration changes were lost	due to a reset
	return true;
}

// Function to read and decode the Raw Alarm Status (command 0x64)
bool BQ77307::readAndDecodeAlarmStatusRaw()
{
	int alarmStatus = readRegister(0x64, 2);
	if (alarmStatus == -1)
	{
		Serial.println("Alarm Status Raw: Read Failed.");
		return false;
	}

	// Decode Raw Alarm Status register bits
	bool SSA =      (alarmStatus & (1 << 15)) != 0;
	bool SSB =      (alarmStatus & (1 << 14)) != 0;
	bool SAA =      (alarmStatus & (1 << 13)) != 0;
	bool SAB =      (alarmStatus & (1 << 12)) != 0;
	bool XCHG =     (alarmStatus & (1 << 11)) != 0;
	bool XDSG =     (alarmStatus & (1 << 10)) != 0;
	bool SHUTV =    (alarmStatus & (1 << 9)) != 0;
	bool CHECK1 =   (alarmStatus & (1 << 7)) != 0;
	bool CHECK2 =   (alarmStatus & (1 << 6)) != 0;
	bool INITCOMP = (alarmStatus & (1 << 2)) != 0;
	bool CDTOGGLE = (alarmStatus & (1 << 1)) != 0;
	bool POR =      (alarmStatus & (1 << 0)) != 0;

	Serial.println("Alarm Status Raw:");
	Serial.println(" - Safety Status A: " + (SSA ? "Tripped" : "OK"));
	Serial.println(" - Safety Status B: " + (SSB ? "Tripped" : "OK"));
	Serial.println(" - Safety Alert A: " + (SAA ? "Tripped" : "OK"));
	Serial.println(" - Safety Alert B: " + (SAB ? "Tripped" : "OK"));
	Serial.println(" - Charge Circuit: " + (XCHG ? "Tripped" : "OK"));
	Serial.println(" - Disharge Circuit: " + (XDSG ? "Tripped" : "OK"));
	Serial.println(" - Undervolt Alarm: " + (SHUTV ? "Tripped" : "OK")); // trips when a single cell or the stack drops too low, remains latched through SHUTDOWN mode
	Serial.println(" - Initialization Check 1: " + (CHECK1 ? "Ready" : "Alert")); // This bit is latched when the device completes a CHECK interval while in NORMAL mode, and the bit is included in the mask.
	Serial.println(" - Initialization Check 2: " + (CHECK2 ? "Ready" : "Alert")); // The bit is cleared when written with a "1". A bit set here causes the ALERT pin to be asserted low.
	Serial.println(" - Initialization State: " + (INITCOMP ? "Completed" : "Uninitialized")); // The bit is cleared when written with a "1". A bit set here causes the ALERT pin to be asserted low.
	Serial.println(" - Charge Detector: " + (CDTOGGLE ? "Updated" : "Ready")); // This bit is latched when the debounced CHG Detector signal is different from the last debounced value
	Serial.println(" - RAM State: " + (POR ? "Uninitialized" : "Programmed")); // This bit is set when the device fully resets.It is cleared upon exit of CONFIG_UPDATE mode.It can be used by the host to determine if any RAM configuration changes were lost	due to a reset
	return true;
}

// Function to read and decode the Alarm Status (command 0x66)
bool BQ77307::readAndDecodeAlarmStatusEnabled()
{
	int alarmStatus = readRegister(0x66, 2);
	if (alarmStatus == -1)
	{
		Serial.println("Alarm Status Raw: Read Failed.");
		return false;
	}

	// Decode Alarm Status register bits
	bool SSA =      (alarmStatus & (1 << 15)) != 0;
	bool SSB =      (alarmStatus & (1 << 14)) != 0;
	bool SAA =      (alarmStatus & (1 << 13)) != 0;
	bool SAB =      (alarmStatus & (1 << 12)) != 0;
	bool XCHG =     (alarmStatus & (1 << 11)) != 0;
	bool XDSG =     (alarmStatus & (1 << 10)) != 0;
	bool SHUTV =    (alarmStatus & (1 << 9)) != 0;
	bool CHECK1 =   (alarmStatus & (1 << 7)) != 0;
	bool CHECK2 =   (alarmStatus & (1 << 6)) != 0;
	bool INITCOMP = (alarmStatus & (1 << 2)) != 0;
	bool CDTOGGLE = (alarmStatus & (1 << 1)) != 0;
	bool POR =      (alarmStatus & (1 << 0)) != 0;

	Serial.println("Alarm Status Raw:");
	Serial.println(" - Safety Status A Alarm: " + (SSA ? "Enabled" : "Disabled"));
	Serial.println(" - Safety Status B Alarm: " + (SSB ? "Enabled" : "Disabled"));
	Serial.println(" - Safety Alert A Alarm: " + (SAA ? "Enabled" : "Disabled"));
	Serial.println(" - Safety Alert B Alarm: " + (SAB ? "Enabled" : "Disabled"));
	Serial.println(" - Charge Circuit Alarm: " + (XCHG ? "Enabled" : "Disabled"));
	Serial.println(" - Disharge Circuit Alarm: " + (XDSG ? "Enabled" : "Disabled"));
	Serial.println(" - Undervolt Alarm Alarm: " + (SHUTV ? "Enabled" : "Disabled"));
	Serial.println(" - Initialization Check 1 Alarm: " + (CHECK1 ? "Enabled" : "Disabled"));
	Serial.println(" - Initialization Check 2 Alarm: " + (CHECK2 ? "Enabled" : "Disabled"));
	Serial.println(" - Initialization State Alarm: " + (INITCOMP ? "Enabled" : "Disabled"));
	Serial.println(" - Charge Detector Alarm: " + (CDTOGGLE ? "Enabled" : "Disabled"));
	Serial.println(" - RAM State Alarm: " + (POR ? "Enabled" : "Disabled"));
	return true;
}

// Function to read and decode the Fet Control Status (command 0x68)
bool BQ77307::readAndDecodeFetControl()
{
	int fetControl = readRegister(0x68);
	if (fetControl == -1)
	{
		Serial.println("Fet Control: Read Failed.");
		return false;
	}

	// Decode Safety Alert A register bits
	bool CHG_OFF = (fetControl & (1 << 3)) != 0;
	bool DSG_OFF = (fetControl & (1 << 2)) != 0;
	bool CHG_ON = (fetControl & (1 << 1)) != 0;
	bool DSG_ON = (fetControl & (1 << 0)) != 0;

	Serial.println("FET Control Status:");
	Serial.println(" - Charge FET Forced On: " + (CHG_ON ? "True" : "False"));
	Serial.println(" - Charge FET Forced Off: " + (CHG_OFF ? "True" : "False"));
	Serial.println(" - Discharge FET Forced On: " + (DSG_ON ? "True" : "False"));
	Serial.println(" - Discharge FET Forced Off: " + (DSG_OFF ? "True" : "False"));
	return true;
}

// Function to read and decode the REGOUT Control Status (command 0x69)
bool BQ77307::readAndDecodeREGOUTControl()
{
	int fetControl = readRegister(0x69);
	if (fetControl == -1)
	{
		Serial.println("Fet Control: Read Failed.");
		return false;
	}

	// Decode Safety Alert A register bits
	bool TS_ON = (fetControl & (1 << 4)) != 0;
	bool REG_EN = (fetControl & (1 << 3)) != 0;
	int REGOUT_VOLTAGE_MODE = (fetControl & 0x07);
	float voltage = 0.0f;
	if (REGOUT_VOLTAGE_MODE == 0 || REGOUT_VOLTAGE_MODE == 1 || REGOUT_VOLTAGE_MODE == 2 || REGOUT_VOLTAGE_MODE == 3)
	{
		voltage = 1.8f;
	}
	else if (REGOUT_VOLTAGE_MODE == 4)
	{
		voltage = 2.5f;
	}
	else if (REGOUT_VOLTAGE_MODE == 5)
	{
		voltage = 3.0f;
	}
	else if (REGOUT_VOLTAGE_MODE == 6)
	{
		voltage = 3.3f;
	}
	else if (REGOUT_VOLTAGE_MODE == 7)
	{
		voltage = 5.0f;
	}

	Serial.println("REGOUT Control Status:");
	Serial.println(" - TS Enabled: " + (TS_ON ? "True" : "False"));
	Serial.println(" - REGOUT Enabled: " + (REG_EN ? "True" : "False"));
	Serial.print(" - REGOUT Voltage: ");
	Serial.println(voltage);
	return true;
}

// This command is sent to reset the device
void BQ77307::Reset()
{
	sendCommand(0x0012);
}

// This command is sent to toggle the FET_EN bit in Battery Status().
void BQ77307::Toggle_FET_Control()
{
	sendCommand(0x0022);
}

// This command is sent to place the device in SEALED mode
void BQ77307::Seal_Configuration()
{
	sendCommand(0x0030);
}

// This command is sent to place the device in CONFIG_UPDATE mode
void BQ77307::Enter_Configuration_Mode()
{
	sendCommand(0x0090);
}

// This command is sent to exit CONFIG_UPDATE mode
void BQ77307::Exit_Configuration_Mode()
{
	sendCommand(0x0092);
}

void BQ77307::Enable_CRC() {
	if (CRC_ENABLED) return; // If CRC is already enabled, do nothing.
	int value = readRegister(0x9017, 2); // Read the current value of the register.
	value |= 1; // Set bit 0.
	writeRegister(0x9017, value); // Write the new value back to the register.
	CRC_ENABLED = true; // Set the flag to true as CRC is now enabled.
}

void BQ77307::Disable_CRC() {
	if (!CRC_ENABLED) return; // If CRC is already disabled, do nothing.
	int value = readRegister(0x9017, 2); // Read the current value of the register.
	value &= ~1; // Clear bit 0.
	writeRegister(0x9017, value); // Write the new value back to the register.
	CRC_ENABLED = false; // Set the flag to false as CRC is now disabled.
}

// Sub Commands 9.4
