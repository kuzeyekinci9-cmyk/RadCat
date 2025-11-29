#pragma once
#include <ftd2xx.h>
#include <string>
#include "componentCore.hpp"
#include "FTDIHandler.hpp"

COMPONENT class FTDIConnection : public BaseComponent {
public:
    template<typename DeviceType> FTDIConnection(DeviceType& parentDevice) : BaseComponent(&parentDevice), devInfo{} {setup();}

    //Interface Methods
    FT_STATUS sendData(const unsigned char* data, DWORD size);
    FT_STATUS receiveData(unsigned char* buffer, DWORD size, DWORD& bytesRead);
    bool PollData(DWORD bytesToRead, DWORD& bytesRead, int timeoutMs);
    bool fConnect();
    bool fDisconnect();

    //Getters and Setters
    void setDevInfo(const FT_DEVICE_LIST_INFO_NODE& info) { devInfo = info; }
    int getFTDIIndex() const { return FTDIIndex; }
    FT_DEVICE_LIST_INFO_NODE getDevInfo() const { return devInfo; }
    bool isConnected() const { return connected; }
    bool isDeviceOpen() const { return deviceIsOpen; }
    bool isMPSSEOn() const { return setupDone; }
    bool isTryingToConnect() const { return tryingToConnect; }

private:
    friend class FTDIHandler;
    friend class DeviceHandler;
    FTDIHandler& handler = FTDIHandler::instance();
    std::shared_ptr<FTDIHandler::DeviceSession> session;
    FT_DEVICE_LIST_INFO_NODE devInfo;
    static constexpr bool debug = false; //Debug flag
    FT_HANDLE ftHandle = nullptr;
    FT_STATUS ftStatus = FT_OK;
    DWORD bytesWritten = 0;
    DWORD bytesRead = 0;
    std::string myDeviceName = "";
    int FTDIIndex = -1;
    bool setupDone = false;
    bool connected = false;
    bool tryingToConnect = false;
    bool deviceIsOpen = false;
    bool openDevice();
    bool closeDevice();
    void setup();
    void setFTDIIndex(int index) { FTDIIndex = index; }

};