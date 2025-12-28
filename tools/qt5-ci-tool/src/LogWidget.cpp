#include "LogWidget.hpp"
#include <midicci/tooling/CIToolRepository.hpp>
#include <midicci/midicci.hpp>
#include "AppModel.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QMetaObject>
#include <QApplication>
#include <QScrollBar>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <ctime>

namespace midicci::tooling::qt5 {

SimpleLogWidget::SimpleLogWidget(midicci::tooling::CIToolRepository* repository, QWidget *parent)
    : QTableWidget(parent)
    , m_repository(repository)
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
    if (m_repository) {
        m_repository->add_log_callback([this](const midicci::tooling::LogEntry& entry) {
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
    if (!m_repository) return;
    
    auto logs = m_repository->get_logs();
    bool wasAtBottom = (verticalScrollBar()->value() == verticalScrollBar()->maximum());

    // Only append new rows since last update
    const size_t oldCount = m_lastRowCount;
    const size_t newCount = logs.size();
    if (newCount <= oldCount) {
        return; // nothing new
    }

    // Prevent excessive repaints while appending
    setUpdatesEnabled(false);
    setRowCount(static_cast<int>(newCount));
    for (size_t i = oldCount; i < newCount; ++i) {
        createLogRow(static_cast<int>(i), logs[i]);
    }
    setUpdatesEnabled(true);

    m_lastRowCount = newCount;

    if (wasAtBottom) {
        scrollToBottom();
    }
}

void SimpleLogWidget::clearLogs()
{
    if (m_repository) {
        m_repository->clear_logs();
    }
    setRowCount(0);
    m_lastRowCount = 0;
}

void SimpleLogWidget::setFullTextMode(bool enabled)
{
    if (m_fullTextMode != enabled) {
        m_fullTextMode = enabled;
        // Force a rebuild of visible items with new mode
        auto logs = m_repository ? m_repository->get_logs() : std::vector<midicci::tooling::LogEntry>{};
        setRowCount(0);
        m_lastRowCount = 0;
        setRowCount(static_cast<int>(logs.size()));
        for (size_t i = 0; i < logs.size(); ++i) {
            createLogRow(static_cast<int>(i), logs[i]);
        }
        m_lastRowCount = logs.size();
    }
}


void SimpleLogWidget::createLogRow(int row, const midicci::tooling::LogEntry& entry)
{
    // Time - optimize by formatting only once
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    auto tm = *std::localtime(&time_t);
    char time_str[16];
    std::strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm);
    
    // Pre-allocate items to avoid repeated allocation
    QTableWidgetItem* timeItem = new QTableWidgetItem(QString::fromLatin1(time_str));
    QTableWidgetItem* dirItem = new QTableWidgetItem(entry.direction == midicci::tooling::MessageDirection::In ? QStringLiteral("In") : QStringLiteral("Out"));
    
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

LogWidget::LogWidget(midicci::tooling::CIToolRepository* repository, QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
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
    m_recordCheck = new QCheckBox("Record logs", this);
    m_saveInputsButton = new QPushButton("Save Inputs", this);
    m_saveOutputsButton = new QPushButton("Save Outputs", this);
    
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addWidget(m_fullTextToggle);
    buttonLayout->addWidget(m_recordCheck);
    buttonLayout->addWidget(m_saveInputsButton);
    buttonLayout->addWidget(m_saveOutputsButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    m_logTable = new SimpleLogWidget(m_repository, this);
    mainLayout->addWidget(m_logTable);
    
    connect(m_clearButton, &QPushButton::clicked, this, &LogWidget::onClearLogs);
    connect(m_fullTextToggle, &QPushButton::toggled, this, &LogWidget::onFullTextToggled);
    connect(m_recordCheck, &QCheckBox::toggled, this, &LogWidget::onRecordToggled);
    connect(m_saveInputsButton, &QPushButton::clicked, this, &LogWidget::onSaveInputs);
    connect(m_saveOutputsButton, &QPushButton::clicked, this, &LogWidget::onSaveOutputs);
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

void LogWidget::onRecordToggled(bool enabled)
{
    if (m_repository) {
        m_repository->set_recording_enabled(enabled);
    }
}

static bool save_bytes_to_file(QWidget* parent, const QString& suggested, const std::vector<uint8_t>& data)
{
    QString filename = QFileDialog::getSaveFileName(parent, "Save Bytes", suggested, "Binary files (*.bin);;All files (*)");
    if (filename.isEmpty()) return false;
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(parent, "Save Failed", QString("Could not open %1 for writing").arg(filename));
        return false;
    }
    if (!data.empty()) {
        file.write(reinterpret_cast<const char*>(data.data()), static_cast<qint64>(data.size()));
    }
    file.close();
    return true;
}

void LogWidget::onSaveInputs()
{
    if (!m_repository) return;
    auto bytes = m_repository->get_recorded_inputs();
    save_bytes_to_file(this, "inputs.bin", bytes);
}

void LogWidget::onSaveOutputs()
{
    if (!m_repository) return;
    auto bytes = m_repository->get_recorded_outputs();
    save_bytes_to_file(this, "outputs.bin", bytes);
}
}
