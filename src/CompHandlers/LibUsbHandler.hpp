#pragma once
#include "included/libusb.h"
#include "CompHandlers/BaseComponentHandler.hpp"
#include <vector>
#include <string>
#include "UsbConnection.hpp"

struct ScannedDeviceInfo {
    libusb_device* device;
    libusb_device_descriptor descriptor;
    int scanIndex;
};

class LibUsbHandler : public BaseComponentHandler {
public:
    constexpr static bool debug = true;
    static LibUsbHandler& Instance(){static LibUsbHandler instance; return instance;}
    libusb_context* getContext() { return ctx; }
    bool initialize() override;
    bool shutdown() override;

    //Libusb methods
    std::vector<ScannedDeviceInfo> scanDevices();
    std::vector<ScannedDeviceInfo> scannedDevicesInfo;
    void cleanScannedDeviceReferences();
    void Ref(libusb_device* device) { libusb_ref_device(device); }
    void Unref(libusb_device* device) { libusb_unref_device(device); }
    bool deviceMatch(ScannedDeviceInfo info, UsbConnection& usbComponent);
    

    bool openDevice(libusb_device_handle** handle, uint16_t vendorID, uint16_t productID);
    void closeDevice(libusb_device_handle* handle);
    int readData(libusb_device_handle* handle, unsigned char* data, int length, unsigned int timeout);
    int writeData(libusb_device_handle* handle, const unsigned char* data, int length, unsigned int timeout);


private:
    libusb_context* ctx = nullptr;
    LibUsbHandler();
};