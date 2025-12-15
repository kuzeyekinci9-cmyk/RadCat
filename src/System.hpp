#pragma once
#include "DeviceHandler.hpp"

using namespace std;

class System{
public:
    // Core Handlers
    DeviceHandler deviceHandler;

    System() : deviceHandler() { if(systemInitializor()) isRunning = true; }
    ~System(){}

    void run();
    void logic();
    void stop();
    bool systemInitializor();


    bool isRunning = false;
    float elapsedMS = 0.0f;

};