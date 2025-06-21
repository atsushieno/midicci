#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <memory>

namespace tooling {
    class CIToolRepository;
}

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(tooling::CIToolRepository* repository, QWidget *parent = nullptr);

private slots:
    void onClearLogs();
    void onNewLogEntry();

private:
    void setupUI();
    void setupConnections();
    void updateLogDisplay();

    tooling::CIToolRepository* m_repository;
    
    QPushButton *m_clearButton;
    QTableWidget *m_logTable;
};
