#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollBar>
#include <memory>
#include <vector>
#include "message_logger.h"

namespace midicci::keyboard {

class SimpleLogWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit SimpleLogWidget(MessageLogger* logger, QWidget *parent = nullptr);

    void updateLogs();
    void clearLogs();
    void setFullTextMode(bool enabled);

private slots:
    void onNewLogEntry();

private:
    void setupUI();
    void setupConnections();
    void createLogRow(int row, const LogEntry& entry);

    static constexpr int MAX_TRUNCATED_LENGTH = 256;

    MessageLogger* m_logger;
    bool m_fullTextMode = false;
};

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(MessageLogger* logger, QWidget *parent = nullptr);

private slots:
    void onClearLogs();
    void onFullTextToggled(bool enabled);

private:
    void setupUI();

    MessageLogger* m_logger;

    QPushButton *m_clearButton;
    QPushButton *m_fullTextToggle;
    SimpleLogWidget *m_logTable;
};
}