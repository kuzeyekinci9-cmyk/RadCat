#include "DeviceHandler.hpp"
#include "Debug.hpp"
#include "ftd2xx.h"
#include "AllComponents.hpp"

void DeviceHandler::deviceLogicUpdate() {
    for (auto& device : activeDevices) {
        device->systemUpdate();
    }
}

// Scans for devices using all available handlers and attempts to match them to registered device types.
// Matched devices are instantiated and added to the activeDevices list. Yet they are not connected automatically.
// Neither their automation starts, they are waiting for explicit connect() calls by the UI. This only sets up the devices and UI entries.
void DeviceHandler::deviceScan() {
    ftdiScan();
    libUsbScan();
}

void DeviceHandler::libUsbScan() {
    if constexpr(debug) Debug.Log("Scanning for LibUsb devices...");

    std::vector<LibUsbHandler::ScannedDeviceInfo> scannedDevices = libUsbHandler.scanDevices();
    if (scannedDevices.empty()) { if constexpr(debug) Debug.Warn("No LibUsb devices found during scan."); return; }

    for(LibUsbHandler::ScannedDeviceInfo& info : scannedDevices) { // For each detected LibUsb device
        uint16_t vid = info.descriptor.idVendor;
        uint16_t pid = info.descriptor.idProduct;
        
        // Check active devices to see if already assigned
        bool alreadyAssigned = false;
        for (auto& device : activeDevices) {
            UsbConnection* usbComp = device->systemGetComponent<UsbConnection>();
            if(usbComp && usbComp->deviceInfo.busNumber == libusb_get_bus_number(info.device) && usbComp->deviceInfo.vid == vid && usbComp->deviceInfo.pid == pid) {
                if constexpr(debug) Debug.Log("LibUsb device VID: " , vid , " PID: " , pid , " is already assigned to an active device. Skipping.");
                alreadyAssigned = true;
                break;
            }
        }
        if (alreadyAssigned) continue;

        // Check all registered devices for potential LibUsb matches
        auto UsbDevices = DeviceRegistry::getRegisteredDevicesWithComponents<UsbConnection>();
        for (const auto& entry : UsbDevices) {
            const DeviceRegistry::RegistryEntry::DeviceInfo& deviceInfo = entry->deviceInfo;
            FoundDeviceInfo foundDevice;

            // VID Check
            if (deviceInfo.vid == vid){
                foundDevice.matchData.vidMatch = true;
                foundDevice.matchData.matchScore++;
            }

            // PID Check
            uint16_t pid = info.descriptor.idProduct;
            if (deviceInfo.pid == 0 && deviceInfo.pid == pid){
                foundDevice.matchData.pidMatch = true;
                foundDevice.matchData.matchScore++;
            }

            //TODO: Serial Number Check

            // Name Check TODO: upper lover case insensitive comparison 
            std::string foundDeviceName = deviceInfo.deviceName;
            if(foundDeviceName.find(deviceInfo.deviceName) != std::string::npos 
            || deviceInfo.deviceName.find(foundDeviceName) != std::string::npos){
                foundDevice.matchData.nameMatch = true;
                foundDevice.matchData.matchScore++;
            }

            if(foundDevice.matchData.matchScore <= 2) continue; //Not enough matches
            
            if constexpr(debug) Debug.Log("MATCH FOUND! Device : " , deviceInfo.deviceName);
            foundDevice.connectionType = FoundDeviceInfo::ConnectionType::LibUsb;
            foundDevice.deviceRegistryEntry = entry;
            foundDevice.LibUsbScannedDeviceInfo = std::make_unique<LibUsbHandler::ScannedDeviceInfo>(std::move(info));
            foundDevices.emplace_back(std::move(foundDevice));
            break;
        }
    }
}

