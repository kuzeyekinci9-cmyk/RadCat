#pragma once
#include <QObject>

class System;

class LogicManager : public QObject {
    Q_OBJECT
public:
    explicit LogicManager(QObject* parent = nullptr) : QObject(parent) { 
        // Only connections here ! No logic !

    }
System* system; //Core system instance

public slots:
void start();
void stop();
void mainLoop();

signals:

};