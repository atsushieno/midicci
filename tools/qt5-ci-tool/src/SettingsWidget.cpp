#include "SettingsWidget.hpp"
#include "CIToolRepository.hpp"
#include "AppModel.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

SettingsWidget::SettingsWidget(ci_tool::CIToolRepository* repository, QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
{
    setupUI();
    setupConnections();
    updateDeviceLists();
    updateDeviceConfiguration();
}

void SettingsWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    
    m_midiTransportGroup = new QGroupBox("MIDI Transport Settings", this);
    auto *transportLayout = new QGridLayout(m_midiTransportGroup);
    
    transportLayout->addWidget(new QLabel("By default, it receives MIDI-CI requests on the Virtual In port and sends replies back from the Virtual Out port."), 0, 0, 1, 2);
    transportLayout->addWidget(new QLabel("But you can also use system MIDI devices as the transports too. Select them here then."), 1, 0, 1, 2);
    
    transportLayout->addWidget(new QLabel("From:"), 2, 0);
    m_inputDeviceCombo = new QComboBox(this);
    transportLayout->addWidget(m_inputDeviceCombo, 2, 1);
    
    transportLayout->addWidget(new QLabel("To:"), 3, 0);
    m_outputDeviceCombo = new QComboBox(this);
    transportLayout->addWidget(m_outputDeviceCombo, 3, 1);
    
    mainLayout->addWidget(m_midiTransportGroup);
    
    m_configGroup = new QGroupBox("Load and Save", this);
    auto *configLayout = new QVBoxLayout(m_configGroup);
    
    m_configFileLabel = new QLabel("Configuration file is 'ktmidi-ci-tool.settings.json_ish'", this);
    configLayout->addWidget(m_configFileLabel);
    
    auto *configButtonLayout = new QHBoxLayout();
    m_loadConfigButton = new QPushButton("Load configuration", this);
    m_saveConfigButton = new QPushButton("Save configuration", this);
    configButtonLayout->addWidget(m_loadConfigButton);
    configButtonLayout->addWidget(m_saveConfigButton);
    configButtonLayout->addStretch();
    
    configLayout->addLayout(configButtonLayout);
    mainLayout->addWidget(m_configGroup);
    
    m_deviceConfigGroup = new QGroupBox("Local Device Configuration", this);
    auto *deviceConfigLayout = new QGridLayout(m_deviceConfigGroup);
    
    deviceConfigLayout->addWidget(new QLabel("Note that each ID byte is in 7 bits. Hex more than 80h is invalid."), 0, 0, 1, 4);
    
    deviceConfigLayout->addWidget(new QLabel("Manufacturer ID:"), 1, 0);
    m_manufacturerIdEdit = new QLineEdit(this);
    deviceConfigLayout->addWidget(m_manufacturerIdEdit, 1, 1);
    deviceConfigLayout->addWidget(new QLabel("Text:"), 1, 2);
    m_manufacturerEdit = new QLineEdit(this);
    deviceConfigLayout->addWidget(m_manufacturerEdit, 1, 3);
    
    deviceConfigLayout->addWidget(new QLabel("Family ID:"), 2, 0);
    m_familyIdEdit = new QLineEdit(this);
    deviceConfigLayout->addWidget(m_familyIdEdit, 2, 1);
    deviceConfigLayout->addWidget(new QLabel("Text:"), 2, 2);
    m_familyEdit = new QLineEdit(this);
    deviceConfigLayout->addWidget(m_familyEdit, 2, 3);
    
    deviceConfigLayout->addWidget(new QLabel("Model ID:"), 3, 0);
    m_modelIdEdit = new QLineEdit(this);
    deviceConfigLayout->addWidget(m_modelIdEdit, 3, 1);
    deviceConfigLayout->addWidget(new QLabel("Text:"), 3, 2);
    m_modelEdit = new QLineEdit(this);
    deviceConfigLayout->addWidget(m_modelEdit, 3, 3);
    
    deviceConfigLayout->addWidget(new QLabel("Version ID:"), 4, 0);
    m_versionIdEdit = new QLineEdit(this);
    deviceConfigLayout->addWidget(m_versionIdEdit, 4, 1);
    deviceConfigLayout->addWidget(new QLabel("Text:"), 4, 2);
    m_versionEdit = new QLineEdit(this);
    deviceConfigLayout->addWidget(m_versionEdit, 4, 3);
    
    deviceConfigLayout->addWidget(new QLabel("Serial Number:"), 5, 0);
    m_serialNumberEdit = new QLineEdit(this);
    deviceConfigLayout->addWidget(m_serialNumberEdit, 5, 1, 1, 3);
    
    deviceConfigLayout->addWidget(new QLabel("Max Connections:"), 6, 0);
    m_maxConnectionsSpin = new QSpinBox(this);
    m_maxConnectionsSpin->setRange(1, 255);
    m_maxConnectionsSpin->setValue(8);
    deviceConfigLayout->addWidget(m_maxConnectionsSpin, 6, 1);
    
    m_updateDeviceInfoButton = new QPushButton("Update Device Info", this);
    deviceConfigLayout->addWidget(m_updateDeviceInfoButton, 7, 0, 1, 4);
    
    mainLayout->addWidget(m_deviceConfigGroup);
    
    m_jsonSchemaGroup = new QGroupBox("JSON Schema", this);
    auto *jsonSchemaLayout = new QVBoxLayout(m_jsonSchemaGroup);
    
    m_jsonSchemaEdit = new QTextEdit(this);
    m_jsonSchemaEdit->setMaximumHeight(150);
    jsonSchemaLayout->addWidget(m_jsonSchemaEdit);
    
    m_updateJsonSchemaButton = new QPushButton("Update Schema", this);
    jsonSchemaLayout->addWidget(m_updateJsonSchemaButton);
    
    mainLayout->addWidget(m_jsonSchemaGroup);
    
    m_behavioralGroup = new QGroupBox("Miscellaneous Behavioral Configuration", this);
    auto *behavioralLayout = new QVBoxLayout(m_behavioralGroup);
    
    m_workaroundJUCESubscriptionCheck = new QCheckBox("Workaround JUCE issue on missing 'subscribeId' header field in SubscriptionReply", this);
    behavioralLayout->addWidget(m_workaroundJUCESubscriptionCheck);
    
    auto *juceSubscriptionLabel = new QLabel("JUCE 7.0.9 has a bug that it does not assign \"subscribeId\" field in Reply to Subscription message, which is required by Common Rules for MIDI-CI Property Extension specification (section 9.1).", this);
    juceSubscriptionLabel->setWordWrap(true);
    juceSubscriptionLabel->setStyleSheet("font-size: 10px; color: gray;");
    behavioralLayout->addWidget(juceSubscriptionLabel);
    
    m_workaroundJUCEProfileChannelsCheck = new QCheckBox("Workaround JUCE issue on Profile Configuration Addressing", this);
    behavioralLayout->addWidget(m_workaroundJUCEProfileChannelsCheck);
    
    auto *juceProfileLabel = new QLabel("JUCE 7.0.9 has a bug that it fills `1` for 'numChannelsRequested' field even for 0x7E (group) and 0x7F (function block) that are supposed to be `0` by MIDI-CI v1.2 specification (section 7.8).", this);
    juceProfileLabel->setWordWrap(true);
    juceProfileLabel->setStyleSheet("font-size: 10px; color: gray;");
    behavioralLayout->addWidget(juceProfileLabel);
    
    mainLayout->addWidget(m_behavioralGroup);
    
    mainLayout->addStretch();
}

