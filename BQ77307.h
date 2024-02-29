#ifndef BQ77307_H
#define BQ77307_H

#include <Arduino.h>
#include <Wire.h>

class BQ77307 {
public:
    BQ77307();

    // Function declarations
    void sendCommand(byte regAddress);
    bool readAndDecodeSafetyAlertA();
    bool readAndDecodeSafetyFaultA();
    bool readAndDecodeSafetyAlertB();
    bool readAndDecodeSafetyFaultB();
    bool readAndDecodeBatteryStatus();
    bool readAndDecodeAlarmStatus();
    bool readAndDecodeAlarmStatusRaw();
    bool readAndDecodeAlarmStatusEnabled();
    bool readAndDecodeFetControl();
    bool readAndDecodeREGOUTControl();
    void Reset();
    void Toggle_FET_Control();
    void Seal_Configuration();
    void Enter_Configuration_Mode();
    void Exit_Configuration_Mode();
    void Enable_CRC();
    void Disable_CRC();
    int readRegister(byte regAddress, byte numBytes = 1, unsigned long timeout = 1000);
    int readRegister(byte regAddress, byte* buffer, byte numBytes, unsigned long timeout = 1000);

private:
    byte calculateCRC(byte* data, byte length);
    int readRegisterWithoutCRC(byte regAddress, byte numBytes = 1, unsigned long timeout = 1000);
    int readRegisterWithCRC(byte regAddress, byte numBytes = 1, unsigned long timeout = 1000);
    void writeRegisterWithoutCRC(byte regAddress, byte value);

    const byte _bq77307Address = 0x08;
    const int I2C_BUFFER_LENGTH = 32;
    bool CRC_ENABLED = false;
};

#endif // BQ77307_H
