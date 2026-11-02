// SCAPMS - Smart Clinic Appointment & Patient Management System
// COS202 Programming II lab project. Entry point.

#include "gui/MainWindow.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("SCAPMS");
    QApplication::setStyle(QStyleFactory::create("Fusion")); // same look on every OS

    scapms::gui::MainWindow window;
    window.loadStartupData();
    window.show();
    return QApplication::exec();
}