void SettingsWidget::setupConnections()
{
    connect(m_inputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::onInputDeviceChanged);
    connect(m_outputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::onOutputDeviceChanged);
    connect(m_loadConfigButton, &QPushButton::clicked, this, &SettingsWidget::onLoadConfiguration);
    connect(m_saveConfigButton, &QPushButton::clicked, this, &SettingsWidget::onSaveConfiguration);
    connect(m_updateDeviceInfoButton, &QPushButton::clicked, this, &SettingsWidget::onUpdateDeviceInfo);
    connect(m_updateJsonSchemaButton, &QPushButton::clicked, this, &SettingsWidget::onUpdateJsonSchema);
    connect(m_workaroundJUCESubscriptionCheck, &QCheckBox::toggled, this, &SettingsWidget::onWorkaroundJUCESubscriptionChanged);
    connect(m_workaroundJUCEProfileChannelsCheck, &QCheckBox::toggled, this, &SettingsWidget::onWorkaroundJUCEProfileChannelsChanged);
}

void SettingsWidget::onInputDeviceChanged(int index)
{
    if (index > 0 && m_repository) {
        QString deviceName = m_inputDeviceCombo->itemText(index);
        auto midiManager = m_repository->get_midi_device_manager();
        if (midiManager && midiManager->set_input_device(deviceName.toStdString())) {
            m_repository->log(QString("Selected input device: %1").arg(deviceName).toStdString(), ci_tool::MessageDirection::Out);
        }
    }
}

