#include "System.hpp"
#include "Debug.hpp"
#include <stdlib.h>
#include <string>
#include <iostream>
using namespace std;

bool System::systemInitializor() {
        bool CurrentStatus = true;
        Debug.Log("==================================");
        Debug.Log("System Initializing...");

        //Initialize Data Handler
        Debug.Log("Initializing Data Handler...");

        Debug.Log("Initializing UDP Handler...");
        //if(udpHandler.start()){Debug.Log("UDP Handler Initialized Successfully.");} 
        //else {Debug.Error("UDP Handler Initialization Failed!"); CurrentStatus = false;}

        if (CurrentStatus){Debug.Log("All systems go!"); isRunning = true;}
        else{Debug.Error("System Initialization Failed!",5); isRunning = false;}
        return CurrentStatus;
}

void System::run(){
        
    }

void System::stop() {
        isRunning = false;
        //udpHandler.stop();
    }

void System::logic(){
        //Looped Logic

        }