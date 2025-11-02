#include "app.hpp"
#include <QApplication>
#include "QTHandler.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QTHandler::instance();
    Controller controller;
    return app.exec();
}