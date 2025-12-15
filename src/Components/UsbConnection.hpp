#pragma once
#include <string>
#include <vector>
#include "ComponentCore.hpp"
#include "Included/libusb.h"


class LibUsbHandler;
struct LibUsbDeviceInfo {
    uint16_t vid;
    uint16_t pid;
    std::string serial;          // empty if none
    uint8_t busNumber;
    std::vector<uint8_t> portPath;
    libusb_device* device;       // stored with ref count
};

class UsbConnection : public BaseComponent {
public:
    UsbConnection(EmptyDevice* parentDevice) : BaseComponent(parentDevice) {}
    LibUsbDeviceInfo deviceInfo;
    ~UsbConnection() override = default;
    void initialize() override;
    void update() override;

    LibUsbHandler* handler;
private:
friend class LibUsbHandler;
libusb_device_handle* deviceHandle = nullptr;
};
