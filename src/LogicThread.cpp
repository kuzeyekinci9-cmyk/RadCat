#include "LogicThread.hpp"
#include <QTimer>
#include <QThread>
#include <QDebug>
#include "System.hpp"

void LogicManager::start() {
    system = new System();
    QTimer* timer = new QTimer(this); 
    connect(timer, &QTimer::timeout, this, &LogicManager::mainLoop); 
    timer->start(0); 
}

void LogicManager::stop() { QThread::currentThread()->quit();}


// --> QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
// This is for event procession around if it takes too long.

//
void LogicManager::mainLoop() {
    if (system->isRunning) {
        system->logic();
    } else {
        QThread::currentThread()->quit();
    }
}