#include "LogWidget.hpp"
#include "CIToolRepository.hpp"
#include "AppModel.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QMetaObject>
#include <ctime>

LogWidget::LogWidget(tooling::CIToolRepository* repository, QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
{
    setupUI();
    setupConnections();
    updateLogDisplay();
}

void LogWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    
    auto *buttonLayout = new QHBoxLayout();
    m_clearButton = new QPushButton("Clear", this);
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    m_logTable = new QTableWidget(0, 4, this);
    m_logTable->setHorizontalHeaderLabels({"Time", "Direction", "Source/Dest", "Message"});
    m_logTable->horizontalHeader()->setStretchLastSection(true);
    m_logTable->setAlternatingRowColors(true);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    mainLayout->addWidget(m_logTable);
}

void LogWidget::setupConnections()
{
    connect(m_clearButton, &QPushButton::clicked, this, &LogWidget::onClearLogs);
    
    if (m_repository) {
        m_repository->add_log_callback([this](const tooling::LogEntry& entry) {
            QMetaObject::invokeMethod(this, "onNewLogEntry", Qt::QueuedConnection);
        });
    }
}

void LogWidget::onClearLogs()
{
    if (m_repository) {
        m_repository->clear_logs();
    }
    m_logTable->setRowCount(0);
}

void LogWidget::onNewLogEntry()
{
    updateLogDisplay();
}

void LogWidget::updateLogDisplay()
{
    if (!m_repository) return;
    
    auto logs = m_repository->get_logs();
    m_logTable->setRowCount(logs.size());
    
    for (size_t i = 0; i < logs.size(); ++i) {
        const auto& entry = logs[i];
        
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        auto tm = *std::localtime(&time_t);
        char time_str[32];
        std::strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm);
        
        m_logTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(time_str)));
        m_logTable->setItem(i, 1, new QTableWidgetItem(
                entry.direction == tooling::MessageDirection::In ? "In" : "Out"));
        m_logTable->setItem(i, 2, new QTableWidgetItem("MIDI"));
        m_logTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(entry.message)));
    }
    
    m_logTable->scrollToBottom();
}
