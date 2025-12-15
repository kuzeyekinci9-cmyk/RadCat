#pragma once
#include <string>
#include <tuple>
#include <map>
#include <memory>
#include <functional>
#include <vector>
#include <typeindex>
#include <algorithm>


// Forward declaration
class EmptyDevice;

class DeviceRegistry {
public:
    struct RegistryEntry {
        std::function<std::unique_ptr<EmptyDevice>()> creator;
        std::vector<std::type_index> componentTypes;
        struct DeviceInfo {
            std::string deviceName = "Unset";
            uint16_t vid = 0;
            uint16_t pid = 0;
            std::string serialNumber = "Unset";
            std::string model = "Unset";
            std::string firmwareVersion = "Unset";
        } deviceInfo;
    };

    static std::map<std::string, RegistryEntry>& registry() {
        static std::map<std::string, RegistryEntry> instance;
        return instance;
    }

    static std::unique_ptr<EmptyDevice> createFromName(const std::string& deviceName) {
        auto it = registry().find(deviceName);
        if (it != registry().end()) { return it->second.creator(); }
        return nullptr;
    }

    static bool isKnownDevice(const std::string& deviceName) { return registry().find(deviceName) != registry().end(); }

    static std::vector<std::string> getRegisteredDeviceNames() {
        std::vector<std::string> names;
        for (const auto& [name, entry] : registry()) { names.push_back(name); }
        return names;
    }

    static DeviceRegistry::RegistryEntry::DeviceInfo getDeviceInfo(const std::string& deviceName) {
        auto it = registry().find(deviceName);
        if (it != registry().end()) {
            return it->second.deviceInfo;
        }
        return {};
    }

    static std::vector<std::pair<std::string, DeviceRegistry::RegistryEntry::DeviceInfo>> getAllDeviceInfo() {
        std::vector<std::pair<std::string, DeviceRegistry::RegistryEntry::DeviceInfo>> result;
        for (const auto& [name, entry] : registry()) {
            result.emplace_back(name, entry.deviceInfo);
        }
        return result;
    }

    template<typename... QueryComponents>
    static std::vector<std::string> getRegisteredDeviceNamesWithComponents() {
        std::vector<std::string> out;
        std::vector<std::type_index> needed = { std::type_index(typeid(QueryComponents))... };
        for (const auto& [name, entry] : registry()) {
            bool allFound = true;
            for (const auto& need : needed) {
                if (std::find(entry.componentTypes.begin(), entry.componentTypes.end(), need) == entry.componentTypes.end()) {
                    allFound = false;
                    break;
                }
            }
            if (allFound) out.push_back(name);
        }
        return out;
    }

    template<typename... QueryComponents> 
    static std::vector<const RegistryEntry*> getRegisteredDevicesWithComponents() {
        std::vector<const RegistryEntry*> out;
        std::vector<std::type_index> needed = { std::type_index(typeid(QueryComponents))... };
        for (const auto& [name, entry] : registry()) {
            bool allFound = true;
            for (const auto& need : needed) {
                if (std::find(entry.componentTypes.begin(), entry.componentTypes.end(), need) == entry.componentTypes.end()) {
                    allFound = false;
                    break;
                }
            }
            if (allFound) out.emplace_back(&entry);
        }
        return out;
    }

};