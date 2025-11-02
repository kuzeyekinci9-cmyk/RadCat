#include "QTHandler.hpp"
#include <QCoreApplication>
#include <QMetaObject>
#include <QDebug>
#include "deviceCore.hpp"

QTHandler* QTHandler::s_instance = nullptr;

QTHandler::QTHandler(QObject* parent) : QObject(parent) {
    // empty
}

QTHandler::~QTHandler() = default;

QTHandler& QTHandler::instance() {
    static std::mutex createMutex;
    std::lock_guard<std::mutex> lk(createMutex);
    if (s_instance) return *s_instance;

    // If we are already on the GUI thread, create directly.
    if (QCoreApplication::instance() && QThread::currentThread() == QCoreApplication::instance()->thread()) {
        s_instance = new QTHandler(QCoreApplication::instance());
        return *s_instance;
    }

    // Otherwise, create the object on the GUI thread using a blocking queued call.
    if (QCoreApplication::instance()) {
        QMetaObject::invokeMethod(QCoreApplication::instance(), [&]() {
            if (!s_instance) s_instance = new QTHandler(QCoreApplication::instance());
        }, Qt::BlockingQueuedConnection);
        return *s_instance;
    }

    // No QCoreApplication present; create on current thread (best-effort)
    s_instance = new QTHandler(nullptr);
    return *s_instance;
}

void QTHandler::registerFactory(const std::type_index& type, Factory factory) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_factories[type] = std::move(factory);
}

void QTHandler::unregisterFactory(const std::type_index& type) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_factories.erase(type);
}

QWidget* QTHandler::createWidgetInternal(EmptyDevice* device, QWidget* parent) {
    if (!device) return nullptr;
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_factories.find(std::type_index(typeid(*device)));
    if (it == m_factories.end()) return nullptr;
    try {
        return it->second(device, parent);
    } catch (const std::exception& ex) {
        qWarning() << "QTHandler: factory threw exception:" << ex.what();
    } catch (...) {
        qWarning() << "QTHandler: factory threw unknown exception";
    }
    return nullptr;
}

void QTHandler::createWidgetAsync(EmptyDevice* device, QWidget* parent) {
    // Ensure executed on GUI thread; we use queued invocation of a lambda.
    QMetaObject::invokeMethod(this, [this, device, parent]() {
        QWidget* w = createWidgetInternal(device, parent);
        emit widgetCreated(device, w);
    }, Qt::QueuedConnection);
}

QWidget* QTHandler::createWidgetSync(EmptyDevice* device, QWidget* parent) {
    // If caller is already on the GUI thread, call directly to avoid deadlock.
    if (QThread::currentThread() == this->thread()) {
        return createWidgetInternal(device, parent);
    }

    QWidget* result = nullptr;
    // BlockingQueuedConnection will execute on this object's thread (GUI) and block until done.
    QMetaObject::invokeMethod(this, [&]() {
        result = createWidgetInternal(device, parent);
    }, Qt::BlockingQueuedConnection);
    return result;
}