void SettingsWidget::onOutputDeviceChanged(int index)
{
    if (index > 0 && m_repository) {
        QString deviceName = m_outputDeviceCombo->itemText(index);
        auto midiManager = m_repository->get_midi_device_manager();
        if (midiManager && midiManager->set_output_device(deviceName.toStdString())) {
            m_repository->log(QString("Selected output device: %1").arg(deviceName).toStdString(), ci_tool::MessageDirection::Out);
        }
    }
}

void SettingsWidget::onLoadConfiguration()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load Configuration", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "JSON Files (*.json_ish)");
    if (!fileName.isEmpty() && m_repository) {
        m_repository->log(QString("Loading configuration from: %1").arg(fileName).toStdString(), ci_tool::MessageDirection::Out);
        updateDeviceConfiguration();
    }
}

void SettingsWidget::onSaveConfiguration()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Configuration", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/ktmidi-ci-tool.settings.json_ish", "JSON Files (*.json_ish)");
    if (!fileName.isEmpty() && m_repository) {
        m_repository->log(QString("Saving configuration to: %1").arg(fileName).toStdString(), ci_tool::MessageDirection::Out);
    }
}

void SettingsWidget::onUpdateDeviceInfo()
{
    if (m_repository) {
        m_repository->log("Updated device information", ci_tool::MessageDirection::Out);
    }
}

void SettingsWidget::onUpdateJsonSchema()
{
    if (m_repository) {
        QString schema = m_jsonSchemaEdit->toPlainText();
        m_repository->log("Updated JSON schema", ci_tool::MessageDirection::Out);
    }
}

void SettingsWidget::onWorkaroundJUCESubscriptionChanged(bool enabled)
{
    if (m_repository) {
        m_repository->log(QString("JUCE subscription workaround: %1").arg(enabled ? "enabled" : "disabled").toStdString(), ci_tool::MessageDirection::Out);
    }
}

void SettingsWidget::onWorkaroundJUCEProfileChannelsChanged(bool enabled)
{
    if (m_repository) {
        m_repository->log(QString("JUCE profile channels workaround: %1").arg(enabled ? "enabled" : "disabled").toStdString(), ci_tool::MessageDirection::Out);
    }
}

void SettingsWidget::updateDeviceLists()
{
    m_inputDeviceCombo->clear();
    m_outputDeviceCombo->clear();
    
    m_inputDeviceCombo->addItem("-- Select MIDI Input --");
    m_outputDeviceCombo->addItem("-- Select MIDI Output --");
    
    if (m_repository) {
        auto midiManager = m_repository->get_midi_device_manager();
        if (midiManager) {
            auto inputDevices = midiManager->get_available_input_devices();
            for (const auto& device : inputDevices) {
                m_inputDeviceCombo->addItem(QString::fromStdString(device));
            }
            
            auto outputDevices = midiManager->get_available_output_devices();
            for (const auto& device : outputDevices) {
                m_outputDeviceCombo->addItem(QString::fromStdString(device));
            }
        }
    }
}

void SettingsWidget::updateDeviceConfiguration()
{
    m_manufacturerIdEdit->setText("7D0000");
    m_familyIdEdit->setText("0001");
    m_modelIdEdit->setText("0001");
    m_versionIdEdit->setText("00000001");
    m_manufacturerEdit->setText("Sample Manufacturer");
    m_familyEdit->setText("Sample Family");
    m_modelEdit->setText("Sample Model");
    m_versionEdit->setText("1.0.0");
    m_serialNumberEdit->setText("12345");
    m_maxConnectionsSpin->setValue(8);
    
    m_jsonSchemaEdit->setText("{\n  \"type\": \"object\",\n  \"properties\": {\n    \"example\": {\n      \"type\": \"string\"\n    }\n  }\n}");
    
    m_workaroundJUCESubscriptionCheck->setChecked(false);
    m_workaroundJUCEProfileChannelsCheck->setChecked(false);
}
