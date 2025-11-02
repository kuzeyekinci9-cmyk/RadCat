#pragma once

#include <QObject>
#include <QWidget>
#include <QMetaObject>
#include <QCoreApplication>
#include <QThread>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <mutex>
#include <memory>

class EmptyDevice; // forward

/**
 * QTHandler
 *
 * Thread-safe singleton that lives on the Qt GUI thread. Components and
 * device code (possibly running on other threads) can register widget
 * factories and request widget creation. All QWidget creation and
 * signals are executed on the GUI thread.
 *
 * Usage:
 *  - Register a factory for a concrete device type:
 *      QTHandler::instance().registerFactory(std::type_index(typeid(MyDevice)),
 *          [](EmptyDevice* d, QWidget* parent){ return new MyDeviceWidget(dynamic_cast<MyDevice*>(d), parent); });
 *  - Create widget async (safe from any thread):
 *      QTHandler::instance().createWidgetAsync(devicePtr, parent);
 *    Connect to widgetCreated signal to receive the created widget (GUI thread).
 *  - Create widget synchronously (blocking) from any thread:
 *      QWidget* w = QTHandler::instance().createWidgetSync(devicePtr, parent);
 */

class QTHandler : public QObject {
    Q_OBJECT
public:
    using Factory = std::function<QWidget*(EmptyDevice*, QWidget*)>;

    // Returns the singleton instance. If called off the GUI thread, ensures
    // the QTHandler object is created on the GUI thread before returning.
    static QTHandler& instance();

    // Register a factory for a device type (type index is typically typeid(Device)).
    void registerFactory(const std::type_index& type, Factory factory);

    template<typename T>
    void registerFactoryFor(Factory factory) {
        registerFactory(std::type_index(typeid(T)), std::move(factory));
    }

    // Unregister factory for a type
    void unregisterFactory(const std::type_index& type);

    // Asynchronously create widget on GUI thread; emits widgetCreated when done.
    void createWidgetAsync(EmptyDevice* device, QWidget* parent = nullptr);

    // Synchronously (blocking) create widget on GUI thread and return it.
    QWidget* createWidgetSync(EmptyDevice* device, QWidget* parent = nullptr);

signals:
    // Emitted on GUI thread when a widget is created for a device.
    void widgetCreated(EmptyDevice* device, QWidget* widget);

private:
    explicit QTHandler(QObject* parent = nullptr);
    ~QTHandler() override;

    // Internal creation routine (runs on GUI thread)
    QWidget* createWidgetInternal(EmptyDevice* device, QWidget* parent);

    std::unordered_map<std::type_index, Factory> m_factories;
    mutable std::mutex m_mutex;

    static QTHandler* s_instance;
};
