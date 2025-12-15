#include "FTDIHandler.hpp"
#include "Debug.hpp"
#include <thread>
#include <chrono>
#include "Utilities.hpp"
using namespace Utilities;


bool FTDIHandler::initialize() {
   
    return true;
}

bool FTDIHandler::shutdown() {
    
    return true;
}

// Direct Data Transfer (Non-thread-safe, Only For Main Systems To Use)
FT_STATUS FTDIHandler::sendData(FT_HANDLE deviceHandle, const unsigned char* data, DWORD size) {
    if (!data) { Debug.Error("FTDI sendData: null data pointer"); return FT_INVALID_PARAMETER; }
    if (size == 0) { Debug.Warn("FTDI sendData: zero size requested"); return FT_OK; }
    if (!deviceHandle) { Debug.Error("FTDI sendData: device not connected"); return FT_INVALID_HANDLE; }
    DWORD bytesWritten = 0;
    FT_STATUS ftStatus = FT_Write(deviceHandle, (LPVOID)data, size, &bytesWritten);
    if(ftStatus != FT_OK){ Debug.Error("FTDI Write Error: " + std::to_string(ftStatus)); }
    else if(bytesWritten != size) {Debug.Warn("FTDI sendData: requested " + std::to_string(size) + " bytes, but wrote " + std::to_string(bytesWritten) + " bytes.");}
    return ftStatus;
}

FT_STATUS FTDIHandler::receiveData(FT_HANDLE deviceHandle, unsigned char* buffer, DWORD size, DWORD& bytesRead) {
    bytesRead = 0;
    if (!buffer) { Debug.Error("FTDI receiveData: null buffer pointer"); return FT_INVALID_PARAMETER; }
    if (size == 0) { Debug.Warn("FTDI receiveData: zero size requested"); return FT_OK; }
    if (!deviceHandle) { Debug.Error("FTDI receiveData: device not connected"); return FT_INVALID_HANDLE; }
    FT_STATUS ftStatus = FT_Read(deviceHandle, buffer, size, &bytesRead);
    if (ftStatus != FT_OK) { Debug.Error("FTDI Read Error: " + std::to_string(ftStatus)); }
    else if (bytesRead == 0) { Debug.Warn("FTDI Read: no data available"); }
    else if constexpr (debug) { Debug.Log("FTDI received " + std::to_string(bytesRead) + " bytes"); }
    return ftStatus;
}

// DeviceSession Methods
std::shared_ptr<FTDIHandler::DeviceSession> FTDIHandler::getSession(FT_HANDLE handle, const FT_DEVICE_LIST_INFO_NODE& info) {
    if (!handle) return nullptr;
    std::lock_guard<std::mutex> lk(mapMutex);
    auto it = handleSyncMap.find(handle);
    if (it == handleSyncMap.end()) {
        SyncPair sp;
        sp.tx = std::make_shared<std::mutex>();
        sp.rx = std::make_shared<std::mutex>();
        auto newIt = handleSyncMap.emplace(handle, std::move(sp)).first;
        it = newIt;
    }
    return std::shared_ptr<DeviceSession>(new DeviceSession(handle, info, it->second.tx, it->second.rx));
}

FT_STATUS FTDIHandler::DeviceSession::send(const unsigned char* data, DWORD size) {
    if (!data) { Debug.Error("FTDI sendData: null data pointer"); return FT_INVALID_PARAMETER; }
    if (size == 0) { Debug.Warn("FTDI sendData: zero size requested"); return FT_OK; }
    if (!ftHandle) { Debug.Error("FTDI sendData: device not connected"); return FT_INVALID_HANDLE; }

    std::lock_guard<std::mutex> txLock(*txMutex);
    DWORD bytesWritten = 0;
    FT_STATUS ftStatus = FT_Write(ftHandle, (LPVOID)data, size, &bytesWritten);
    if(ftStatus != FT_OK){ Debug.Error("FTDI Write Error: " + std::to_string(ftStatus)); }
    else if(bytesWritten != size) {Debug.Warn("FTDI sendData: requested " + std::to_string(size) + " bytes, but wrote " + std::to_string(bytesWritten) + " bytes.");}
    return ftStatus;
}

FT_STATUS FTDIHandler::DeviceSession::receive(unsigned char* buffer, DWORD size, DWORD& bytesRead) {
    bytesRead = 0;
    if (!buffer) { Debug.Error("FTDI receiveData: null buffer pointer"); return FT_INVALID_PARAMETER; }
    if (size == 0) { Debug.Warn("FTDI receiveData: zero size requested"); return FT_OK; }
    if (!ftHandle) { Debug.Error("FTDI receiveData: device not connected"); return FT_INVALID_HANDLE; }

    std::lock_guard<std::mutex> rxLock(*rxMutex);
    FT_STATUS ftStatus = FT_Read(ftHandle, buffer, size, &bytesRead);
    if (ftStatus != FT_OK) { Debug.Error("FTDI Read Error: " + std::to_string(ftStatus)); }
    else if (bytesRead == 0) { Debug.Warn("FTDI Read: no data available"); }
    else if constexpr (FTDIHandler::debug) { Debug.Log("FTDI received " + std::to_string(bytesRead) + " bytes"); }
    return ftStatus;
}

