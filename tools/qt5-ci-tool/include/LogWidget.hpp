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
#include <midicci/tooling/CIToolRepository.hpp>

namespace midicci::tooling::qt5 {

class VirtualizedLogWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit VirtualizedLogWidget(midicci::tooling::CIToolRepository* repository, QWidget *parent = nullptr);

    void updateLogs();
    void clearLogs();
    void setFullTextMode(bool enabled);

protected:
    void scrollContentsBy(int dx, int dy) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onNewLogEntry();

private:
    void setupUI();
    void setupConnections();
    void updateVisibleItems();
    int getVisibleItemCount() const;
    int getFirstVisibleIndex() const;
    void createLogRow(int row, const midicci::tooling::LogEntry& entry);

    static constexpr int ITEM_HEIGHT = 25;
    static constexpr int BUFFER_ITEMS = 10;
    static constexpr int MAX_TRUNCATED_LENGTH = 256;

    midicci::tooling::CIToolRepository* m_repository;
    std::vector<midicci::tooling::LogEntry> m_cachedLogs;
    size_t m_lastLogCount = 0;
    
    // Performance optimization: track which rows have been rendered
    std::vector<bool> m_renderedRows;
    
    // Text truncation control
    bool m_fullTextMode = false;
};

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(midicci::tooling::CIToolRepository* repository, QWidget *parent = nullptr);

private slots:
    void onClearLogs();
    void onFullTextToggled(bool enabled);

private:
    void setupUI();

    midicci::tooling::CIToolRepository* m_repository;

    QPushButton *m_clearButton;
    QPushButton *m_fullTextToggle;
    VirtualizedLogWidget *m_logTable;
};
}
