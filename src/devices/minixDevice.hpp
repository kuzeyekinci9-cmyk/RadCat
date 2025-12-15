#pragma once
#include "DeviceCore.hpp"
#include "FTDIConnection.hpp"

class MiniXDevice : public BaseDevice<FTDIConnection> {
public:
    static constexpr bool debug = false;
    MiniXDevice() : BaseDevice() { startingParameters(); setupTasks(); }
    static inline const DeviceRegistry::RegistryEntry::DeviceInfo deviceInfo = {"Mini-X"};

    // Implement virtual methods
    virtual bool connect() override;
    virtual bool disconnect() override;
    virtual void update() override;
    virtual void setupTasks() override;
    virtual double readValue(const std::string& parameter) override;
    virtual bool setValue(const std::string& parameter, double value) override;

    // Additional Interface Methods
    bool initialize();
    void setVoltage(double voltage);
    void setCurrent(double voltage);
    void setHVOnOff(bool on);

    // Hardware State Variables
    unsigned char LowByteHiLowState;
    unsigned char HighByteHiLowState;

private:
    bool initializeGPIOs();
    void startingParameters();
    double readVoltage();
    double readCurrent();
    double readTemperature();
    bool safetyChecks();
    bool setupTemperatureSensor();
    bool setupClockDivisor();
    void setClockDivisor(unsigned char* tx, int& pos, int clockDivisor = 3);

    // Conversion Utilities
    double convertToVoltage(unsigned char rx0, unsigned char rx1, double VRef = 4.096, double DAC_ADC_Scale = 4096.0, double HighVoltageConversionFactor = 10.0);
    double convertToCurrent(unsigned char rx0, unsigned char rx1, double VRef = 4.096, double DAC_ADC_Scale = 4096.0, double CurrentConversionFactor = 50.0);
    double convertToTemperature(unsigned char MSB, unsigned char LSB, bool isF = false);

    // Temperature sensor control
    void activateTemperatureSensor(unsigned char* tx, int& pos, unsigned char& HighByteHiLowState);
    void deactivateTemperatureSensor(unsigned char* tx, int& pos, unsigned char& HighByteHiLowState);

    // Minix Components
    FTDIConnection& connection = getComponentRef<FTDIConnection>();

    // Minix-specific variables
    bool hvOn = false;
    bool HVOffErr = false;
    bool HVSetErr = false;
    bool CurrentSetErr = false;

    // Minix Status Variables
    double currentVoltage;
    double currentCurrent;
    double currentTemperature;

    // Mini-X Configuration Parameters
    double DefaultHighVoltage;
    double HighVoltageMin;
    double HighVoltageMax;
    double TemperatureMax;
    double TemperatureMin;
    double HighVoltageConversionFactor;
    double DefaultCurrent;
    double CurrentMin;
    double CurrentMax;
    double CurrentConversionFactor; // controller to tube current conversion factor
    double VRef;					// DAC/ADC reference voltage 
    double DAC_ADC_Scale;			// DAC/ADC scale = 2^resolution bits wide
    double WattageMax;
    double SafetyMargin;
    double SafeWattageMW;

};