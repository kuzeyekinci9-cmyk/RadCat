#pragma once
#include <string>
#include <tuple>
#include <memory>
#include <functional>
#include <vector>
#include <typeindex>
#include <algorithm>
#include <chrono>
#include "deviceRegistry.hpp"
#include "deviceHandler.hpp"


// Device registration macro REGISTER_DEVICE(class, "Name")
// Usage: Place this macro in the cpp file of your device implementation to register it.
// Without this, the device will not be recognized by the system. Thus its imperative to include it in the build.
#define REGISTER_DEVICE(TYPE, NAME) \
    static inline bool registered_##TYPE = [](){ \
        DeviceRegistry::registry()[NAME] = { [](){ return std::make_unique<TYPE>(); }, \
            BaseDevice<>::tuple_types<decltype(std::declval<TYPE>().components)>::get(), \
            TYPE::deviceInfo }; \
        return true; \
    }();





// Polymorphic base for all devices. Can not inherit directly.
// This class is for system use only. Use BaseDevice<T...> instead.
class EmptyDevice {
public:
    virtual ~EmptyDevice() = default;

    // Device Info (to be filled by derived classes)
    static inline const DeviceRegistry::RegistryEntry::DeviceInfo deviceInfo;

    // This function is called by the system whenever a device found in scan is to be connected.
    virtual bool connect() = 0;

    // This function is called by the system whenever a device is to be disconnected.
    virtual bool disconnect() = 0;

    // This function is called by the system after updating all components.
    virtual void update(){}

    // This function is called by the system to read a parameter from the device.
    virtual double readValue(const std::string& parameter) = 0;

    // This function is called by the system to set a parameter on the device.
    virtual bool setValue(const std::string& parameter, double value) = 0;

    // Placeholder function for setting up periodic tasks inside the device. This is not mandatory to implement.
    // But if used, its a better place to setup periodic tasks. You can set up your tasks in constructor or initalize as well.
    // However, this function is called once the device is fully constructed and all components are initialized which is safer.
    virtual void setupTasks(){}

    // Indicates whether there are active periodic tasks. If set to false, the task scheduler is skipped for performance.
    bool tasksActive = false;

    // Indicates whether the device has been initialized (connected successfully)
    // This flag should be set to true once the device has successfully connected and is ready for operation.
    bool isInitialized = false;

    // Component Access For Systems and Handlers (Not For Device Use). 
    // As a device programmer, if you need component access inside device, use getComponentRef<T>() instead of this.
    template<typename T> T* systemGetComponent() { return static_cast<T*>(baseGetComponent(typeid(T))); }

    // Constant component Access For Systems and Handlers (Not For Device Use). 
    // As a device programmer, if you need component access inside device, use const getComponentRef<T>() instead of this.
    template<typename T> const T* systemGetComponent() const { return static_cast<const T*>(baseGetComponent(typeid(T))); }

protected:
    virtual void* baseGetComponent(const std::type_info& ti) = 0;
    virtual const void* baseGetComponent(const std::type_info& ti) const = 0;

private:
    friend class DeviceHandler;


// Some parts should only be accessible to BaseDevice<T...> and not to its derivatives.
// This private section is for that purpose.
private:
    template<typename... Components> friend class BaseDevice;

    // Protected constructor to prevent direct instantiation
    EmptyDevice() {}

    // This function is called by the system every logic cycle.
    // Not for device programmer use. Use update() instead.
    virtual void systemUpdate() = 0;
};


// Struct for periodic tasks
struct PeriodicTask {
    std::chrono::steady_clock::time_point nextUpdate;
    int intervalMs;
    std::function<void()> task;
};



// Templated base device class. Implements component management and periodic tasks.
// Derive your device classes from this, specifying the component types as template parameters.
// Do Not derive directly from EmptyDevice! It will lead to missing functionality.
// Example: class MyDevice : public BaseDevice<CompA, CompB, CompC> { ... };
template<typename... Components> class BaseDevice : public EmptyDevice {
public:
    // Constructor of BaseDevice automatically initializes all components and sets up parent references.
    BaseDevice() : components(Components(*this)...) { }
    virtual ~BaseDevice() { }

    // Array of all components. As a device programmer, please do not access this directly. Use getComponentRef<T>() instead.
    // Never add or remove components at runtime. The component list is fixed at compile time.
    // Modifying the component list at runtime will lead to undefined behavior.
    std::tuple<Components...> components;

    // Get all components as a tuple (for advanced use cases)
    std::tuple<Components...>& getAllComponents() { return components; }

    // Get all components as a tuple (for advanced use cases)
    const std::tuple<Components...>& getAllComponents() const { return components; }

    // Get reference to specific component type T, where T is one of the component types specified in the template parameters.
    // This is the safe and preferred way to access components within device code.
    //
    // Throws a compile-time error if T is not part of the component list.
    // Do not use dynamic casting or typeid for component access within device code.
    //
    // Good Example: auto& myComp = getComponentRef<MyComponentType>();
    //
    // Bad Example: auto* myComp = static_cast<MyComponentType*>(getComponent(typeid(MyComponentType)));
    template<typename T> T& getComponentRef() { return std::get<T>(components); }

    // Constant version of getComponentRef()
    template<typename T> const T& getComponentRef() const { return std::get<T>(components); }

    // Helper to extract component type_index list from a tuple type
    template<typename Tuple> struct tuple_types;
    template<typename... Ts> struct tuple_types<std::tuple<Ts...>> {
        static std::vector<std::type_index> get() { return { std::type_index(typeid(Ts))... }; }
    };

    // Add a periodic task to be executed every intervalMs milliseconds.
    void addTask(std::function<void()> func, int intervalMs) {
        PeriodicTask t;
        t.intervalMs = intervalMs;
        t.task = func;
        t.nextUpdate = std::chrono::steady_clock::now() + std::chrono::milliseconds(intervalMs);
        tasks.push_back(t);
    }

protected:
    std::vector<PeriodicTask> tasks;


private:
    friend class DeviceHandler;

    // For internal use only: Calls update() on all components.
    void componentUpdate() { ([&] { getComponentRef<Components>().update(); }(), ...); }

    // Device Logic Update - Not for device programmer use. Use update() instead.
    virtual void systemUpdate() override final {
        if (!isInitialized) return;
        componentUpdate();
        update();
        if (!tasksActive) return;

        auto now = std::chrono::steady_clock::now();
        for (auto& t : tasks) {
            if (now >= t.nextUpdate) {
                t.task();
                t.nextUpdate = now + std::chrono::milliseconds(t.intervalMs);
            }
        }
    }

    // Component Access For Systems and Handlers (Not For Device Use). 
    // As a device programmer, if you need component access inside device, use getComponentRef<T>() instead of this.
    void* baseGetComponent(const std::type_info& ti) override final {
        void* result = nullptr;
        ([&] { if (typeid(Components) == ti) result = static_cast<void*>(&getComponentRef<Components>()); }(), ...);
        return result;
    }

    // Constant component Access For Systems and Handlers (Not For Device Use). 
    // As a device programmer, if you need component access inside device, use const getComponentRef<T>() instead of this.
    const void* baseGetComponent(const std::type_info& ti) const override final {
        const void* result = nullptr;
        ([&] { if (typeid(Components) == ti) result = static_cast<const void*>(&getComponentRef<Components>()); }(), ...);
        return result;
    }

};

// Specialization for empty component list (backward compatibility)
using BaseDeviceEmpty = BaseDevice<>;