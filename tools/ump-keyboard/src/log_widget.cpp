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

VirtualizedLogWidget::VirtualizedLogWidget(MessageLogger* logger, QWidget *parent)
    : QTableWidget(parent)
    , m_logger(logger)
{
    setupUI();
    setupConnections();
    updateLogs();
}

void VirtualizedLogWidget::setupUI()
{
    setColumnCount(6);
    setHorizontalHeaderLabels({"Time", "Direction", "Type", "Source MUID", "Dest MUID", "Message"});
    horizontalHeader()->setStretchLastSection(true);
    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Set fixed row height for consistent virtualization
    verticalHeader()->setDefaultSectionSize(ITEM_HEIGHT);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}

void VirtualizedLogWidget::setupConnections()
{
    if (m_logger) {
        m_logger->add_log_callback([this](const LogEntry& entry) {
            QMetaObject::invokeMethod(this, "onNewLogEntry", Qt::QueuedConnection);
        });
    }
}

void VirtualizedLogWidget::onNewLogEntry()
{
    updateLogs();
}

void VirtualizedLogWidget::updateLogs()
{
    if (!m_logger) return;
    
    auto logs = m_logger->get_logs();
    
    // Only update if log count changed
    if (logs.size() != m_lastLogCount) {
        bool wasAtBottom = (verticalScrollBar()->value() == verticalScrollBar()->maximum());
        size_t oldSize = m_lastLogCount;
        
        m_cachedLogs = logs;
        m_lastLogCount = logs.size();
        
        setRowCount(m_cachedLogs.size());
        
        // Only invalidate rendered rows for new entries, not all rows
        if (logs.size() > oldSize) {
            // New entries added - only need to render new ones
            // Existing rendered items can stay as-is
        } else if (logs.size() < oldSize) {
            // Logs were cleared or removed - reset rendered state
            m_renderedRows.clear();
            m_renderedRows.resize(logs.size(), false);
        }
        
        updateVisibleItems();
        
        // Auto-scroll to bottom only if we were already at bottom
        if (wasAtBottom && logs.size() > oldSize) {
            scrollToBottom();
        }
    }
}

void VirtualizedLogWidget::clearLogs()
{
    if (m_logger) {
        m_logger->clear_logs();
    }
    m_cachedLogs.clear();
    m_renderedRows.clear();
    m_lastLogCount = 0;
    setRowCount(0);
}

void VirtualizedLogWidget::setFullTextMode(bool enabled)
{
    if (m_fullTextMode != enabled) {
        m_fullTextMode = enabled;
        // Clear rendered rows cache to force re-rendering with new truncation setting
        m_renderedRows.clear();
        m_renderedRows.resize(m_cachedLogs.size(), false);
        updateVisibleItems();
    }
}

void VirtualizedLogWidget::scrollContentsBy(int dx, int dy)
{
    QTableWidget::scrollContentsBy(dx, dy);
    updateVisibleItems();
}

void VirtualizedLogWidget::resizeEvent(QResizeEvent* event)
{
    QTableWidget::resizeEvent(event);
    updateVisibleItems();
}

int VirtualizedLogWidget::getVisibleItemCount() const
{
    if (ITEM_HEIGHT <= 0) return 0;
    return (height() / ITEM_HEIGHT) + 2; // +2 for partial items at top/bottom
}

int VirtualizedLogWidget::getFirstVisibleIndex() const
{
    if (ITEM_HEIGHT <= 0) return 0;
    return verticalScrollBar()->value() / ITEM_HEIGHT;
}

void VirtualizedLogWidget::updateVisibleItems()
{
    if (m_cachedLogs.empty()) return;
    
    int firstVisible = getFirstVisibleIndex();
    int visibleCount = getVisibleItemCount();
    
    // Add buffer items above and below
    int startIndex = qMax(0, firstVisible - BUFFER_ITEMS);
    int endIndex = qMin(static_cast<int>(m_cachedLogs.size()) - 1, 
                       firstVisible + visibleCount + BUFFER_ITEMS);
    
    // Clear items outside visible range - but only if they exist
    for (int i = 0; i < rowCount(); ++i) {
        if (i < startIndex || i > endIndex) {
            if (i < static_cast<int>(m_renderedRows.size()) && m_renderedRows[i]) {
                for (int col = 0; col < columnCount(); ++col) {
                    setItem(i, col, nullptr);
                }
                m_renderedRows[i] = false;
            }
        }
    }
    
    // Create/update items in visible range - only if not already rendered
    for (int i = startIndex; i <= endIndex; ++i) {
        if (i >= 0 && i < static_cast<int>(m_cachedLogs.size())) {
            if (i >= static_cast<int>(m_renderedRows.size()) || !m_renderedRows[i]) {
                createLogRow(i, m_cachedLogs[i]);
                if (i >= static_cast<int>(m_renderedRows.size())) {
                    m_renderedRows.resize(i + 1, false);
                }
                m_renderedRows[i] = true;
            }
        }
    }
}

void VirtualizedLogWidget::createLogRow(int row, const LogEntry& entry)
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
    
    m_logTable = new VirtualizedLogWidget(m_logger, this);
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