#include "MiniXDevice.hpp"
#include "ftd2xx.h"
#include "Debug.hpp"
#include "Utilities.hpp"

REGISTER_DEVICE(MiniXDevice,"Mini-X");
using namespace std;
using namespace Utilities;

#pragma region Bit Definitions

/*
LOW
Bit 0 (0x01): CLKSTATE     - SPI Clock signal
Bit 1 (0x02): DATASTATE    - SPI Data signal  
Bit 2 (0x04): [Unused/Reserved]
Bit 3 (0x08): ADCS         - ADC Chip Select
Bit 4 (0x10): DACS         - DAC Chip Select
Bit 5 (0x20): CTRL_HV_EN_A - High Voltage Enable A
Bit 6 (0x40): CTRL_HV_EN_B - High Voltage Enable B  
Bit 7 (0x80): [Unused/Reserved]
High
Bit 0 (0x01): [Unused/Reserved]
Bit 1 (0x02): [Unused/Reserved]
Bit 2 (0x04): [Unused/Reserved]
Bit 3 (0x08): TSCS         - Temperature Sensor Chip Select
Bit 4 (0x10): [Unused/Reserved]
Bit 5 (0x20): [Unused/Reserved]
Bit 6 (0x40): [Unused/Reserved]
Bit 7 (0x80): [Unused/Reserved]
*/

// Byte Definitions for Mini-X control

#define  SET(x, y)   ((x) |= (y)) 
#define  CLEAR(x, y)   ((x) &= (~y)) 

#define CLK_FN_NEG 0x10     //Clock out 2 bytes on negative edge, MSB first
#define CLK_FN CLK_FN_NEG

// MPSSE Commands
#define CMD_SET_DATA_BITS_LOWBYTE   0x80    // Set Data Bits Low Byte
#define CMD_SET_DATA_BITS_HIGHBYTE  0x82    // Set Data Bits High Byte
#define CMD_GET_DATA_BITS_LOWBYTE   0x81    // Get Data Bits Low Byte
#define CMD_GET_DATA_BITS_HIGHBYTE  0x83    // Get Data Bits High Byte
#define CMD_SET_CLK_DIVISOR         0x86    // Set CLK divisor
#define CMD_CLOCK_OUT_BITS_MSB      0x13    // Clock out bits, MSB first
#define CMD_CLOCK_IN_BYTES_MSB      0x20    // Clock in bytes, MSB first
#define CMD_CLOCK_OUT_BYTES_NEG     0x10    // Clock out bytes on negative edge, MSB first

// Pin/Port Definitions
#define OUTPUTMODE                  0x7B    // Output mode mask
#define INPUTMODE                   OUTPUTMODE
#define OUTPUTMODE_H                0x08    // High byte output mode
#define INPUTMODE_H                 OUTPUTMODE_H

// Chip Selects and Control Pins
#define TSCS                        0x08    // Temperature sensor chip select (high byte)
#define ADCS                        0x08    // ADC chip select
#define DACS                        0x10    // DAC chip select
#define CTRL_HV_EN_A                0x20    // HV Enable A
#define CTRL_HV_EN_B                0x40    // HV Enable B
#define DATASTATE                   0x02    // Data state when configured as output
#define CLKSTATE                    0x01    // Clock state

// DAC/ADC Channel Addresses
#define DACA                        0x18    // DAC Channel A
#define DACB                        0x19    // DAC Channel B
#define AD0                         0xD0    // ADC Channel 0 (Voltage)
#define AD1                         0xF0    // ADC Channel 1 (Current)

// GPIO Register Offsets
#define GPIO_LOW                    0x00    // Low byte GPIO register
#define GPIO_HIGH                   0x02    // High byte GPIO register

// Temperature Sensor Commands
#define TSCMD                       0xE0    // Temperature sensor command
#define TSSTATUS                    0x00    // Temperature sensor status
#define TSLSB                       0x01    // Temperature sensor LSB
#define TSMSB                       0x02    // Temperature sensor MSB
#define TSCONFIG                    0x80    // Temperature sensor config

