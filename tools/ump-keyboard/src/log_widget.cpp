#include "log_widget.h"
#include "message_logger.h"
#include "midicci/midicci.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QMetaObject>
#include <QApplication>
#include <QScrollBar>
#include <ctime>

namespace midicci::keyboard {

SimpleLogWidget::SimpleLogWidget(MessageLogger* logger, QWidget *parent)
    : QTableWidget(parent)
    , m_logger(logger)
{
    setupUI();
    setupConnections();
    updateLogs();
}

void SimpleLogWidget::setupUI()
{
    setColumnCount(6);
    setHorizontalHeaderLabels({"Time", "Direction", "Type", "Source MUID", "Dest MUID", "Message"});
    horizontalHeader()->setStretchLastSection(true);
    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
}

void SimpleLogWidget::setupConnections()
{
    if (m_logger) {
        m_logger->add_log_callback([this](const LogEntry& entry) {
            QMetaObject::invokeMethod(this, "onNewLogEntry", Qt::QueuedConnection);
        });
    }
}

void SimpleLogWidget::onNewLogEntry()
{
    updateLogs();
}

void SimpleLogWidget::updateLogs()
{
    if (!m_logger) return;
    
    auto logs = m_logger->get_logs();
    bool wasAtBottom = (verticalScrollBar()->value() == verticalScrollBar()->maximum());
    
    setRowCount(logs.size());
    
    for (size_t i = 0; i < logs.size(); ++i) {
        createLogRow(i, logs[i]);
    }
    
    if (wasAtBottom) {
        scrollToBottom();
    }
}

void SimpleLogWidget::clearLogs()
{
    if (m_logger) {
        m_logger->clear_logs();
    }
    setRowCount(0);
}

void SimpleLogWidget::setFullTextMode(bool enabled)
{
    if (m_fullTextMode != enabled) {
        m_fullTextMode = enabled;
        updateLogs();
    }
}


void SimpleLogWidget::createLogRow(int row, const LogEntry& entry)
{
    // Time - optimize by formatting only once
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    auto tm = *std::localtime(&time_t);
    char time_str[16];
    std::strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm);
    
    // Pre-allocate items to avoid repeated allocation
    QTableWidgetItem* timeItem = new QTableWidgetItem(QString::fromLatin1(time_str));
    QTableWidgetItem* dirItem = new QTableWidgetItem(entry.direction == MessageDirection::In ? QStringLiteral("In") : QStringLiteral("Out"));
    
    // Message type - optimize string comparison
    QTableWidgetItem* typeItem;
    if (entry.message.find("MIDI-CI") != std::string::npos || 
        entry.message.find("Discovery") != std::string::npos ||
        entry.message.find("Property") != std::string::npos ||
        entry.message.find("Profile") != std::string::npos) {
        typeItem = new QTableWidgetItem(QStringLiteral("MIDI-CI"));
    } else {
        typeItem = new QTableWidgetItem(QStringLiteral("SysEx"));
    }
    
    // MUID formatting - optimize for common case of 0
    QTableWidgetItem* sourceMuidItem;
    if (entry.source_muid == 0) {
        sourceMuidItem = new QTableWidgetItem(QStringLiteral("-"));
    } else {
        uint32_t muid28 = midicci::CIFactory::midiCI32to28(entry.source_muid);
        sourceMuidItem = new QTableWidgetItem(QString("0x%1").arg(muid28, 7, 16, QChar('0')).toUpper());
    }
    
    QTableWidgetItem* destMuidItem;
    if (entry.destination_muid == 0) {
        destMuidItem = new QTableWidgetItem(QStringLiteral("-"));
    } else {
        uint32_t muid28 = midicci::CIFactory::midiCI32to28(entry.destination_muid);
        destMuidItem = new QTableWidgetItem(QString("0x%1").arg(muid28, 7, 16, QChar('0')).toUpper());
    }
    
    // Message - apply truncation if not in full text mode
    QTableWidgetItem* messageItem;
    if (!m_fullTextMode && entry.message.length() > MAX_TRUNCATED_LENGTH) {
        std::string truncated = entry.message.substr(0, MAX_TRUNCATED_LENGTH) + "... [truncated]";
        messageItem = new QTableWidgetItem(QString::fromStdString(truncated));
    } else {
        messageItem = new QTableWidgetItem(QString::fromStdString(entry.message));
    }
    
    // Set all items at once
    setItem(row, 0, timeItem);
    setItem(row, 1, dirItem);
    setItem(row, 2, typeItem);
    setItem(row, 3, sourceMuidItem);
    setItem(row, 4, destMuidItem);
    setItem(row, 5, messageItem);
}

LogWidget::LogWidget(MessageLogger* logger, QWidget *parent)
    : QWidget(parent)
    , m_logger(logger)
{
    setupUI();
}

void LogWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    
    auto *buttonLayout = new QHBoxLayout();
    m_clearButton = new QPushButton("Clear", this);
    m_fullTextToggle = new QPushButton("Full Text: OFF", this);
    m_fullTextToggle->setCheckable(true);
    m_fullTextToggle->setChecked(false);
    
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addWidget(m_fullTextToggle);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    m_logTable = new SimpleLogWidget(m_logger, this);
    mainLayout->addWidget(m_logTable);
    
    connect(m_clearButton, &QPushButton::clicked, this, &LogWidget::onClearLogs);
    connect(m_fullTextToggle, &QPushButton::toggled, this, &LogWidget::onFullTextToggled);
}

void LogWidget::onClearLogs()
{
    m_logTable->clearLogs();
}

void LogWidget::onFullTextToggled(bool enabled)
{
    m_fullTextToggle->setText(enabled ? "Full Text: ON" : "Full Text: OFF");
    m_logTable->setFullTextMode(enabled);
}
}