#include "log_widget.h"
#include "message_logger.h"
#include "midicci/midicci.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QMetaObject>
#include <ctime>

namespace midicci::keyboard {

LogWidget::LogWidget(MessageLogger* logger, QWidget *parent)
    : QWidget(parent)
    , m_logger(logger)
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
    
    m_logTable = new QTableWidget(0, 6, this);
    m_logTable->setHorizontalHeaderLabels({"Time", "Direction", "Type", "Source MUID", "Dest MUID", "Message"});
    m_logTable->horizontalHeader()->setStretchLastSection(true);
    m_logTable->setAlternatingRowColors(true);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    mainLayout->addWidget(m_logTable);
}

void LogWidget::setupConnections()
{
    connect(m_clearButton, &QPushButton::clicked, this, &LogWidget::onClearLogs);
    
    if (m_logger) {
        m_logger->add_log_callback([this](const LogEntry& entry) {
            QMetaObject::invokeMethod(this, "onNewLogEntry", Qt::QueuedConnection);
        });
    }
}

void LogWidget::onClearLogs()
{
    if (m_logger) {
        m_logger->clear_logs();
    }
    m_logTable->setRowCount(0);
}

void LogWidget::onNewLogEntry()
{
    updateLogDisplay();
}

void LogWidget::updateLogDisplay()
{
    if (!m_logger) return;
    
    auto logs = m_logger->get_logs();
    m_logTable->setRowCount(logs.size());
    
    for (size_t i = 0; i < logs.size(); ++i) {
        const auto& entry = logs[i];
        
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        auto tm = *std::localtime(&time_t);
        char time_str[32];
        std::strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm);
        
        m_logTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(time_str)));
        m_logTable->setItem(i, 1, new QTableWidgetItem(
                entry.direction == MessageDirection::In ? "In" : "Out"));
        
        // Try to determine if this is a MIDI-CI message or SysEx
        QString messageType = "SysEx";
        if (entry.message.find("MIDI-CI") != std::string::npos || 
            entry.message.find("Discovery") != std::string::npos ||
            entry.message.find("Property") != std::string::npos ||
            entry.message.find("Profile") != std::string::npos) {
            messageType = "MIDI-CI";
        }
        
        m_logTable->setItem(i, 2, new QTableWidgetItem(messageType));
        
        // Display source MUID (convert from 7-bit encoded to actual 28-bit value)
        QString sourceMuidStr = entry.source_muid == 0 ? "-" : 
            QString("0x%1").arg(QString::number(midicci::CIFactory::midiCI32to28(entry.source_muid), 16).toUpper(), 7, QChar('0'));
        m_logTable->setItem(i, 3, new QTableWidgetItem(sourceMuidStr));
        
        // Display destination MUID (convert from 7-bit encoded to actual 28-bit value)
        QString destMuidStr = entry.destination_muid == 0 ? "-" : 
            QString("0x%1").arg(QString::number(midicci::CIFactory::midiCI32to28(entry.destination_muid), 16).toUpper(), 7, QChar('0'));
        m_logTable->setItem(i, 4, new QTableWidgetItem(destMuidStr));
        
        m_logTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(entry.message)));
    }
    
    m_logTable->scrollToBottom();
}
}