// Clock and Timing
#define CLK_DIVISOR_LOW             0x03    // Clock divisor low byte
#define CLK_DIVISOR_HIGH            0x00    // Clock divisor high byte

// Length Definitions
#define LENGTH_1_BYTE               0x00    // 0=1byte
#define LENGTH_2_BYTES              0x01    // 1=2bytes
#define LENGTH_4_BITS               0x03    // 3=4bits

#pragma endregion

bool MiniXDevice::connect() {
    if (!connection.fConnect()) return false;
    if (!initializeGPIOs()) return false;
    return true;
}

bool MiniXDevice::disconnect() {
    return true;
}

void MiniXDevice::update() {
    safetyChecks();
}

void MiniXDevice::setupTasks() {
    addTask([this]{ currentTemperature = readTemperature(); }, 1000);
    addTask([this]{ currentCurrent = readCurrent(); }, 2000);
    addTask([this]{ currentVoltage = readVoltage(); }, 3000);
}

double MiniXDevice::readValue(const std::string& parameter) {
    // Implement reading values based on the parameter
    return 0.0;
}

bool MiniXDevice::setValue(const std::string& parameter, double value) {
    // Implement setting values based on the parameter
    return true;
}

bool MiniXDevice::initialize() {
    if (!setupTemperatureSensor()) return false;
    if (!setupClockDivisor()) return false;
    

    //setVoltage(0.0);
    //sleepMs(200);
    //setCurrent(0.0);

    if constexpr (debug) Debug.Log("MiniX initialization completed successfully.");
    return true;
}

// Minix Self Functions

bool MiniXDevice::setupClockDivisor(){
    unsigned char tx[10]; DWORD ret_bytes; int pos = 0;
    setClockDivisor(tx, pos, 3);
    FT_STATUS status = connection.sendData(tx, pos);
    if (status != FT_OK) { Debug.Error("Error setting clock divisor: ", status); return false; }
    if constexpr (debug) Debug.Log("Clock divisor set successfully.");
    return true;
}

bool MiniXDevice::setupTemperatureSensor(){
    unsigned char tx[100]; DWORD ret_bytes; int pos = 0;
    setClockDivisor(tx, pos, 0);

    //take TSCS low and clock 2 bytes of data into the Temp Sensor
    tx[pos++] = 0x80;//Setup MPSSE Low byte I/O lines
    CLEAR(LowByteHiLowState, CLKSTATE);//take CLock low
    tx[pos++] = LowByteHiLowState;
    tx[pos++] = OUTPUTMODE;

    // set TS chip select high
    tx[pos++] = 0x82;//Setup MPSSE High byte I/O lines
    SET(HighByteHiLowState, TSCS);  //take chip select high
    tx[pos++] = HighByteHiLowState;//1110 0011  hi/low state if output (clock & data init to high)
    tx[pos++] = OUTPUTMODE_H;

    //take TSCS low and clock 2 bytes of data into the Temp Sensor
    tx[pos++] = 0x80;//Setup MPSSE Low byte I/O lines
    CLEAR(LowByteHiLowState, CLKSTATE);//take CLock low
    tx[pos++] = LowByteHiLowState;
    tx[pos++] = OUTPUTMODE;

    //take TSCS low and clock 2 bytes of data into the Temp Sensor
    tx[pos++] = 0x80;//Setup MPSSE Low byte I/O lines
    CLEAR(LowByteHiLowState, CLKSTATE);//take CLock low
    tx[pos++] = LowByteHiLowState;
    tx[pos++] = OUTPUTMODE;

    //take TSCS low and clock 2 bytes of data into the Temp Sensor
    tx[pos++] = 0x80;//Setup MPSSE Low byte I/O lines
    SET(LowByteHiLowState, CLKSTATE);
    tx[pos++] = LowByteHiLowState;
    tx[pos++] = OUTPUTMODE;

    tx[pos++] = 0x10;   //Clock out 2 bytes on positive edge, MSB first
    tx[pos++] = 0x01;   //LengthL 0=1byte, 1=2bytes
    tx[pos++] = 0x00;   //LengthH 
    tx[pos++] = TSCONFIG; //set configuration register
    tx[pos++] = TSCMD + 0x08;   // continuous convert, 12-bit res., no shutdown

    // set TS chip select low
    tx[pos++] = 0x82;//Setup MPSSE High byte I/O lines
    CLEAR(HighByteHiLowState, TSCS);  //take chip select low
    tx[pos++] = HighByteHiLowState;// 1110 0011  hi/low state if output (clock & data init to high)
    tx[pos++] = OUTPUTMODE_H;

    FT_STATUS status = connection.sendData(tx, pos);
    if (status != FT_OK) { Debug.Error("Temperature Sensor setup error"); return false; }
    return true;
}

