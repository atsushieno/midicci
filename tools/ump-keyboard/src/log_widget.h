#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <memory>
#include "message_logger.h"

namespace midicci::keyboard {

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(MessageLogger* logger, QWidget *parent = nullptr);

private slots:
    void onClearLogs();
    void onNewLogEntry();

private:
    void setupUI();
    void setupConnections();
    void updateLogDisplay();

    MessageLogger* m_logger;

    QPushButton *m_clearButton;
    QTableWidget *m_logTable;
};
}