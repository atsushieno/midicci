#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QFileDialog>
#include <memory>
#include <midicci/tooling/MidiDeviceManager.hpp>
#include <midicci/tooling/CIToolRepository.hpp>

namespace midicci::tooling::qt5 {

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(midicci::tooling::CIToolRepository* repository, QWidget *parent = nullptr);

private slots:
    void onInputDeviceChanged(int index);
    void onOutputDeviceChanged(int index);
    void onLoadConfiguration();
    void onSaveConfiguration();
    void onUpdateDeviceInfo();
    void onUpdateJsonSchema();
    void onWorkaroundJUCESubscriptionChanged(bool enabled);
    void onWorkaroundJUCEProfileChannelsChanged(bool enabled);

private:
    void setupUI();
    void setupConnections();
    void updateDeviceLists();
    void updateDeviceConfiguration();
    void checkAndAutoConnect();
    static QString normalizeDeviceName(const QString& deviceName);

    midicci::tooling::CIToolRepository* m_repository;
    
    QGroupBox *m_midiTransportGroup;
    QComboBox *m_inputDeviceCombo;
    QComboBox *m_outputDeviceCombo;
    
    QGroupBox *m_configGroup;
    QPushButton *m_loadConfigButton;
    QPushButton *m_saveConfigButton;
    QLabel *m_configFileLabel;
    
    QGroupBox *m_deviceConfigGroup;
    QLineEdit *m_manufacturerIdEdit;
    QLineEdit *m_familyIdEdit;
    QLineEdit *m_modelIdEdit;
    QLineEdit *m_versionIdEdit;
    QLineEdit *m_manufacturerEdit;
    QLineEdit *m_familyEdit;
    QLineEdit *m_modelEdit;
    QLineEdit *m_versionEdit;
    QLineEdit *m_serialNumberEdit;
    QSpinBox *m_maxConnectionsSpin;
    QPushButton *m_updateDeviceInfoButton;
    
    QGroupBox *m_jsonSchemaGroup;
    QTextEdit *m_jsonSchemaEdit;
    QPushButton *m_updateJsonSchemaButton;
    
    QGroupBox *m_behavioralGroup;
    QCheckBox *m_workaroundJUCESubscriptionCheck;
    QCheckBox *m_workaroundJUCEProfileChannelsCheck;
};
}