double MiniXDevice::readVoltage() {
    if (!connection.isDeviceOpen() || !connection.isMPSSEOn()) {Debug.Error("Device not open or MPSSE not enabled for temperature reading.");return -1.0;}
    unsigned char tx[100], rx[100]; DWORD ret_bytes; int pos = 0;

    setClockDivisor(tx, pos);
    
    // Start condition - take ADC clock enable low
    tx[pos++] = CMD_SET_DATA_BITS_LOWBYTE;
    LowByteHiLowState &= ~ADCS; // Clear ADCS bit (take ADC clock enable low)
    LowByteHiLowState &= ~CLKSTATE; // Clear CLKSTATE (take clock low)
    tx[pos++] = LowByteHiLowState;
    tx[pos++] = OUTPUTMODE;

    // Send control nibble - clock out 4 bits
    tx[pos++] = CMD_CLOCK_OUT_BITS_MSB;
    tx[pos++] = LENGTH_4_BITS;
    tx[pos++] = AD0; // Voltage channel

    // Set data direction to input
    tx[pos++] = CMD_SET_DATA_BITS_LOWBYTE;
    tx[pos++] = LowByteHiLowState;
    tx[pos++] = INPUTMODE;

    // Read 2 bytes from A/D conversion
    tx[pos++] = CMD_CLOCK_IN_BYTES_MSB;
    tx[pos++] = LENGTH_2_BYTES;
    tx[pos++] = LENGTH_1_BYTE; // LengthH

    // Take ADCS back high
    tx[pos++] = CMD_SET_DATA_BITS_LOWBYTE;
    LowByteHiLowState |= ADCS; // Set ADCS bit (take ADC enable high)
    tx[pos++] = LowByteHiLowState;
    tx[pos++] = OUTPUTMODE;

    // Send the command string
    FT_STATUS status = connection.sendData(tx, pos);
    if (status != FT_OK) {Debug.Error("HV ADC write command error: ", status);return -1.0;}
    if (status == FT_OK) {Debug.Log("HV ADC write command sent successfully.");}

    sleepMs(50);

    // Read the reply and checksum validation
    status = connection.receiveData(rx, 2, ret_bytes);
    if (status != FT_OK) {Debug.Error("HV ADC read data status error: ", status);return -1.0;}
    if (ret_bytes < 2) {Debug.Error("HV ADC too few data bytes returned: ", ret_bytes);return -1.0;}
    if (!Utilities::validateAndLogData(rx, 2, "HV ADC")) {return -1.0;}

    // Convert ADC result to voltage (bit manipulation handled in utility)
    double voltage = convertToVoltage(rx[0], rx[1]);

    Debug.Log("Read voltage: " + std::to_string(voltage) + " kV");
    return voltage;
}

