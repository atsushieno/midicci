#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include "MainWindow.hpp"
#include "AppModel.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("Qt5 MIDI-CI Tool");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("MIDI-CI Tools");
    
    qt5_ci_tool::initializeAppModel();
    
    MainWindow window;
    window.show();
    
    int result = app.exec();
    
    qt5_ci_tool::shutdownAppModel();
    
    return result;
}