void DeviceHandler::ftdiScan() {
    if constexpr(debug) Debug.Log("Scanning for FTDI devices...");
    
    std::vector<FTDIHandler::ScannedDeviceInfo> scannedDevices = ftdiHandler.scanDevices();
    if (scannedDevices.empty()) { if constexpr(debug) Debug.Warn("No FTDI devices found during scan."); return; }
    auto FTDIDevices = DeviceRegistry::getRegisteredDevicesWithComponents<FTDIConnection>();

    for (const FTDIHandler::ScannedDeviceInfo& scannedDevice : scannedDevices) {
        
        // Check active devices
        bool alreadyAssigned = false;
        for (auto& device : activeDevices) {
            auto* ftdiComp = device->systemGetComponent<FTDIConnection>();
            if (ftdiComp && ftdiComp->getFTDIIndex() == scannedDevice.scanIndex) {
                if constexpr(debug) Debug.Log("FTDI device at index " , scannedDevice.scanIndex , " is already assigned to an active device. Skipping.");
                alreadyAssigned = true;
                break;
            }
        }
        if (alreadyAssigned) continue;


        // Check all registered devices for potential FTDI matches
        bool isMatch = false;
        for (const auto& entry : FTDIDevices) {
            const DeviceRegistry::RegistryEntry::DeviceInfo& deviceInfo = entry->deviceInfo;
            FoundDeviceInfo foundDevice;
            
            // Serial Number Check
            if (entry->deviceInfo.serialNumber == std::string(scannedDevice.devInfo.SerialNumber)){
                foundDevice.matchData.serialMatch = true;
                foundDevice.matchData.matchScore++;
            }
            
            // Vid Pid Check
            uint16_t vid = (scannedDevice.devInfo.ID & 0xFFFF);
            uint16_t pid = ((scannedDevice.devInfo.ID >> 16) & 0xFFFF);
            if (entry->deviceInfo.vid != 0 && entry->deviceInfo.vid == vid) {
                foundDevice.matchData.vidMatch = true;
                foundDevice.matchData.matchScore++;
            }
            if (entry->deviceInfo.pid != 0 && entry->deviceInfo.pid == pid) {
                foundDevice.matchData.pidMatch = true;
                foundDevice.matchData.matchScore++;
            }
            
            std::string foundDeviceName = std::string(scannedDevice.devInfo.Description);
            std::string deviceNameLower = deviceInfo.deviceName;
            std::string foundDeviceNameLower = foundDeviceName;
            std::transform(deviceNameLower.begin(), deviceNameLower.end(), deviceNameLower.begin(), ::tolower);
            std::transform(foundDeviceNameLower.begin(), foundDeviceNameLower.end(), foundDeviceNameLower.begin(), ::tolower);
            if (foundDeviceNameLower.find(deviceNameLower) != std::string::npos || 
                deviceNameLower.find(foundDeviceNameLower) != std::string::npos){
                foundDevice.matchData.nameMatch = true;
                foundDevice.matchData.matchScore++;
            }
            
            if(foundDevice.matchData.matchScore <= 2) continue; //Not enough matches

            if constexpr(debug) Debug.Log("MATCH FOUND! Device: ", deviceInfo.deviceName);
            foundDevice.deviceRegistryEntry = entry;
            foundDevice.connectionType = FoundDeviceInfo::ConnectionType::FTDI;
            foundDevice.FTDIScannedDeviceInfo = std::make_unique<FTDIHandler::ScannedDeviceInfo>(scannedDevice);
            foundDevices.emplace_back(std::move(foundDevice));
            break;
        }
    }
}

void DeviceHandler::activateDevice(FoundDeviceInfo& DeviceInfo) {
    // Create device instance
    auto matchedDevice = DeviceInfo.deviceRegistryEntry->creator();

    if (DeviceInfo.connectionType == FoundDeviceInfo::ConnectionType::FTDI) {
        FTDIConnection* ftdiComp = matchedDevice->systemGetComponent<FTDIConnection>();
        
    } 
    else if (DeviceInfo.connectionType == FoundDeviceInfo::ConnectionType::LibUsb) {
        UsbConnection* usbComp = matchedDevice->systemGetComponent<UsbConnection>();

        if ( !libUsbHandler.deviceMatch(DeviceInfo.LibUsbScannedDeviceInfo, *usbComp) ) return; // Matching failed

    }



    activeDevices.push_back(std::move(matchedDevice));
}