double MiniXDevice::readCurrent() {
    if (!connection.isDeviceOpen() || !connection.isMPSSEOn()) {Debug.Error("Device not open or MPSSE not enabled for temperature reading.");return -1.0;}
    unsigned char tx[100], rx[100]; DWORD ret_bytes; int pos = 0;

    setClockDivisor(tx, pos);
    
    // Start condition - take ADC clock enable low
    tx[pos++] = CMD_SET_DATA_BITS_LOWBYTE;
    LowByteHiLowState &= ~ADCS; // Clear ADCS bit (take ADC clock enable low)
    LowByteHiLowState &= ~CLKSTATE; // Clear CLKSTATE (take clock low)
    tx[pos++] = LowByteHiLowState;
    tx[pos++] = OUTPUTMODE;

    // Send control nibble - clock out 4 bits
    tx[pos++] = CMD_CLOCK_OUT_BITS_MSB;
    tx[pos++] = LENGTH_4_BITS;
    tx[pos++] = AD1; // Current channel

    // Set data direction to input
    tx[pos++] = CMD_SET_DATA_BITS_LOWBYTE;
    tx[pos++] = LowByteHiLowState;
    tx[pos++] = INPUTMODE;

    // Read 2 bytes from A/D conversion
    tx[pos++] = CMD_CLOCK_IN_BYTES_MSB;
    tx[pos++] = LENGTH_2_BYTES;
    tx[pos++] = LENGTH_1_BYTE; // LengthH

    // Take ADCS back high
    tx[pos++] = CMD_SET_DATA_BITS_LOWBYTE;
    LowByteHiLowState |= ADCS; // Set ADCS bit (take ADC enable high)
    tx[pos++] = LowByteHiLowState;
    tx[pos++] = OUTPUTMODE;

    // Send the command string
    FT_STATUS status = connection.sendData(tx, pos);
    if (status != FT_OK) {Debug.Error("Current ADC write command error: ", status);return -1.0;}
    if constexpr (debug) Debug.Log("Current ADC write command sent successfully.");
    sleepMs(200);

    // Read the reply
    status = connection.receiveData(rx, 2, ret_bytes);
    if (status != FT_OK) {Debug.Error("Current ADC read data status error: ", status);return -1.0;}
    if (ret_bytes < 2) {Debug.Error("Current ADC too few data bytes returned: ", ret_bytes);return -1.0;}
    if (!Utilities::validateAndLogData(rx, 2, "Current ADC")) {return -1.0;}

    double current = convertToCurrent(rx[0], rx[1]);

    Debug.Log("Read current: " + std::to_string(current) + " uA");
    return current;
}

double MiniXDevice::readTemperature() {
    if (!connection.isDeviceOpen() || !connection.isMPSSEOn()) {Debug.Error("Device not open or MPSSE not enabled for temperature reading.");return -1.0;}
    unsigned char tx[100], rx[100]; DWORD ret_bytes; int pos = 0;

    setClockDivisor(tx, pos, 3);
    // Set TS chip select low (prepare for communication)
    tx[pos++] = CMD_SET_DATA_BITS_HIGHBYTE;
    CLEAR(HighByteHiLowState, TSCS);  // TS chip select low
    tx[pos++] = HighByteHiLowState;
    tx[pos++] = OUTPUTMODE_H;
    tx[pos++] = CMD_SET_DATA_BITS_LOWBYTE;
    CLEAR(LowByteHiLowState, CLKSTATE);
    CLEAR(LowByteHiLowState, DATASTATE);
    tx[pos++] = LowByteHiLowState;
    tx[pos++] = OUTPUTMODE;
    activateTemperatureSensor(tx, pos, HighByteHiLowState);
    tx[pos++] = CMD_SET_DATA_BITS_LOWBYTE;
    SET(LowByteHiLowState, CLKSTATE);
    CLEAR(LowByteHiLowState, DATASTATE);
    tx[pos++] = LowByteHiLowState;
    tx[pos++] = OUTPUTMODE;
    // Read cmd
    tx[pos++] = 0x12; // Clock out bits, MSB first (different from 0x13!)
    tx[pos++] = 0x07; // 7 = 8 bits - 1
    tx[pos++] = 0x01; // CORRECT temp sensor command (not 0xE0!)
    // Read 2 bytes from temperature sensor (CORRECT COMMAND!)
    tx[pos++] = 0x20; // Clock IN bytes, MSB first (not CMD_CLOCK_IN_BYTES_MSB!)
    tx[pos++] = LENGTH_2_BYTES;  // Read 2 bytes
    tx[pos++] = LENGTH_1_BYTE;   // High byte = 0 (total = 2 bytes)
    deactivateTemperatureSensor(tx, pos, HighByteHiLowState);

    FT_STATUS status = connection.sendData(tx, pos);
    if (status != FT_OK) {Debug.Error("Temperature sensor write command error: ", status);return -1.0;}
    if constexpr (debug) Debug.Log("Temperature sensor write command sent successfully.");
    sleepMs(50);

    status = connection.receiveData(rx, 2, ret_bytes);
    if (status != FT_OK) {Debug.Error("Temperature sensor read data status error: ", status);return -1.0;}
    if (ret_bytes < 2) {Debug.Error("Temperature sensor too few data bytes returned: ", ret_bytes);return -1.0;}
    if (!Utilities::validateAndLogData(rx, 2, "Temperature Sensor")) {return -1.0;}
    // Process temperature result (different from ADC - direct MSB/LSB)
    double temperature = convertToTemperature(rx[1], rx[0], false); // Celsius
    if constexpr (debug) Debug.Log("Read temperature: " + std::to_string(temperature) + " C");
    return temperature;
}

