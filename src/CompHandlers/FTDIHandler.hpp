#pragma once
#include "BaseComponentHandler.hpp"
#include <ftd2xx.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

class FTDIHandler : public BaseComponentHandler {
public:
    constexpr static bool debug = false;

    struct ScannedDeviceInfo {
        FT_DEVICE_LIST_INFO_NODE devInfo;
        int scanIndex;
    };
    
    bool initialize() override;
    bool shutdown() override;
	static FTDIHandler& Instance() {static FTDIHandler s_instance; return s_instance;} // Singleton Instance

    // ---- FTDI Methods ----
    std::vector<ScannedDeviceInfo> scanDevices();
    int getDeviceCount();

    class DeviceSession {
    public:
        FT_HANDLE handle() const { return ftHandle; }
        // Per -device synchronized methods
        FT_STATUS send(const unsigned char* data, DWORD size);
        FT_STATUS receive(unsigned char* buffer, DWORD size, DWORD& bytesRead);
        bool connectionStatus();
        bool pollData(DWORD bytesToRead, DWORD& bytesRead, int timeoutMs);
        bool openMPSSE();

    private:
        friend class FTDIHandler;
        DeviceSession(FT_HANDLE h,
                      const FT_DEVICE_LIST_INFO_NODE& info,
                      std::shared_ptr<std::mutex> tx,
                      std::shared_ptr<std::mutex> rx)
            : ftHandle(h), devInfo(info), txMutex(std::move(tx)), rxMutex(std::move(rx)) {
                if (!txMutex) txMutex = std::make_shared<std::mutex>();
                if (!rxMutex) rxMutex = std::make_shared<std::mutex>();
            }
        FT_HANDLE ftHandle;
        FT_DEVICE_LIST_INFO_NODE devInfo;
        std::shared_ptr<std::mutex> txMutex;
        std::shared_ptr<std::mutex> rxMutex;
    };
    std::shared_ptr<DeviceSession> getSession(FT_HANDLE handle, const FT_DEVICE_LIST_INFO_NODE& info);

private:
	FTDIHandler() = default;
	~FTDIHandler() = default;
	FTDIHandler(const FTDIHandler&) = delete;
	FTDIHandler& operator=(const FTDIHandler&) = delete;
	FTDIHandler(FTDIHandler&&) = delete;
	FTDIHandler& operator=(FTDIHandler&&) = delete;



    // Direct Data Transfer (Non-thread-safe, Only For Main Systems To Use)
    FT_STATUS sendData(FT_HANDLE deviceHandle, const unsigned char* data, DWORD size);
    FT_STATUS receiveData(FT_HANDLE deviceHandle, unsigned char* buffer, DWORD size, DWORD& bytesRead);

    std::mutex mapMutex;
    struct SyncPair { std::shared_ptr<std::mutex> tx, rx; };
    std::unordered_map<FT_HANDLE, SyncPair> handleSyncMap;
};
