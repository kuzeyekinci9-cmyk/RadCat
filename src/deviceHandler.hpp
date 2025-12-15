#pragma once
#include <vector>
#include <memory>
#include "DeviceCore.hpp"
#include "FTDIHandler.hpp"
#include "LibUsbHandler.hpp"


class DeviceHandler {
public:

    struct FoundDeviceInfoForUI {
        bool serialMatch = false;
        bool nameMatch = false;
        bool vidMatch = false;
        bool pidMatch = false;
        bool modified = false;
        uint8_t matchScore = 0;
    };

    struct FoundDeviceInfo {
        enum class ConnectionType {
            FTDI,
            LibUsb,
            Other
        } connectionType;
        const DeviceRegistry::RegistryEntry* deviceRegistryEntry;

        //Matched data types
        FoundDeviceInfoForUI matchData;

        //LibUsb Data
        std::unique_ptr<LibUsbHandler::ScannedDeviceInfo> LibUsbScannedDeviceInfo = nullptr;

        //FTDI Data

    };


    std::vector<FoundDeviceInfo> foundDevices;
    std::vector<std::unique_ptr<EmptyDevice>> activeDevices;

    
    const static bool debug = false;

    void deviceScan();
    void deviceLogicUpdate();

    void activateDevice(FoundDeviceInfo& DeviceInfo);
    

private:
    LibUsbHandler& libUsbHandler = LibUsbHandler::Instance();
    FTDIHandler& ftdiHandler = FTDIHandler::Instance();
    void ftdiScan();
    void libUsbScan();
    
};