bool FTDIHandler::DeviceSession::pollData(DWORD bytesToRead, DWORD& bytesRead, int timeoutMs) {
    bytesRead = 0;
    if (!ftHandle || bytesToRead == 0) { Debug.Error("PollData: Invalid handle or bytesToRead."); return false; }
    if (timeoutMs <= 0) { Debug.Error("PollData: timeout must be positive."); return false; }

    const int pollInterval = 20; // ms
    int elapsed = 0;
    DWORD rxBytes = 0;

    // Lock rx mutex for per-handle synchronization with receive()
    std::lock_guard<std::mutex> rxLock(*rxMutex);

    while (bytesRead < bytesToRead && elapsed < timeoutMs) {
        FT_STATUS st = FT_GetQueueStatus(ftHandle, &rxBytes);
        if (st != FT_OK) {Debug.Error("FTDI GetQueueStatus error: " + std::to_string(st)); return false;}
        if (rxBytes > 0) {
            bytesRead = rxBytes;
            if (bytesRead > bytesToRead) bytesRead = bytesToRead;
        }
        if (bytesRead >= bytesToRead) break;
        sleepMs(pollInterval);
        elapsed += pollInterval;
    }

    if (bytesRead < bytesToRead) {Debug.Warn("PollData: Timeout waiting for data. Requested: " + std::to_string(bytesToRead) + ", Received: " + std::to_string(bytesRead));
        return false;
    }

    if constexpr (FTDIHandler::debug) Debug.Log("PollData: Successfully polled " + std::to_string(bytesRead) + " bytes of data.");
    return true;
}

bool FTDIHandler::DeviceSession::openMPSSE() {
    switch (devInfo.Type) { // Devices that dont need MPSSE
        case FT_DEVICE_2232C:
        case FT_DEVICE_232R:
        case FT_DEVICE_AM:
        case FT_DEVICE_BM:
        case FT_DEVICE_100AX:
            return true;
    }

    if (!ftHandle) { Debug.Error("OpenMPSSE: invalid FT_HANDLE"); return false; }

    std::lock_guard<std::mutex> txLock(*txMutex);
    std::lock_guard<std::mutex> rxLock(*rxMutex);

    unsigned char tx[5]; unsigned char rx[5]; FT_STATUS status; DWORD ret_bytes;
    //Set USB Parameters and Latency Timer
    FT_SetUSBParameters(ftHandle, 65536, 65536); //Set USB request transfer sizes
    FT_SetLatencyTimer(ftHandle, 4); //4ms
    FT_SetTimeouts(ftHandle, 40, 40); //40ms read/write timeouts
    FT_SetFlowControl(ftHandle, FT_FLOW_RTS_CTS, 0, 0);
    sleepMs(20);

    status = FT_SetBitMode(ftHandle, 0x0, 0x02);  //enable MPSSE 
    if(status != FT_OK){Debug.Error("Failed to enable MPSSE: " + std::to_string(status)); return false;}
    sleepMs(20);
    FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);

    //Test MPSSE by sending command 0xAA and expecting response 0xFA 0xAA
    tx[0] = 0xAA;
    FT_Write(ftHandle, tx, 1, &ret_bytes);
    sleepMs(20);
    FT_Read(ftHandle, rx, 2, &ret_bytes);
    // Expect: 0xFA 0xAA back
    if (ret_bytes == 2 && rx[0] == 0xFA && rx[1] == 0xAA) { if constexpr (FTDIHandler::debug) Debug.Log("MPSSE ENGINE OK."); }
    else { Debug.Error("Unexpected response from MPSSE engine."); return false; }
    return true;
}

bool FTDIHandler::DeviceSession::connectionStatus() {
    if (!ftHandle) return false;
    DWORD rxBytes = 0;
    std::lock_guard<std::mutex> rxLock(*rxMutex);
    FT_STATUS status = FT_GetQueueStatus(ftHandle, &rxBytes);
    return (status == FT_OK);
}

int FTDIHandler::getDeviceCount() {
    if constexpr(debug) Debug.Log("FTDIHandler: Scanning for FTDI devices...");
    FT_STATUS status; DWORD numDevs;
    status = FT_CreateDeviceInfoList(&numDevs);
    if (status != FT_OK) {Debug.Error("Error getting device list: " , status); return -1;}
    if (numDevs == 0) {Debug.Warn("No FTDI devices found."); return 0;}
    if constexpr(debug) Debug.Log("Number of FTDI devices found: " , numDevs);
    return static_cast<int>(numDevs);
}

std::vector<FTDIHandler::ScannedDeviceInfo> FTDIHandler::scanDevices() {
    if constexpr(debug) Debug.Log("FTDIHandler: Scanning for FTDI devices...");
    FT_STATUS status; DWORD numDevs;

    status = FT_CreateDeviceInfoList(&numDevs);
    std::vector<ScannedDeviceInfo> scannedDevices;
    if (status != FT_OK) {Debug.Error("Error getting device list: " , status); return scannedDevices;}
    if (numDevs == 0) {Debug.Warn("No FTDI devices found."); return scannedDevices;}

    for (DWORD i = 0; i < numDevs; i++) {
        FT_DEVICE_LIST_INFO_NODE devInfo;
        status = FT_GetDeviceInfoDetail(i, &devInfo.Flags, &devInfo.Type, &devInfo.ID, &devInfo.LocId, devInfo.SerialNumber, devInfo.Description, &devInfo.ftHandle);
        if (status != FT_OK) {Debug.Error("Error getting device info for device " , i , ": " , status); continue;}
        scannedDevices.push_back({devInfo, static_cast<int>(i)});
        if constexpr(debug) Debug.Log("Found FTDI Device - Type: " , devInfo.Type , ", ID: " , devInfo.ID , ", Description: " , devInfo.Description);
    }
    
    return scannedDevices;
}