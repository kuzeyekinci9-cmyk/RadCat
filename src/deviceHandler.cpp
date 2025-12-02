#include "deviceHandler.hpp"
#include "debug.hpp"
#include "ftd2xx.h"
#include "allComponents.hpp"
#include "FTDIHandler.hpp"
#include "LibUsbHandler.hpp"

void DeviceHandler::deviceLogicUpdate() {
    for (auto& device : activeDevices) {
        device->systemUpdate();
    }
}

void DeviceHandler::deviceScan() {
    ftdiScan();
    libUsbScan();
}

void DeviceHandler::libUsbScan() {
    if constexpr(debug) Debug.Log("Scanning for LibUsb devices...");
    LibUsbHandler& libUsbHandler = LibUsbHandler::Instance();

    std::vector<ScannedDeviceInfo> scannedDevices = libUsbHandler.scanDevices();
    if (scannedDevices.empty()) { if constexpr(debug) Debug.Warn("No LibUsb devices found during scan."); return; }
    auto UsbDevices = DeviceRegistry::getRegisteredDevicesWithComponents<UsbConnection>();

    for(const ScannedDeviceInfo info : scannedDevices) { // For each detected LibUsb device
        uint16_t vid = info.descriptor.idVendor;
        uint16_t pid = info.descriptor.idProduct;
        
        // Check active devices to see if already assigned
        for (auto& device : activeDevices) {
            auto* usbComp = device->systemGetComponent<UsbConnection>();
            if (usbComp) {
                if(usbComp->deviceInfo.busNumber == libusb_get_bus_number(info.device) && usbComp->deviceInfo.vid == vid && usbComp->deviceInfo.pid == pid) {
                    if constexpr(debug) Debug.Log("LibUsb device VID: " , vid , " PID: " , pid , " is already assigned to an active device. Skipping.");
                    continue;
                }
            }
        }

        // Check all registered devices for potential LibUsb matches
        // But check VID first bcs it's the easiest
        for (const auto& [deviceName, entry] : UsbDevices) {
            if (entry.deviceInfo.vid != vid) continue; 
            bool isMatch = false;

            // PID Check
            uint16_t pid = info.descriptor.idProduct;
            if (entry.deviceInfo.pid != 0 && entry.deviceInfo.pid == pid) { isMatch = true; }

            //TODO: Serial Number Check

            // Name Check
            //TODO: upper lover case insensitive comparison 
            std::string foundDeviceName = entry.deviceInfo.deviceName;
            if(foundDeviceName.find(deviceName) != std::string::npos || deviceName.find(foundDeviceName) != std::string::npos) isMatch = true;

            // Finalize Match
            if (isMatch) {
                if constexpr(debug) Debug.Log("MATCH FOUND! Device : " , deviceName);
                auto matchedDevice = entry.creator();
                auto* usbComp = matchedDevice->systemGetComponent<UsbConnection>();

                if(!libUsbHandler.deviceMatch(info, *usbComp)) Debug.Error("Failed to match device " + deviceName);

                activeDevices.push_back(std::move(matchedDevice));
                break;
            }
        }
    }
    libUsbHandler.cleanScannedDeviceReferences();
}

void DeviceHandler::ftdiScan() {
    if constexpr(debug) Debug.Log("Scanning for FTDI devices...");
    FTDIHandler& ftdiHandler = FTDIHandler::Instance();
    FT_STATUS status;

    int ConnectedFTDIDevices = ftdiHandler.getDeviceCount();
    if (ConnectedFTDIDevices <= 0) { if constexpr(debug) Debug.Warn("No FTDI devices found during scan."); return; }
    auto FTDIDevices = DeviceRegistry::getRegisteredDevicesWithComponents<FTDIConnection>();

    for (int i = 0; i < ConnectedFTDIDevices; i++) { // For each detected FTDI device
        for (auto& device : activeDevices) { // Check if already assigned
            auto* ftdiComp = device->systemGetComponent<FTDIConnection>();
            if (ftdiComp && ftdiComp->getFTDIIndex() == static_cast<int>(i)) {
                if constexpr(debug) Debug.Log("FTDI device at index " , i , " is already assigned to an active device. Skipping.");
                continue;
            }
        }

        FT_DEVICE_LIST_INFO_NODE devInfo;
        status = FT_GetDeviceInfoDetail(i, &devInfo.Flags, &devInfo.Type, &devInfo.ID, &devInfo.LocId, devInfo.SerialNumber, devInfo.Description, &devInfo.ftHandle);
        if (status != FT_OK) {Debug.Error("Def inf fail " , i , " : " , status); continue;}
        if constexpr(debug) Debug.Log("Found FTDI device: " + std::string(devInfo.Description) + " (Serial: " + std::string(devInfo.SerialNumber) + ")");

        // Check all registered devices for potential FTDI matches
        for (const auto& [deviceName, entry] : FTDIDevices) {
            if constexpr(debug) Debug.Log("Checking registered device: " + deviceName + " against FTDI device: " + std::string(devInfo.Description));
            bool isMatch = false;

            if (entry.deviceInfo.serialNumber == std::string(devInfo.SerialNumber)) { isMatch = true; } //Serial Check
            
            std::string foundDeviceName = std::string(devInfo.Description); //Name Check
            std::string foundSerial = std::string(devInfo.SerialNumber);
            std::string deviceNameLower = deviceName;
            std::string foundDeviceNameLower = foundDeviceName;
            std::transform(deviceNameLower.begin(), deviceNameLower.end(), deviceNameLower.begin(), ::tolower);
            std::transform(foundDeviceNameLower.begin(), foundDeviceNameLower.end(), foundDeviceNameLower.begin(), ::tolower);
            if (foundDeviceNameLower.find(deviceNameLower) != std::string::npos || 
                deviceNameLower.find(foundDeviceNameLower) != std::string::npos) isMatch = true;
            
            if (isMatch) { 
                if constexpr(debug) Debug.Log("MATCH FOUND! Device '", deviceName , "' matches FTDI device '", foundDeviceName);
                auto matchedDevice = entry.creator();
                auto* ftdiComp = matchedDevice->systemGetComponent<FTDIConnection>();
                ftdiComp->setFTDIIndex(static_cast<int>(i));
                ftdiComp->setDevInfo(devInfo);
                activeDevices.push_back(std::move(matchedDevice));
                break;
            }
        }
    }
}