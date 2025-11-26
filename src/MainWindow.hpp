#pragma once
#include <QThread>
#include <QObject>
#include <QMainWindow>

class LogicManager;
class QMenuBar;

class MainWindow : public QMainWindow { 
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() {}
    void closeEvent(QCloseEvent* event) override; // Handle window close event for safe thread shutdown

private:
    QThread logicThread;
    LogicManager* logicManager;

    void setupMenuBar();
    void MenuBarStyle();
    QMenuBar* menu;
    QMenu* fileMenu;
    QMenu* editMenu;
    QMenu* viewMenu;
    QMenu* helpMenu;
    

};