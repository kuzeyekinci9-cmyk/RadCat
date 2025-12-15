#include "LibUsbHandler.hpp"


bool LibUsbHandler::initialize() {
    if (ctx) {
        if constexpr (debug) Debug.Log("LibUsbHandler initialized successfully.");
        return true;
    }

    Debug.Error("LibUsbHandler initialization failed: context is null.");
    return false;
}

bool LibUsbHandler::shutdown() {
    if (ctx) {
        libusb_exit(ctx);
        ctx = nullptr;
        if constexpr (debug) Debug.Log("LibUsbHandler shut down successfully.");
        return true;
    } else {
        Debug.Warn("LibUsbHandler shutdown called, but context was already null.");
        return false;
    }
}

std::vector<LibUsbHandler::ScannedDeviceInfo> LibUsbHandler::scanDevices() {
    if (!ctx) if (!attemptReinitialize()){Debug.Error("LibUsbHandler scanDevices called but context is null after re-initialization."); return {}; }

    libusb_device **list;
    ssize_t cnt = libusb_get_device_list(ctx, &list);
    if (cnt < 0) { 
    Debug.Error("Error getting USB device list: " , cnt); 
    libusb_free_device_list(list, 1); 
    return {};
    }
    std::vector<ScannedDeviceInfo> scannedDevicesInfo;
    scannedDevicesInfo.reserve(static_cast<size_t>(cnt));
    for (ssize_t i = 0; i < cnt; i++) {
        libusb_device* device = list[i];
        libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(device, &desc);
        if (r < 0) { Debug.Error("Failed to get device descriptor for device " , i , ": " , r); continue; }
        scannedDevicesInfo.emplace_back(*device, desc, static_cast<int>(i));
        if constexpr (debug) Debug.Log("Found USB Device - VID: " ,  desc.idVendor, ", PID: " ,  desc.idProduct);
    }
    libusb_free_device_list(list, 1);
    return scannedDevicesInfo;
}

bool LibUsbHandler::deviceMatch(std::unique_ptr<LibUsbHandler::ScannedDeviceInfo>& info, UsbConnection& usbComponent) {
    if (!info) {
        Debug.Error("LibUsbHandler::deviceMatch called with null ScannedDeviceInfo.");
        return false;
    }
    usbComponent.deviceInfo.vid = info->descriptor.idVendor;
    usbComponent.deviceInfo.pid = info->descriptor.idProduct;
    usbComponent.deviceInfo.device = info->device;
    usbComponent.deviceInfo.busNumber = libusb_get_bus_number(info->device);
    int r = libusb_open(info->device, &usbComponent.deviceHandle);
    if (r < 0) { Debug.Error("Failed to open USB device: " , r); return false; }
    return true;
}

bool LibUsbHandler::attemptReinitialize() {
    Debug.Warn("LibUsbHandler scanDevices called but context is null. Attempting re-initialization...");
    if (ctx) {
        Debug.Log("LibUsbHandler context is already initialized.");
        return true;
    }
    int r = libusb_init(&ctx);
    if (r < 0) {
        Debug.Error("Failed to re-initialize libusb: " , r);
        return false;
    }
    Debug.Log("LibUsbHandler re-initialized successfully.");
    return true;
}