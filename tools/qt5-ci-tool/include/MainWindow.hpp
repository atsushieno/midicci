#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <memory>
#include "InitiatorWidget.hpp"
#include "ResponderWidget.hpp"
#include "LogWidget.hpp"
#include "SettingsWidget.hpp"

namespace midicci::tooling::qt5 {

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
}
