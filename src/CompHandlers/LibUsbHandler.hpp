#pragma once
#include "included/libusb.h"
#include "CompHandlers/BaseComponentHandler.hpp"
#include <vector>
#include <string>
#include "UsbConnection.hpp"
#include "debug.hpp"

class LibUsbHandler : public BaseComponentHandler {
public:
    constexpr static bool debug = true;
    static LibUsbHandler& Instance(){static LibUsbHandler instance; return instance;}
    libusb_context* getContext() { return ctx; }
    bool initialize() override;
    bool shutdown() override;

    struct ScannedDeviceInfo {
        libusb_device* device = nullptr;
        libusb_device_descriptor descriptor{};
        int scanIndex = 0;
        
        // No default construction, copy, or assignment,
        ScannedDeviceInfo() = delete;
        ScannedDeviceInfo(const ScannedDeviceInfo&) = delete;
        ScannedDeviceInfo& operator=(const ScannedDeviceInfo&) = delete;
        ~ScannedDeviceInfo() noexcept { if (device) libusb_unref_device(device); }
        // Custom constructor for initialization with ref count management
        explicit ScannedDeviceInfo(libusb_device& dev, const libusb_device_descriptor& desc, int index)
        : device(&dev), descriptor(desc), scanIndex(index) { libusb_ref_device(device); }
        // Move allowed
        ScannedDeviceInfo(ScannedDeviceInfo&& other) noexcept
        : device(other.device),
        descriptor(other.descriptor),
        scanIndex(other.scanIndex) {
            other.device = nullptr;
        }
        ScannedDeviceInfo& operator=(ScannedDeviceInfo&& other) noexcept {
            if (this != &other) {
                if (device) libusb_unref_device(device);
                device = other.device;
                descriptor = other.descriptor;
                scanIndex = other.scanIndex;
                other.device = nullptr;
            }
            return *this;
        }


    };

    //Libusb methods
    std::vector<ScannedDeviceInfo> scanDevices();
    bool attemptReinitialize();
    bool deviceMatch(std::unique_ptr<LibUsbHandler::ScannedDeviceInfo>& info, UsbConnection& usbComponent);
    bool openDevice(libusb_device_handle** handle, uint16_t vendorID, uint16_t productID);
    void closeDevice(libusb_device_handle* handle);
    int readData(libusb_device_handle* handle, unsigned char* data, int length, unsigned int timeout);
    int writeData(libusb_device_handle* handle, const unsigned char* data, int length, unsigned int timeout);

private:
    libusb_context* ctx = nullptr;
    LibUsbHandler(){
    int r = libusb_init(&ctx);
    if (r < 0) Debug.Error("Failed to initialize libusb: " , r);
    }
};