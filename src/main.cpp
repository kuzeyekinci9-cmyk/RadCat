#include "app.hpp"
#include <QApplication>
#include "QTHandler.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QTHandler::instance();
    Controller controller;
    cout << "Application started." << endl;
    return app.exec();
}