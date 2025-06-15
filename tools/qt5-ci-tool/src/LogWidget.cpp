#include "LogWidget.hpp"
#include "CIToolRepository.hpp"
#include "AppModel.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDateTime>

LogWidget::LogWidget(ci_tool::CIToolRepository* repository, QWidget *parent)
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
}

void LogWidget::onClearLogs()
{
    m_logTable->setRowCount(0);
    if (m_repository) {
        m_repository->log("Log cleared", ci_tool::MessageDirection::Out);
    }
}

void LogWidget::onNewLogEntry()
{
    updateLogDisplay();
}

void LogWidget::updateLogDisplay()
{
    int row = m_logTable->rowCount();
    m_logTable->insertRow(row);
    
    QDateTime currentTime = QDateTime::currentDateTime();
    m_logTable->setItem(row, 0, new QTableWidgetItem(currentTime.toString("hh:mm:ss.zzz")));
    m_logTable->setItem(row, 1, new QTableWidgetItem("Out"));
    m_logTable->setItem(row, 2, new QTableWidgetItem("Local -> Remote"));
    m_logTable->setItem(row, 3, new QTableWidgetItem("Sample log message"));
    
    m_logTable->scrollToBottom();
}
