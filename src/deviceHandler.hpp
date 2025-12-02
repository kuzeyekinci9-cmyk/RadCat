#pragma once
#include <vector>
#include <memory>
#include "deviceCore.hpp"


class DeviceHandler {
public:
    std::vector<std::unique_ptr<EmptyDevice>> activeDevices;
    const static bool debug = false;


    void ftdiScan();
    void libUsbScan();
    void deviceScan();
    void deviceLogicUpdate();
    
};