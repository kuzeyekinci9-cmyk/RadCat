#include "LibUsbHandler.hpp"
#include "debug.hpp"

struct DeviceListGuard {
    libusb_device** list;
    DeviceListGuard(libusb_device** l) : list(l) {}
    ~DeviceListGuard() { if (list) libusb_free_device_list(list, 1); }
};

LibUsbHandler::LibUsbHandler() {
    int r = libusb_init(&ctx);
    if (r < 0) {
        Debug.Error("Failed to initialize libusb: " , r);
    }
}

bool LibUsbHandler::initialize() {
    if (ctx) {
        if constexpr (debug) Debug.Log("LibUsbHandler initialized successfully.");
        return true;
    } else {
        if constexpr (debug) Debug.Error("LibUsbHandler initialization failed: context is null.");
        return false;
    }
}

bool LibUsbHandler::shutdown() {
    if (ctx) {
        libusb_exit(ctx);
        ctx = nullptr;
        Debug.Log("LibUsbHandler shut down successfully.");
        return true;
    } else {
        Debug.Warn("LibUsbHandler shutdown called, but context was already null.");
        return false;
    }
}

std::vector<ScannedDeviceInfo> LibUsbHandler::scanDevices() {
    if(!ctx) { Debug.Error("LibUsbHandler scanDevices called but context is null."); return std::vector<ScannedDeviceInfo>(); }
    libusb_device **list;
    ssize_t cnt = libusb_get_device_list(ctx, &list);
    if (cnt < 0) { Debug.Error("Error getting USB device list: " , cnt); libusb_free_device_list(list, 1); return std::vector<ScannedDeviceInfo>(); }
    DeviceListGuard guard(list);
    for (ssize_t i = 0; i < cnt; i++) {
        libusb_device* device = list[i];
        libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(device, &desc);
        if (r < 0) { Debug.Error("Failed to get device descriptor for device " , i , ": " , r); continue; }
        libusb_ref_device(device);
        scannedDevicesInfo.push_back({device, desc, static_cast<int>(i)});
        if constexpr (debug) Debug.Log("Found USB Device - VID: " ,  desc.idVendor, ", PID: " ,  desc.idProduct);
    }
    libusb_free_device_list(list, 1);
    return scannedDevicesInfo;
}

void LibUsbHandler::cleanScannedDeviceReferences() {
    for (auto& info : scannedDevicesInfo) {
        if (info.device) {
            libusb_unref_device(info.device);
            info.device = nullptr;
        }
    }
    scannedDevicesInfo.clear();
}

bool LibUsbHandler::deviceMatch(ScannedDeviceInfo info, UsbConnection& usbComponent) {
    usbComponent.deviceInfo.vid = info.descriptor.idVendor;
    usbComponent.deviceInfo.pid = info.descriptor.idProduct;
    usbComponent.deviceInfo.device = info.device;
    usbComponent.deviceInfo.busNumber = libusb_get_bus_number(info.device);
    libusb_ref_device(info.device);
    return true;
}