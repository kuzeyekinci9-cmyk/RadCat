#include "MainWindow.hpp"
#include "LogicThread.hpp"
#include <QMenuBar>


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("RadCat!");
    resize(1200, 1000);
    QIcon icon(":/Main_Icon.png");
    setWindowIcon(icon);

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    setupMenuBar();

    // ---- Start backend logic thread
    logicManager = new LogicManager();
    logicManager->moveToThread(&logicThread);
    connect(&logicThread, &QThread::started, logicManager, &LogicManager::start);
    logicThread.start();
}

void MainWindow::setupMenuBar() {
    menu = menuBar();

    fileMenu = menu->addMenu("File");
    editMenu = menu->addMenu("Edit");
    viewMenu = menu->addMenu("View");
    helpMenu = menu->addMenu("Help");

    MenuBarStyle();
    //Menu Actions here:
}

void MainWindow::MenuBarStyle() {
    menu -> setStyleSheet(
    R"(

    QMenuBar {
        background-color: rgb(0,128,0);
        color: #fff;

        font-family: "Segoe UI";
        font-size: 30px;
        font-weight: bold;

        border: 1px solid #555;
        border-radius: 10px;
        padding: 5px 20px;
    }


)");
}

void MainWindow::closeEvent(QCloseEvent* event) { // Handle window close event for safe thread shutdown
    logicManager->stop();
    logicThread.quit();
    logicThread.wait();
    QMainWindow::closeEvent(event);
}