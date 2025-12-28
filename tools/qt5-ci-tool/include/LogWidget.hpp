#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollBar>
#include <memory>
#include <vector>
#include <midicci/tooling/CIToolRepository.hpp>

namespace midicci::tooling::qt5 {

class SimpleLogWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit SimpleLogWidget(midicci::tooling::CIToolRepository* repository, QWidget *parent = nullptr);

    void updateLogs();
    void clearLogs();
    void setFullTextMode(bool enabled);

private slots:
    void onNewLogEntry();

private:
    void setupUI();
    void setupConnections();
    void createLogRow(int row, const midicci::tooling::LogEntry& entry);

    static constexpr int MAX_TRUNCATED_LENGTH = 256;

    midicci::tooling::CIToolRepository* m_repository;
    bool m_fullTextMode = false;
    size_t m_lastRowCount = 0;
};

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(midicci::tooling::CIToolRepository* repository, QWidget *parent = nullptr);

private slots:
    void onClearLogs();
    void onFullTextToggled(bool enabled);
    void onRecordToggled(bool enabled);
    void onSaveInputs();
    void onSaveOutputs();

private:
    void setupUI();

    midicci::tooling::CIToolRepository* m_repository;

    QPushButton *m_clearButton;
    QPushButton *m_fullTextToggle;
    QCheckBox *m_recordCheck;
    QPushButton *m_saveInputsButton;
    QPushButton *m_saveOutputsButton;
    SimpleLogWidget *m_logTable;
};
}
