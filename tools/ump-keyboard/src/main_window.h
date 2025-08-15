#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <memory>
#include "message_logger.h"
#include "keyboard_widget.h"
#include "log_widget.h"

namespace midicci::keyboard {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    MessageLogger* getLogger() const { return m_logger.get(); }

private slots:
    void onTabChanged(int index);

private:
    void setupUI();
    void setupConnections();

    QTabWidget *m_tabWidget;
    KeyboardWidget *m_keyboardWidget;
    LogWidget *m_logWidget;
    
    std::unique_ptr<MessageLogger> m_logger;
};

}