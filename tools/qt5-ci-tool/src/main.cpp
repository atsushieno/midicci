#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include "MainWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("Qt5 MIDI-CI Tool");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("MIDI-CI Tools");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
