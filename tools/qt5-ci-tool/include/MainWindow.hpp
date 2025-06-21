#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <memory>

class InitiatorWidget;
class ResponderWidget;
class LogWidget;
class SettingsWidget;

namespace tooling {
    class CIToolRepository;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onTabChanged(int index);

private:
    void setupUI();
    void setupConnections();

    QTabWidget *m_tabWidget;
    InitiatorWidget *m_initiatorWidget;
    ResponderWidget *m_responderWidget;
    LogWidget *m_logWidget;
    SettingsWidget *m_settingsWidget;
    

};
