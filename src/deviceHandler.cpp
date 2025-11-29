#include "deviceHandler.hpp"
#include "debug.hpp"
#include "ftd2xx.h"
#include "allComponents.hpp"

void DeviceHandler::deviceLogicUpdate() {
    for (auto& device : activeDevices) {
        device->systemUpdate();
    }
}

int DeviceHandler::FTDIScan() {
    if constexpr(debug) Debug.Log("Scanning for FTDI devices...");
    FT_STATUS status; DWORD numDevs;

    status = FT_CreateDeviceInfoList(&numDevs);
    if (status != FT_OK) {Debug.Error("Error getting device list: " , status); return -1;}
    if (numDevs == 0) {Debug.Warn("No FTDI devices found."); return 0;}
    if constexpr(debug) Debug.Log("Number of FTDI devices found: " + std::to_string(numDevs));

    auto FTDIDevices = DeviceRegistry::getRegisteredDevicesWithComponents<FTDIConnection>();
    for (DWORD i = 0; i < numDevs; i++) {

        for (auto& device : activeDevices) {
            auto* ftdiComp = device->systemGetComponent<FTDIConnection>();
            if (ftdiComp && ftdiComp->getFTDIIndex() == static_cast<int>(i)) {
                if constexpr(debug) Debug.Log("FTDI device at index " + std::to_string(i) + " is already assigned to an active device. Skipping.");
                continue;
            }
        }

        FT_DEVICE_LIST_INFO_NODE devInfo;
        status = FT_GetDeviceInfoDetail(i, &devInfo.Flags, &devInfo.Type, &devInfo.ID, &devInfo.LocId, devInfo.SerialNumber, devInfo.Description, &devInfo.ftHandle);
        if (status != FT_OK) {Debug.Error("Def inf fail " + std::to_string(i) + ": " , status); continue;}
        if constexpr(debug) Debug.Log("Found FTDI device: " + std::string(devInfo.Description) + " (Serial: " + std::string(devInfo.SerialNumber) + ")");

        // Check all registered devices for potential FTDI matches
        for (const auto& [deviceName, entry] : FTDIDevices) {
            if constexpr(debug) Debug.Log("Checking registered device: " + deviceName + " against FTDI device: " + std::string(devInfo.Description));
            bool isMatch = false;

            if (entry.deviceInfo.serialNumber == std::string(devInfo.SerialNumber)) { //Serial Number Check
                if constexpr(debug) Debug.Log("MATCH FOUND by Serial! Device '" + entry.deviceInfo.deviceName + "' matches FTDI device '" + std::string(devInfo.Description) + "' (Serial: " + std::string(devInfo.SerialNumber) + ")");
                isMatch = true;
            }
            
            std::string foundDeviceName = std::string(devInfo.Description); //Name Check
            std::string foundSerial = std::string(devInfo.SerialNumber);
            std::string deviceNameLower = deviceName;
            std::string foundDeviceNameLower = foundDeviceName;
            std::transform(deviceNameLower.begin(), deviceNameLower.end(), deviceNameLower.begin(), ::tolower);
            std::transform(foundDeviceNameLower.begin(), foundDeviceNameLower.end(), foundDeviceNameLower.begin(), ::tolower);
            if (foundDeviceNameLower.find(deviceNameLower) != std::string::npos || 
                deviceNameLower.find(foundDeviceNameLower) != std::string::npos) isMatch = true;
            
            if (isMatch) { if constexpr(debug) Debug.Log("MATCH FOUND! Device '" + deviceName + "' matches FTDI device '" + foundDeviceName + "' (Serial: " + foundSerial + ")");
                auto matchedDevice = entry.creator();
                auto* ftdiComp = matchedDevice->systemGetComponent<FTDIConnection>();
                if (ftdiComp == nullptr) {
                    Debug.Error("FTDIConnection component not found in device created for " + deviceName + ", this is a fatal error. Restart the program.");
                    continue;
                }
                ftdiComp->setFTDIIndex(static_cast<int>(i));
                ftdiComp->setDevInfo(devInfo);
                activeDevices.push_back(std::move(matchedDevice));
                break;
            }
        }
    }
    return numDevs;
}