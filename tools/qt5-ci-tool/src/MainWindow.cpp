#include "MainWindow.hpp"
#include "InitiatorWidget.hpp"
#include "ResponderWidget.hpp"
#include "LogWidget.hpp"
#include "SettingsWidget.hpp"
#include "CIToolRepository.hpp"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QApplication>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_initiatorWidget(nullptr)
    , m_responderWidget(nullptr)
    , m_logWidget(nullptr)
    , m_settingsWidget(nullptr)
    , m_repository(std::make_unique<ci_tool::CIToolRepository>())
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
    
    m_initiatorWidget = new InitiatorWidget(m_repository.get(), this);
    m_responderWidget = new ResponderWidget(m_repository.get(), this);
    m_logWidget = new LogWidget(m_repository.get(), this);
    m_settingsWidget = new SettingsWidget(m_repository.get(), this);
    
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
