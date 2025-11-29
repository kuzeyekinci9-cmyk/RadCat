#include "FTDIConnection.hpp"
#include "debug.hpp"
#include "Utilities.hpp"
#include "deviceCore.hpp"

using namespace Utilities;

bool FTDIConnection::fConnect() {
    if (connected) return true;
    tryingToConnect = true;

    if(!openDevice()) { tryingToConnect = false; connected = false; return false; }
    if(!session) { Debug.Error("FTDI fConnect: No valid session available."); tryingToConnect = false; connected = false; return false; }
    if(!(session->openMPSSE())) { tryingToConnect = false; connected = false; return false; }

    connected = true; tryingToConnect = false; return true; // Successfully connected
}

bool FTDIConnection::fDisconnect() {
    if (!connected && !deviceIsOpen) return true;
    if(!closeDevice()) return false;
    connected = false;
    deviceIsOpen = false;
    if constexpr (debug) Debug.Log("FTDI device disconnected successfully.");
    return true;
}

bool FTDIConnection::closeDevice(){
    if (!deviceIsOpen) return true;
    FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
    FT_ResetDevice(ftHandle);
    FT_STATUS status = FT_Close(ftHandle);
    if (status != FT_OK) { Debug.Error("Failed to close FTDI device: " + std::to_string(status)); return false; }
    deviceIsOpen = false;
    ftHandle = nullptr;
    if constexpr (debug) Debug.Log("FTDI device closed successfully.");
    return true;
}

bool FTDIConnection::openDevice(){
    if (deviceIsOpen) return true;

    ftStatus = FT_Open(FTDIIndex, &ftHandle);
    if (ftStatus != FT_OK) {
        Debug.Error("Failed to open FTDI device: "+ std::to_string(FTDIIndex) + ", " + std::to_string(ftStatus));
        tryingToConnect = false; connected = false; return false;
    }

    deviceIsOpen = true;
    if constexpr (debug) Debug.Log("FTDI device opened successfully.");
    session = FTDIHandler::instance().getSession(ftHandle, devInfo);
    FT_ResetDevice(ftHandle); // Reset device to ensure clean state
    FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX); // Clear RX and TX buffers
    sleepMs(100); // Wait for device to stabilize
    return true;
}

FT_STATUS FTDIConnection::sendData(const unsigned char* data, DWORD size){
    if (session) return session->send(data, size);
    Debug.Error("FTDI sendData: No valid session available."); return FT_INVALID_HANDLE;
}

FT_STATUS FTDIConnection::receiveData(unsigned char* buffer, DWORD size, DWORD& bytesRead){
    if (session) return session->receive(buffer, size, bytesRead);
    Debug.Error("FTDI receiveData: No valid session available."); return FT_INVALID_HANDLE;
}

bool FTDIConnection::PollData(DWORD bytesToRead, DWORD& bytesRead, int timeoutMs) {
    if (session) return session->pollData(bytesToRead, bytesRead, timeoutMs);
    Debug.Error("FTDI PollData: No valid session available."); return false;
}

void FTDIConnection::setup() {
    myDeviceName = parent->deviceInfo.deviceName;
}