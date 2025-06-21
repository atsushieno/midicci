#include "MainWindow.hpp"
#include "InitiatorWidget.hpp"
#include "ResponderWidget.hpp"
#include "LogWidget.hpp"
#include "SettingsWidget.hpp"
#include "AppModel.hpp"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QApplication>
#include <QIcon>

namespace midicci::tooling::qt5 {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_initiatorWidget(nullptr)
    , m_responderWidget(nullptr)
    , m_logWidget(nullptr)
    , m_settingsWidget(nullptr)
{
    setupUI();
    setupConnections();
    
    setWindowTitle("Qt5 MIDI-CI Tool");
    setMinimumSize(1000, 700);
    resize(1200, 800);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI()
{
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);
    
    auto& repository = getAppModel();
    
    m_initiatorWidget = new InitiatorWidget(&repository, this);
    m_responderWidget = new ResponderWidget(&repository, this);
    m_logWidget = new LogWidget(&repository, this);
    m_settingsWidget = new SettingsWidget(&repository, this);
    
    m_tabWidget->addTab(m_initiatorWidget, "Initiator");
    m_tabWidget->addTab(m_responderWidget, "Responder");
    m_tabWidget->addTab(m_logWidget, "Logs");
    m_tabWidget->addTab(m_settingsWidget, "Settings");
    
    statusBar()->showMessage("Ready");
}

void MainWindow::setupConnections()
{
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
}

void MainWindow::onTabChanged(int index)
{
    QString tabName = m_tabWidget->tabText(index);
    statusBar()->showMessage(QString("Switched to %1 tab").arg(tabName));
}

}