bool MiniXDevice::initializeGPIOs() {
    unsigned char tx[100]; FT_STATUS status;
    //initialize clock, data, and digital I/O on Low-Byte, 8-bit port 
    int pos=0;
    // set TS chip select low
    tx[pos++] = 0x82;//Setup MPSSE High byte I/O lines
    HighByteHiLowState = OUTPUTMODE_H;
    CLEAR(HighByteHiLowState, TSCS);  //take chip select high
    tx[pos++] = HighByteHiLowState; //1110 0011  hi/low state if output (clock & data init to high)
    tx[pos++] = OUTPUTMODE_H;
    tx[pos++] = 0x80;//Setup MPSSE Low byte I/O lines
    LowByteHiLowState = 0xFB;//Initialize all high 1111 1011
    //LowByteHiLowState = OUTPUTMODE;//Initialize 20071203
    CLEAR(LowByteHiLowState, CTRL_HV_EN_A); //HVEN A disabled
    CLEAR(LowByteHiLowState, CTRL_HV_EN_B); //HVEN B disabled
    tx[pos++] = LowByteHiLowState;
    tx[pos++] = OUTPUTMODE;//0xFB
    status = connection.sendData(tx, pos);
    if(status != FT_OK){printf("Error initializing I/O lines.\n");return false;}
    return true;
}

void MiniXDevice::startingParameters() {
    // Initialize MiniX parameters
    DefaultHighVoltage = 15.0;          // Default High Voltage kV
    HighVoltageMin = (double)9.999999;  // High Voltage Min
    HighVoltageMax = 40.0;            // High Voltage Max
    HighVoltageConversionFactor = 10.0;     // High Voltage Conversion Factor
    DefaultCurrent = 15.0;              // Default Current
    CurrentMin = (double)4.999999;
    CurrentMax = 200.0;             // Current Max
    CurrentConversionFactor = 50.0;       // Current Conversion Factor
    VRef = 4.096;           // Reference Voltage 
    DAC_ADC_Scale = 4096;           // DAC ADC Scale 
    WattageMax = 4.00;              // Wattage Max 
    SafetyMargin = 0.050;       // Safety Margin
    SafeWattageMW = (double)((WattageMax - SafetyMargin) * 1000.0);
    TemperatureMax = 50.0;         // Temperature Max C
    TemperatureMin = 0.0;          // Temperature Min C 
}

bool MiniXDevice::safetyChecks() {

    if (currentVoltage > HighVoltageMax || currentVoltage < HighVoltageMin) {
        Debug.Error("Voltage out of bounds: ", currentVoltage);
        return false;
    }

    if (currentCurrent > CurrentMax || currentCurrent < CurrentMin) {
        Debug.Error("Current out of bounds: ", currentCurrent);
        return false;
    }

    if (currentTemperature > TemperatureMax || currentTemperature < TemperatureMin) {
        Debug.Error("Temperature out of bounds: ", currentTemperature);
        return false;
    }

    return true;
}

