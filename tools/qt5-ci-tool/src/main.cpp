#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include "MainWindow.hpp"
#include "AppModel.hpp"

using namespace midicci::tooling::qt5;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("Qt5 MIDI-CI Tool");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("MIDI-CI Tools");
    
    initializeAppModel();
    
    MainWindow window;
    window.show();
    
    int result = app.exec();
    
    shutdownAppModel();
    
    return result;
}
