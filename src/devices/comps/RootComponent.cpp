#include "RootComponent.hpp"
#include "deviceCore.hpp"
#include "TDirectory.h"
#include "TFile.h"


void RootComponent::initialize() {
    dirName = "Device_" + parent->deviceInfo.deviceName;
    rootFile = new TFile(dirName.c_str(), "RECREATE");
    rootDir = rootFile->mkdir(dirName.c_str());
}