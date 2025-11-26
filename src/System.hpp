#pragma once
#include "udpHandler.hpp"

using namespace std;

class System{
public:
    // Core Handlers
    UDPHandler udpHandler;

    System() : udpHandler() { if(systemInitializor()) isRunning = true; }
    ~System(){}

    void run();
    void logic();
    void stop();
    bool systemInitializor();


    bool isRunning{false};
    float elapsedMS{0.0f};

};