#include "main_window.h"
#include "keyboard_widget.h"
#include "log_widget.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QApplication>

namespace midicci::keyboard {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_keyboardWidget(nullptr)
    , m_logWidget(nullptr)
    , m_logger(std::make_unique<MessageLogger>())
{
    setupUI();
    setupConnections();
    
    setWindowTitle("MIDICCI UMP Keyboard");
    setMinimumSize(1000, 700);
    resize(1200, 800);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI()
{
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);
    
    m_keyboardWidget = new KeyboardWidget(this);
    m_logWidget = new LogWidget(m_logger.get(), this);
    
    m_tabWidget->addTab(m_keyboardWidget, "Keyboard");
    m_tabWidget->addTab(m_logWidget, "Logs");
    
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