void MiniXDevice::setClockDivisor(unsigned char* tx, int& pos, int clockDivisor) {
        tx[pos++] = CMD_SET_CLK_DIVISOR;
        tx[pos++] = clockDivisor & 0xFF;
        tx[pos++] = (clockDivisor >> 8) & 0xFF;
    }

double MiniXDevice::convertToVoltage(unsigned char rx0, unsigned char rx1, double VRef, double DAC_ADC_Scale, double HighVoltageConversionFactor) {
    // Extract 12-bit ADC value using original main.cpp bit manipulation
    unsigned char msb = rx0 >> 3;
    unsigned char lsb = ((rx0 & 0x07) << 5) | (rx1 >> 3);
    int ivolts = (msb << 8) | lsb;
    
    // Convert ADC value to voltage using the reference voltage and scale
    double voltage = (static_cast<double>(ivolts) / DAC_ADC_Scale * VRef) * HighVoltageConversionFactor;

    Debug.Log("MSB: " + std::to_string(msb) + ", LSB: " + std::to_string(lsb) + 
                ", ADC Value: " + std::to_string(ivolts) + 
                ", Converted Voltage: " + std::to_string(voltage) + "V");

    return voltage;
    }

double MiniXDevice::convertToCurrent(unsigned char rx0, unsigned char rx1, double VRef, double DAC_ADC_Scale, double CurrentConversionFactor) {
        // Extract 12-bit ADC value using original main.cpp bit manipulation
        unsigned char msb = rx0 >> 3;
        unsigned char lsb = ((rx0 & 0x07) << 5) | (rx1 >> 3);
        int ivolts = (msb << 8) | lsb;
        
        // Convert ADC value to voltage first, then to current
        double current = (static_cast<double>(ivolts) / DAC_ADC_Scale * VRef) * CurrentConversionFactor;

        Debug.Log("MSB: " + std::to_string(msb) + ", LSB: " + std::to_string(lsb) + 
                  ", ADC Value: " + std::to_string(ivolts) + 
                  ", Converted Current: " + std::to_string(current) + "µA");

        return current;
    }

double MiniXDevice::convertToTemperature(unsigned char MSB, unsigned char LSB, bool isF) {
        int tempRaw = (MSB << 4) + (LSB >> 4); double temp_c;
        if (MSB >= 0x80) { temp_c = static_cast<double>(tempRaw - 4096) * 0.0625; } 
        else { temp_c = static_cast<double>(tempRaw) * 0.0625; }
        double temperature = isF ? (temp_c * 9.0 / 5.0 + 32.0) : temp_c;
        // Debug.Log("MSB: " + std::to_string(MSB) + ", LSB: " + std::to_string(LSB) + ", Raw 12-bit: " + std::to_string(tempRaw) + ", Temperature: " + std::to_string(temperature) + (isF ? "°F" : "°C"));
        return temperature;
    }

void MiniXDevice::activateTemperatureSensor(unsigned char* tx, int& pos, unsigned char& HighByteHiLowState) {
        tx[pos++] = CMD_SET_DATA_BITS_HIGHBYTE;
        SET(HighByteHiLowState, TSCS); 
        tx[pos++] = HighByteHiLowState;
        tx[pos++] = OUTPUTMODE_H;
        // Debug.Log("Activated temperature sensor (TSCS low).");
}

void MiniXDevice::deactivateTemperatureSensor(unsigned char* tx, int& pos, unsigned char& HighByteHiLowState) {
        tx[pos++] = CMD_SET_DATA_BITS_HIGHBYTE;
        CLEAR(HighByteHiLowState, TSCS);
        tx[pos++] = HighByteHiLowState;
        tx[pos++] = OUTPUTMODE_H;
        // Debug.Log("Deactivated temperature sensor (TSCS high).");
    }

