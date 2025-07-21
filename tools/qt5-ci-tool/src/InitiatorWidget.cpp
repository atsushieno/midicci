#include "InitiatorWidget.hpp"
#include <midicci/tooling/CIToolRepository.hpp>
#include <midicci/tooling/CIDeviceModel.hpp>
#include "AppModel.hpp"
#include <midicci/midicci.hpp>
#include <midicci/midicci.hpp>

#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QHeaderView>

namespace midicci::tooling::qt5 {

InitiatorWidget::InitiatorWidget(tooling::CIToolRepository* repository, QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
    , m_selectedDeviceMUID(0)
    , m_lastConnectionCount(0)
    , m_propertyCallbacksSetup(false)
    , m_lastRequestedProperty("")
{
    setupUI();
    setupConnections();
    updateDeviceList();
}

void InitiatorWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    
    auto *topLayout = new QHBoxLayout();
    m_sendDiscoveryButton = new QPushButton("Send Discovery", this);
    m_deviceSelector = new QComboBox(this);
    m_deviceSelector->addItem("-- Select CI Device --", 0);
    
    topLayout->addWidget(m_sendDiscoveryButton);
    topLayout->addWidget(new QLabel("Device:", this));
    topLayout->addWidget(m_deviceSelector);
    topLayout->addStretch();
    
    mainLayout->addLayout(topLayout);
    
    m_deviceInfoGroup = new QGroupBox("Device Information", this);
    auto *deviceInfoLayout = new QGridLayout(m_deviceInfoGroup);
    
    m_muidLabel = new QLabel("--", this);
    m_manufacturerLabel = new QLabel("--", this);
    m_familyLabel = new QLabel("--", this);
    m_modelLabel = new QLabel("--", this);
    m_versionLabel = new QLabel("--", this);
    m_serialLabel = new QLabel("--", this);
    m_maxConnectionsLabel = new QLabel("--", this);
    
    deviceInfoLayout->addWidget(new QLabel("MUID:"), 0, 0);
    deviceInfoLayout->addWidget(m_muidLabel, 0, 1);
    deviceInfoLayout->addWidget(new QLabel("Manufacturer:"), 0, 2);
    deviceInfoLayout->addWidget(m_manufacturerLabel, 0, 3);
    deviceInfoLayout->addWidget(new QLabel("Family:"), 1, 0);
    deviceInfoLayout->addWidget(m_familyLabel, 1, 1);
    deviceInfoLayout->addWidget(new QLabel("Model:"), 1, 2);
    deviceInfoLayout->addWidget(m_modelLabel, 1, 3);
    deviceInfoLayout->addWidget(new QLabel("Version:"), 2, 0);
    deviceInfoLayout->addWidget(m_versionLabel, 2, 1);
    deviceInfoLayout->addWidget(new QLabel("Serial:"), 2, 2);
    deviceInfoLayout->addWidget(m_serialLabel, 2, 3);
    deviceInfoLayout->addWidget(new QLabel("Max Connections:"), 3, 0);
    deviceInfoLayout->addWidget(m_maxConnectionsLabel, 3, 1);
    
    mainLayout->addWidget(m_deviceInfoGroup);
    
    auto *profileGroup = new QGroupBox("Profiles", this);
    auto *profileLayout = new QVBoxLayout(profileGroup);
    
    m_profileSplitter = new QSplitter(Qt::Horizontal, this);
    
    m_profileList = new QListWidget(this);
    m_profileList->setMaximumWidth(200);
    m_profileSplitter->addWidget(m_profileList);
    
    m_profileDetailsWidget = new QWidget(this);
    auto *profileDetailsLayout = new QVBoxLayout(m_profileDetailsWidget);
    
    auto *detailsInquiryLayout = new QHBoxLayout();
    detailsInquiryLayout->addWidget(new QLabel("Channel:", this));
    m_profileAddressSelector = new QComboBox(this);
    m_profileAddressSelector->addItem("Function Block", 0x7F);
    m_profileAddressSelector->addItem("Group", 0x7E);
    for (int i = 0; i < 16; ++i) {
        m_profileAddressSelector->addItem(QString::number(i), i);
    }
    detailsInquiryLayout->addWidget(m_profileAddressSelector);
    
    detailsInquiryLayout->addWidget(new QLabel("Target:", this));
    m_profileTargetEdit = new QLineEdit("0", this);
    m_profileTargetEdit->setMaximumWidth(60);
    detailsInquiryLayout->addWidget(m_profileTargetEdit);
    
    m_sendProfileDetailsButton = new QPushButton("Send Details Inquiry", this);
    detailsInquiryLayout->addWidget(m_sendProfileDetailsButton);
    detailsInquiryLayout->addStretch();
    
    profileDetailsLayout->addLayout(detailsInquiryLayout);
    
    m_profileConfigTable = new QTableWidget(0, 4, this);
    m_profileConfigTable->setHorizontalHeaderLabels({"Group", "Address", "Enabled", "Channels"});
    m_profileConfigTable->horizontalHeader()->setStretchLastSection(true);
    profileDetailsLayout->addWidget(m_profileConfigTable);
    
    m_profileSplitter->addWidget(m_profileDetailsWidget);
    profileLayout->addWidget(m_profileSplitter);
    
    mainLayout->addWidget(profileGroup);
    
    auto *propertyGroup = new QGroupBox("Properties", this);
    auto *propertyLayout = new QVBoxLayout(propertyGroup);
    
    m_propertySplitter = new QSplitter(Qt::Horizontal, this);
    
    m_propertyList = new QListWidget(this);
    m_propertyList->setMaximumWidth(200);
    m_propertySplitter->addWidget(m_propertyList);
    
    m_propertyDetailsWidget = new QWidget(this);
    auto *propertyDetailsLayout = new QVBoxLayout(m_propertyDetailsWidget);
    
    m_propertyMetadataLabel = new QLabel("No property selected", this);
    m_propertyMetadataLabel->setWordWrap(true);
    propertyDetailsLayout->addWidget(m_propertyMetadataLabel);
    
    auto *editModeLayout = new QHBoxLayout();
    m_propertyEditCheckbox = new QCheckBox("Edit mode", this);
    editModeLayout->addWidget(m_propertyEditCheckbox);
    editModeLayout->addStretch();
    propertyDetailsLayout->addLayout(editModeLayout);
    
    auto *resIdLayout = new QHBoxLayout();
    resIdLayout->addWidget(new QLabel("Resource ID (if applicable):", this));
    m_propertyResIdEdit = new QLineEdit(this);
    resIdLayout->addWidget(m_propertyResIdEdit);
    propertyDetailsLayout->addLayout(resIdLayout);
    
    m_propertyValueEdit = new QTextEdit(this);
    m_propertyValueEdit->setMinimumHeight(100);
    propertyDetailsLayout->addWidget(m_propertyValueEdit);
    
    m_propertyPartialEdit = new QTextEdit(this);
    m_propertyPartialEdit->setPlaceholderText("RFC6901 JSON Pointer for partial updates (leave empty for full update)");
    m_propertyPartialEdit->setMaximumHeight(60);
    propertyDetailsLayout->addWidget(m_propertyPartialEdit);
    
    auto *propertyButtonLayout = new QHBoxLayout();
    m_refreshPropertyButton = new QPushButton("Refresh", this);
    m_subscribePropertyButton = new QPushButton("Subscribe", this);
    m_propertyCommitButton = new QPushButton("Commit Changes", this);
    
    propertyButtonLayout->addWidget(m_refreshPropertyButton);
    propertyButtonLayout->addWidget(m_subscribePropertyButton);
    propertyButtonLayout->addWidget(m_propertyCommitButton);
    propertyButtonLayout->addWidget(new QLabel("Encoding:", this));
    
    m_propertyEncodingSelector = new QComboBox(this);
    m_propertyEncodingSelector->addItem("", "");
    m_propertyEncodingSelector->addItem("ASCII", "ASCII");
    m_propertyEncodingSelector->addItem("Mcoded7", "Mcoded7");
    m_propertyEncodingSelector->addItem("zlib+Mcoded7", "zlib+Mcoded7");
    propertyButtonLayout->addWidget(m_propertyEncodingSelector);
    propertyButtonLayout->addStretch();
    propertyDetailsLayout->addLayout(propertyButtonLayout);
    
    m_propertyPaginationGroup = new QGroupBox("Pagination", this);
    auto *paginationLayout = new QHBoxLayout(m_propertyPaginationGroup);
    paginationLayout->addWidget(new QLabel("Offset:", this));
    m_propertyPaginateOffsetEdit = new QLineEdit("0", this);
    m_propertyPaginateOffsetEdit->setMaximumWidth(80);
    paginationLayout->addWidget(m_propertyPaginateOffsetEdit);
    paginationLayout->addWidget(new QLabel("Limit:", this));
    m_propertyPaginateLimitEdit = new QLineEdit("9999", this);
    m_propertyPaginateLimitEdit->setMaximumWidth(80);
    paginationLayout->addWidget(m_propertyPaginateLimitEdit);
    paginationLayout->addStretch();
    m_propertyPaginationGroup->setVisible(false);
    propertyDetailsLayout->addWidget(m_propertyPaginationGroup);
    
    m_propertySplitter->addWidget(m_propertyDetailsWidget);
    propertyLayout->addWidget(m_propertySplitter);
    
    mainLayout->addWidget(propertyGroup);
    
    m_processInquiryGroup = new QGroupBox("Process Inquiry", this);
    auto *processInquiryLayout = new QHBoxLayout(m_processInquiryGroup);
    
    processInquiryLayout->addWidget(new QLabel("Channel:", this));
    m_midiReportAddressSelector = new QComboBox(this);
    m_midiReportAddressSelector->addItem("Function Block", 0x7F);
    m_midiReportAddressSelector->addItem("Group", 0x7E);
    for (int i = 0; i < 16; ++i) {
        m_midiReportAddressSelector->addItem(QString::number(i), i);
    }
    processInquiryLayout->addWidget(m_midiReportAddressSelector);
    
    m_requestMidiReportButton = new QPushButton("Request MIDI Message Report", this);
    processInquiryLayout->addWidget(m_requestMidiReportButton);
    processInquiryLayout->addStretch();
    
    mainLayout->addWidget(m_processInquiryGroup);
}

void InitiatorWidget::setupConnections()
{
    connect(m_sendDiscoveryButton, &QPushButton::clicked, this, &InitiatorWidget::onSendDiscovery);
    connect(m_deviceSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &InitiatorWidget::onDeviceSelectionChanged);
    connect(m_profileList, &QListWidget::currentRowChanged, this, &InitiatorWidget::onProfileSelectionChanged);
    connect(m_propertyList, &QListWidget::currentRowChanged, this, &InitiatorWidget::onPropertySelectionChanged);
    connect(m_refreshPropertyButton, &QPushButton::clicked, this, &InitiatorWidget::onRefreshProperty);
    connect(m_subscribePropertyButton, &QPushButton::clicked, this, &InitiatorWidget::onSubscribeProperty);
    connect(m_requestMidiReportButton, &QPushButton::clicked, this, &InitiatorWidget::onRequestMidiMessageReport);
    connect(m_propertyEditCheckbox, &QCheckBox::toggled, this, &InitiatorWidget::onPropertyEditModeChanged);
    connect(m_propertyCommitButton, &QPushButton::clicked, this, &InitiatorWidget::onPropertyCommitChanges);
    connect(m_propertyValueEdit, &QTextEdit::textChanged, this, &InitiatorWidget::onPropertyValueTextChanged);
    
    m_propertyEditingMode.set_value_changed_handler([this](bool editing) {
        m_propertyEditCheckbox->setChecked(editing);
        m_propertyValueEdit->setReadOnly(!editing);
        m_propertyPartialEdit->setVisible(editing);
        m_propertyResIdEdit->setVisible(editing);
        m_propertyCommitButton->setVisible(editing);
    });
    
    m_propertyValueText.set_value_changed_handler([this](const QString& text) {
        if (m_propertyValueEdit->toPlainText() != text) {
            m_propertyValueEdit->setPlainText(text);
        }
    });
    connect(m_propertyEditCheckbox, &QCheckBox::toggled, this, &InitiatorWidget::onPropertyEditModeChanged);
    connect(m_propertyCommitButton, &QPushButton::clicked, this, &InitiatorWidget::onPropertyCommitChanges);
    connect(m_propertyValueEdit, &QTextEdit::textChanged, this, &InitiatorWidget::onPropertyValueTextChanged);
    
    m_propertyEditingMode.set_value_changed_handler([this](bool editing) {
        m_propertyEditCheckbox->setChecked(editing);
        m_propertyValueEdit->setReadOnly(!editing);
        m_propertyPartialEdit->setVisible(editing);
        m_propertyResIdEdit->setVisible(editing);
        m_propertyCommitButton->setVisible(editing);
    });
    
    m_propertyValueText.set_value_changed_handler([this](const QString& text) {
        if (m_propertyValueEdit->toPlainText() != text) {
            m_propertyValueEdit->setPlainText(text);
        }
    });
    connect(m_propertyEditCheckbox, &QCheckBox::toggled, this, &InitiatorWidget::onPropertyEditModeChanged);
    connect(m_propertyCommitButton, &QPushButton::clicked, this, &InitiatorWidget::onPropertyCommitChanges);
    connect(m_propertyValueEdit, &QTextEdit::textChanged, this, &InitiatorWidget::onPropertyValueTextChanged);
    
    m_propertyEditingMode.set_value_changed_handler([this](bool editing) {
        m_propertyEditCheckbox->setChecked(editing);
        m_propertyValueEdit->setReadOnly(!editing);
        m_propertyPartialEdit->setVisible(editing);
        m_propertyResIdEdit->setVisible(editing);
        m_propertyCommitButton->setVisible(editing);
    });
    
    m_propertyValueText.set_value_changed_handler([this](const QString& text) {
        if (m_propertyValueEdit->toPlainText() != text) {
            m_propertyValueEdit->setPlainText(text);
        }
    });
    

    
    connect(this, &InitiatorWidget::deviceConnected, this, [this](int muid) {
        updateDeviceList();
        updateConnectionInfo();
    });
    
    connect(this, &InitiatorWidget::deviceDisconnected, this, [this](int muid) {
        updateDeviceList();
    });
    
    connect(this, &InitiatorWidget::deviceInfoUpdated, this, [this](int muid) {
        if (muid == m_selectedDeviceMUID) {
            updateConnectionInfo();
        }
    });
    
    connect(this, &InitiatorWidget::profilesUpdated, this, [this](int muid) {
        if (muid == m_selectedDeviceMUID) {
            updateProfileList();
        }
    });
    
    connect(this, &InitiatorWidget::propertiesUpdated, this, [this](int muid) {
        if (muid == m_selectedDeviceMUID) {
            updatePropertyList();
        }
    });
    
    setupEventBridge();
}

void InitiatorWidget::onSendDiscovery()
{
    if (m_repository && m_repository->get_ci_device_manager()) {
        auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
        if (deviceModel) {
            deviceModel->send_discovery();
            m_repository->log("Sending discovery inquiry", tooling::MessageDirection::Out);
        }
    }
}

void InitiatorWidget::onDeviceSelectionChanged(int index)
{
    if (index > 0) {
        m_selectedDeviceMUID = m_deviceSelector->itemData(index).toInt();
        m_propertyCallbacksSetup = false; // Reset callback setup flag for new device
        m_lastRequestedProperty = ""; // Reset last requested property for new device
        updateConnectionInfo();
        updateProfileList();
        updatePropertyList();
        setupPropertyCallbacks();
    } else {
        m_selectedDeviceMUID = 0;
        m_muidLabel->setText("--");
        m_manufacturerLabel->setText("--");
        m_familyLabel->setText("--");
        m_modelLabel->setText("--");
        m_versionLabel->setText("--");
        m_serialLabel->setText("--");
        m_maxConnectionsLabel->setText("--");
    }
}

void InitiatorWidget::onProfileSelectionChanged()
{
    auto *item = m_profileList->currentItem();
    if (item) {
        m_selectedProfile = item->text();
    } else {
        m_selectedProfile.clear();
    }
}

void InitiatorWidget::onPropertySelectionChanged()
{
    auto *item = m_propertyList->currentItem();
    if (item) {
        m_selectedProperty = item->text();
        
        // Send GetPropertyData request to fetch current value
        sendGetPropertyDataRequest();
        
        // Display cached property data and metadata while waiting for the response
        if (m_repository && m_repository->get_ci_device_manager()) {
            auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
            if (deviceModel) {
                auto& connections = deviceModel->get_connections();
                for (const auto& connection : connections) {
                    if (connection && connection->get_connection()) {
                        uint32_t connectionMuid = connection->get_connection()->get_target_muid();
                        if (static_cast<int>(connectionMuid) == m_selectedDeviceMUID) {
                            // Display cached property value (if any)
                            const auto& properties = connection->get_properties();
                            auto properties_vec = properties.to_vector();
                            
                            bool foundCachedValue = false;
                            for (const auto& property : properties_vec) {
                                if (property.id == m_selectedProperty.toStdString()) {
                                    QString propertyText = QString::fromStdString(
                                        std::string(property.body.begin(), property.body.end()));
                                    m_propertyValueText.set(propertyText);
                                    foundCachedValue = true;
                                    break;
                                }
                            }
                            
                            // If no cached value, show loading indicator
                            if (!foundCachedValue) {
                                m_propertyValueText.set("Loading property value...");
                            }
                            
                            // Display property metadata
                            auto metadata = connection->get_metadata_list();
                            for (const auto& meta : metadata) {
                                if (meta->getResourceId() == m_selectedProperty.toStdString()) {
                                    QString metadataText = QString("Property: %1\nMedia Type: %2\nCan Set: %3\nCan Subscribe: %4\nCan Paginate: %5")
                                        .arg(QString::fromStdString(meta->getResourceId()))
                                        .arg(QString::fromStdString(meta->getMediaType()))
                                        .arg(QString::fromStdString("Unknown"))
                                        .arg(QString::fromStdString("Unknown"))
                                        .arg(QString::fromStdString("Unknown"));
                                    
                                    auto* commonMeta = dynamic_cast<const midicci::commonproperties::CommonRulesPropertyMetadata*>(meta.get());
                                    if (commonMeta) {
                                        metadataText = QString("Property: %1\nMedia Type: %2\nCan Set: %3\nCan Subscribe: %4\nCan Paginate: %5")
                                            .arg(QString::fromStdString(meta->getResourceId()))
                                            .arg(QString::fromStdString(meta->getMediaType()))
                                            .arg(QString::fromStdString(commonMeta->canSet))
                                            .arg(commonMeta->canSubscribe ? "Yes" : "No")
                                            .arg(commonMeta->canPaginate ? "Yes" : "No");
                                        
                                        m_propertyPaginationGroup->setVisible(commonMeta->canPaginate);
                                    } else {
                                        m_propertyPaginationGroup->setVisible(false);
                                    }
                                    
                                    m_propertyMetadataLabel->setText(metadataText);
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
        
        m_propertyEditingMode.set(false);
        m_propertyResId.set("");
        m_propertySelectedEncoding.set("");
    } else {
        m_selectedProperty.clear();
        m_propertyValueText.set("");
        m_propertyMetadataLabel->setText("No property selected");
        m_propertyPaginationGroup->setVisible(false);
    }
}

void InitiatorWidget::onRefreshProperty()
{
    if (m_selectedProperty.isEmpty() || m_selectedDeviceMUID == 0) {
        return;
    }
    
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    auto& connections = deviceModel->get_connections();
    for (const auto& connection : connections) {
        if (connection && connection->get_connection()) {
            uint32_t connectionMuid = connection->get_connection()->get_target_muid();
            if (static_cast<int>(connectionMuid) == m_selectedDeviceMUID) {
                auto conn = connection->get_connection();
                if (conn) {
                    auto& property_facade = conn->get_property_client_facade();
                    
                    QString encoding = m_propertyEncodingSelector->currentData().toString();
                    int offset = m_propertyPaginateOffsetEdit->text().toInt();
                    int limit = m_propertyPaginateLimitEdit->text().toInt();
                    
                    if (m_propertyPaginationGroup->isVisible()) {
                        property_facade.send_get_property_data(
                            m_selectedProperty.toStdString(),
                            "",  // res_id - empty for now
                            encoding.toStdString(),
                            offset,
                            limit
                        );
                    } else {
                        property_facade.send_get_property_data(
                            m_selectedProperty.toStdString(),
                            "",  // res_id - empty for now
                            encoding.toStdString()
                        );
                    }
                    
                    m_repository->log(QString("Refreshing property: %1 with encoding: %2")
                        .arg(m_selectedProperty)
                        .arg(encoding.isEmpty() ? "default" : encoding).toStdString(),
                                      tooling::MessageDirection::Out);
                    
                    // Note: The property value will be updated via the callback when GetPropertyDataReply arrives
                }
                break;
            }
        }
    }
}

void InitiatorWidget::onSubscribeProperty()
{
    if (m_selectedProperty.isEmpty() || m_selectedDeviceMUID == 0) {
        return;
    }
    
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    auto& connections = deviceModel->get_connections();
    for (const auto& connection : connections) {
        if (connection && connection->get_connection()) {
            uint32_t connectionMuid = connection->get_connection()->get_target_muid();
            if (static_cast<int>(connectionMuid) == m_selectedDeviceMUID) {
                auto conn = connection->get_connection();
                if (conn) {
                    auto& property_facade = conn->get_property_client_facade();
                    
                    bool isSubscribed = m_subscribePropertyButton->text() == "Unsubscribe";
                    QString encoding = m_propertyEncodingSelector->currentData().toString();
                    
                    if (isSubscribed) {
                        property_facade.send_unsubscribe_property(m_selectedProperty.toStdString(), "");
                        m_subscribePropertyButton->setText("Subscribe");
                        m_repository->log(QString("Unsubscribing from property: %1")
                            .arg(m_selectedProperty).toStdString(), tooling::MessageDirection::Out);
                    } else {
                        property_facade.send_subscribe_property(
                            m_selectedProperty.toStdString(),
                            "",  // res_id - empty for now
                            encoding.toStdString()
                        );
                        m_subscribePropertyButton->setText("Unsubscribe");
                        m_repository->log(QString("Subscribing to property: %1 with encoding: %2")
                            .arg(m_selectedProperty)
                            .arg(encoding.isEmpty() ? "default" : encoding).toStdString(),
                                          tooling::MessageDirection::Out);
                    }
                }
                break;
            }
        }
    }
}

void InitiatorWidget::onRequestMidiMessageReport()
{
    if (m_selectedDeviceMUID != 0) {
        uint8_t address = m_midiReportAddressSelector->currentData().toUInt();
        m_repository->log(QString("Requesting MIDI Message Report for address %1").arg(address).toStdString(), tooling::MessageDirection::Out);
    }
}

void InitiatorWidget::updateDeviceList()
{
    m_deviceSelector->clear();
    m_deviceSelector->addItem("-- Select CI Device --", 0);
    
    if (m_repository && m_repository->get_ci_device_manager()) {
        auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
        if (deviceModel) {
            const auto& connections = deviceModel->get_connections();
            auto connections_vec = connections.to_vector();
            for (const auto& connection : connections_vec) {
                if (connection && connection->get_connection()) {
                    uint32_t muid = connection->get_connection()->get_target_muid();
                    QString deviceName = QString("Device 0x%1").arg(muid, 8, 16, QChar('0'));
                    m_deviceSelector->addItem(deviceName, static_cast<int>(muid));
                }
            }
        }
    }
}

void InitiatorWidget::updateConnectionInfo()
{
    if (m_selectedDeviceMUID == 0) {
        m_muidLabel->setText("No device selected");
        m_manufacturerLabel->setText("--");
        m_familyLabel->setText("--");
        m_modelLabel->setText("--");
        m_versionLabel->setText("--");
        m_serialLabel->setText("--");
        m_maxConnectionsLabel->setText("--");
        return;
    }
    
    m_muidLabel->setText(QString("0x%1").arg(m_selectedDeviceMUID, 8, 16, QChar('0')));
    
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        m_manufacturerLabel->setText("--");
        m_familyLabel->setText("--");
        m_modelLabel->setText("--");
        m_versionLabel->setText("--");
        m_serialLabel->setText("--");
        m_maxConnectionsLabel->setText("--");
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        m_manufacturerLabel->setText("--");
        m_familyLabel->setText("--");
        m_modelLabel->setText("--");
        m_versionLabel->setText("--");
        m_serialLabel->setText("--");
        m_maxConnectionsLabel->setText("--");
        return;
    }
    
    const auto& connections = deviceModel->get_connections();
    auto connections_vec = connections.to_vector();
    std::shared_ptr<tooling::ClientConnectionModel> targetConnection = nullptr;
    
    for (const auto& connection : connections_vec) {
        if (connection && connection->get_connection()) {
            uint32_t connectionMuid = connection->get_connection()->get_target_muid();
            if (static_cast<int>(connectionMuid) == m_selectedDeviceMUID) {
                targetConnection = connection;
                break;
            }
        }
    }
    
    if (targetConnection && targetConnection->get_connection()) {
        auto deviceDetails = targetConnection->get_connection()->get_device_info();
        if (deviceDetails) {
            m_manufacturerLabel->setText(QString("0x%1").arg(deviceDetails->manufacturer_id, 6, 16, QChar('0')));
            m_familyLabel->setText(QString("0x%1").arg(deviceDetails->family_id, 4, 16, QChar('0')));
            m_modelLabel->setText(QString("0x%1").arg(deviceDetails->model_id, 4, 16, QChar('0')));
            m_versionLabel->setText(QString("0x%1").arg(deviceDetails->version_id, 8, 16, QChar('0')));
            m_serialLabel->setText("--");
            m_maxConnectionsLabel->setText("--");
        } else {
            m_manufacturerLabel->setText("Unknown");
            m_familyLabel->setText("Unknown");
            m_modelLabel->setText("Unknown");
            m_versionLabel->setText("Unknown");
            m_serialLabel->setText("--");
            m_maxConnectionsLabel->setText("--");
        }
    } else {
        m_manufacturerLabel->setText("Device not found");
        m_familyLabel->setText("--");
        m_modelLabel->setText("--");
        m_versionLabel->setText("--");
        m_serialLabel->setText("--");
        m_maxConnectionsLabel->setText("--");
    }
}

void InitiatorWidget::updateProfileList()
{
    m_profileList->clear();
    if (m_selectedDeviceMUID == 0) {
        return;
    }
    
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    const auto& connections = deviceModel->get_connections();
    auto connections_vec = connections.to_vector();
    std::shared_ptr<tooling::ClientConnectionModel> targetConnection = nullptr;
    
    for (const auto& connection : connections_vec) {
        if (connection && connection->get_connection()) {
            uint32_t connectionMuid = connection->get_connection()->get_target_muid();
            if (static_cast<int>(connectionMuid) == m_selectedDeviceMUID) {
                targetConnection = connection;
                break;
            }
        }
    }
    
    if (targetConnection) {
        const auto& profiles = targetConnection->get_profiles();
        auto profiles_vec = profiles.to_vector();
        for (const auto& profile : profiles_vec) {
            if (profile) {
                auto profileId = profile->get_profile();
                QString profileText = QString("%1 (G%2 A%3) %4")
                    .arg(QString::fromStdString(profileId.to_string()))
                    .arg(profile->group().get())
                    .arg(profile->address().get())
                    .arg(profile->enabled().get() ? "ON" : "OFF");
                m_profileList->addItem(profileText);
            }
        }
    }
}

void InitiatorWidget::updatePropertyList()
{
    // Save the currently selected property to restore it after updating the list
    QString previouslySelectedProperty = m_selectedProperty;
    
    m_propertyList->clear();
    if (m_selectedDeviceMUID == 0) {
        return;
    }
    
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    auto& connections = deviceModel->get_connections();
    std::shared_ptr<tooling::ClientConnectionModel> targetConnection = nullptr;
    
    for (const auto& connection : connections) {
        if (connection && connection->get_connection()) {
            uint32_t connectionMuid = connection->get_connection()->get_target_muid();
            if (static_cast<int>(connectionMuid) == m_selectedDeviceMUID) {
                targetConnection = connection;
                break;
            }
        }
    }
    
    if (targetConnection) {
        const auto& properties = targetConnection->get_properties();
        auto properties_vec = properties.to_vector();
        
        for (const auto& property : properties_vec) {
            // Filter out ResourceList - it's a meta-property that shouldn't be user-visible
            if (property.id != "ResourceList") {
                m_propertyList->addItem(QString::fromStdString(property.id));
            }
        }
        
        if (properties_vec.empty()) {
            auto metadata = targetConnection->get_metadata_list();
            for (const auto& meta : metadata) {
                // Filter out ResourceList from metadata as well
                if (meta->getResourceId() != "ResourceList") {
                    m_propertyList->addItem(QString::fromStdString(meta->getResourceId()));
                }
            }
        }
    }
    
    // Restore the previously selected property if it still exists in the updated list
    if (!previouslySelectedProperty.isEmpty()) {
        for (int i = 0; i < m_propertyList->count(); ++i) {
            if (m_propertyList->item(i)->text() == previouslySelectedProperty) {
                // Temporarily disconnect the selection change signal to prevent recursive calls
                disconnect(m_propertyList, &QListWidget::currentRowChanged, this, &InitiatorWidget::onPropertySelectionChanged);
                m_propertyList->setCurrentRow(i);
                m_selectedProperty = previouslySelectedProperty;
                // Reconnect the signal
                connect(m_propertyList, &QListWidget::currentRowChanged, this, &InitiatorWidget::onPropertySelectionChanged);
                break;
            }
        }
    }
}

void InitiatorWidget::setupEventBridge()
{
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    deviceModel->add_connections_changed_callback([this]() {
        QMetaObject::invokeMethod(this, [this]() {
            updateDeviceList();
            if (m_selectedDeviceMUID != 0) {
                m_propertyCallbacksSetup = false; // Reset callback setup flag when connections change
                updateConnectionInfo();
                updateProfileList();
                updatePropertyList();
                setupPropertyCallbacks();
            }
            emit deviceConnected(m_selectedDeviceMUID);
        }, Qt::QueuedConnection);
    });
    
    deviceModel->add_profiles_updated_callback([this]() {
        QMetaObject::invokeMethod(this, [this]() {
            if (m_selectedDeviceMUID != 0) {
                updateProfileList();
            }
            emit profilesUpdated(m_selectedDeviceMUID);
        }, Qt::QueuedConnection);
    });
    
    // Note: Device-level properties callback removed to prevent unnecessary property list rebuilds
    // Individual property updates are handled by the property-specific callbacks in setupPropertyCallbacks()
    // Only catalog-level changes (like ResourceList updates) should trigger full property list rebuilds
}

void InitiatorWidget::setupPropertyCallbacks()
{
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    // Only set up callbacks once per device selection to avoid duplicates
    if (m_propertyCallbacksSetup) {
        return;
    }
    
    const auto& connections = deviceModel->get_connections();
    auto connections_vec = connections.to_vector();
    
    for (const auto& connection : connections_vec) {
        if (connection && connection->get_connection()) {
            uint32_t connectionMuid = connection->get_connection()->get_target_muid();
            if (static_cast<int>(connectionMuid) == m_selectedDeviceMUID) {
                auto conn = connection->get_connection();
                if (conn) {
                    auto& property_facade = conn->get_property_client_facade();
                    auto* observable_properties = property_facade.get_properties();
                    
                    if (observable_properties) {
                        m_repository->log(QString("Setting up property callbacks for device MUID 0x%1")
                            .arg(m_selectedDeviceMUID, 8, 16, QChar('0')).toStdString(), 
                            tooling::MessageDirection::In);
                        
                        observable_properties->addPropertyUpdatedCallback([this](const std::string& propertyId) {
                            m_repository->log(QString("Property updated callback triggered for property: %1 (currently selected: %2)")
                                .arg(QString::fromStdString(propertyId))
                                .arg(m_selectedProperty).toStdString(), 
                                tooling::MessageDirection::In);
                            
                            QMetaObject::invokeMethod(this, [this, propertyId]() {
                                // Handle ResourceList updates (meta-property containing available properties catalog)
                                if (propertyId == "ResourceList") {
                                    m_repository->log("ResourceList property updated, refreshing property list", 
                                        tooling::MessageDirection::In);
                                    updatePropertyList();
                                    return; // ResourceList is not user-visible, so no need to update display
                                }
                                
                                // Update property value display if the updated property is currently selected
                                if (m_selectedProperty.toStdString() == propertyId) {
                                    m_repository->log(QString("Updating display for selected property: %1")
                                        .arg(QString::fromStdString(propertyId)).toStdString(), 
                                        tooling::MessageDirection::In);
                                    updateCurrentPropertyValue();
                                } else {
                                    m_repository->log(QString("Property %1 updated but not currently selected (%2), skipping UI update")
                                        .arg(QString::fromStdString(propertyId))
                                        .arg(m_selectedProperty).toStdString(), 
                                        tooling::MessageDirection::In);
                                }
                            }, Qt::QueuedConnection);
                        });
                        
                        observable_properties->addPropertyCatalogUpdatedCallback([this]() {
                            m_repository->log("Property catalog updated callback triggered", 
                                tooling::MessageDirection::In);
                            
                            QMetaObject::invokeMethod(this, [this]() {
                                updatePropertyList();
                            }, Qt::QueuedConnection);
                        });
                        
                        m_propertyCallbacksSetup = true;
                    }
                }
                break;
            }
        }
    }
}

void InitiatorWidget::onPropertyEditModeChanged(bool editing)
{
    m_propertyEditingMode.set(editing);
}

void InitiatorWidget::onPropertyValueTextChanged()
{
    if (!m_propertyEditingMode.get()) {
        return;
    }
    m_propertyValueText.set(m_propertyValueEdit->toPlainText());
}

void InitiatorWidget::onPropertyCommitChanges()
{
    if (m_selectedProperty.isEmpty() || m_selectedDeviceMUID == 0) {
        return;
    }
    
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    auto& connections = deviceModel->get_connections();
    for (const auto& connection : connections) {
        if (connection && connection->get_connection()) {
            uint32_t connectionMuid = connection->get_connection()->get_target_muid();
            if (static_cast<int>(connectionMuid) == m_selectedDeviceMUID) {
                auto conn = connection->get_connection();
                if (conn) {
                    auto& property_facade = conn->get_property_client_facade();
                    
                    QString propertyText = m_propertyValueEdit->toPlainText();
                    QString resId = m_propertyResIdEdit->text();
                    QString encoding = m_propertyEncodingSelector->currentData().toString();
                    QString partialContent = m_propertyPartialEdit->toPlainText();
                    bool isPartial = !partialContent.isEmpty();
                    
                    std::string textToSend = isPartial ? partialContent.toStdString() : propertyText.toStdString();
                    std::vector<uint8_t> data(textToSend.begin(), textToSend.end());
                    
                    property_facade.send_set_property_data(
                        m_selectedProperty.toStdString(),
                        resId.toStdString(),
                        data,
                        encoding.toStdString(),
                        isPartial
                    );
                    
                    m_repository->log(QString("Committing changes to property: %1 (partial: %2, encoding: %3)")
                        .arg(m_selectedProperty)
                        .arg(isPartial ? "yes" : "no")
                        .arg(encoding.isEmpty() ? "default" : encoding).toStdString(),
                                      tooling::MessageDirection::Out);
                    
                    m_propertyEditingMode.set(false);
                }
                break;
            }
        }
    }
}

void InitiatorWidget::updateCurrentPropertyValue()
{
    if (m_selectedProperty.isEmpty() || m_selectedDeviceMUID == 0) {
        return;
    }
    
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    const auto& connections = deviceModel->get_connections();
    for (const auto& connection : connections) {
        if (connection && connection->get_connection()) {
            uint32_t connectionMuid = connection->get_connection()->get_target_muid();
            if (static_cast<int>(connectionMuid) == m_selectedDeviceMUID) {
                // Try to get properties directly from the PropertyClientFacade first
                auto conn = connection->get_connection();
                if (conn) {
                    auto& property_facade = conn->get_property_client_facade();
                    auto* observable_properties = property_facade.get_properties();
                    
                    if (observable_properties) {
                        auto properties_vec = observable_properties->getValues();
                        
                        m_repository->log(QString("updateCurrentPropertyValue: Found %1 properties from PropertyClientFacade for device 0x%2, looking for '%3'")
                            .arg(properties_vec.size())
                            .arg(m_selectedDeviceMUID, 8, 16, QChar('0'))
                            .arg(m_selectedProperty)
                            .toStdString(), tooling::MessageDirection::In);
                        
                        for (const auto& property : properties_vec) {
                            if (property.id == m_selectedProperty.toStdString()) {
                                QString propertyText = QString::fromStdString(
                                    std::string(property.body.begin(), property.body.end()));
                                
                                m_repository->log(QString("updateCurrentPropertyValue: Found property '%1' via PropertyClientFacade, updating display with %2 bytes")
                                    .arg(QString::fromStdString(property.id))
                                    .arg(property.body.size())
                                    .toStdString(), tooling::MessageDirection::In);
                                
                                m_propertyValueText.set(propertyText);
                                
                                m_repository->log(QString("updateCurrentPropertyValue: Successfully updated UI for property '%1'")
                                    .arg(QString::fromStdString(property.id))
                                    .toStdString(), tooling::MessageDirection::In);
                                return; // Found and updated, exit the method
                            }
                        }
                    }
                }
                
                // Fallback: try getting properties from connection (original method)
                const auto& properties = connection->get_properties();
                auto properties_vec = properties.to_vector();
                
                m_repository->log(QString("updateCurrentPropertyValue: Fallback - Found %1 properties from connection for device 0x%2, looking for '%3'")
                    .arg(properties_vec.size())
                    .arg(m_selectedDeviceMUID, 8, 16, QChar('0'))
                    .arg(m_selectedProperty)
                    .toStdString(), tooling::MessageDirection::In);
                
                for (const auto& property : properties_vec) {
                    if (property.id == m_selectedProperty.toStdString()) {
                        QString propertyText = QString::fromStdString(
                            std::string(property.body.begin(), property.body.end()));
                        
                        m_repository->log(QString("updateCurrentPropertyValue: Found property '%1' via connection fallback, updating display with %2 bytes")
                            .arg(QString::fromStdString(property.id))
                            .arg(property.body.size())
                            .toStdString(), tooling::MessageDirection::In);
                        
                        m_propertyValueText.set(propertyText);
                        break;
                    }
                }
                break;
            }
        }
    }
}

void InitiatorWidget::sendGetPropertyDataRequest()
{
    if (m_selectedProperty.isEmpty() || m_selectedDeviceMUID == 0) {
        return;
    }
    
    // Avoid sending duplicate requests for the same property
    if (m_lastRequestedProperty == m_selectedProperty) {
        m_repository->log(QString("Skipping duplicate request for property: %1")
            .arg(m_selectedProperty).toStdString(), tooling::MessageDirection::In);
        return;
    }
    
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    auto& connections = deviceModel->get_connections();
    for (const auto& connection : connections) {
        if (connection && connection->get_connection()) {
            uint32_t connectionMuid = connection->get_connection()->get_target_muid();
            if (static_cast<int>(connectionMuid) == m_selectedDeviceMUID) {
                auto conn = connection->get_connection();
                if (conn) {
                    auto& property_facade = conn->get_property_client_facade();
                    
                    // Use the current encoding selection (if any) or default to empty
                    QString encoding = m_propertyEncodingSelector->currentData().toString();
                    
                    // Check if pagination is supported and visible
                    if (m_propertyPaginationGroup->isVisible()) {
                        int offset = m_propertyPaginateOffsetEdit->text().toInt();
                        int limit = m_propertyPaginateLimitEdit->text().toInt();
                        
                        property_facade.send_get_property_data(
                            m_selectedProperty.toStdString(),
                            "",  // res_id - empty for now
                            encoding.toStdString(),
                            offset,
                            limit
                        );
                        
                        m_repository->log(QString("Auto-requesting property data for: %1 (paginated: offset=%2, limit=%3, encoding=%4)")
                            .arg(m_selectedProperty)
                            .arg(offset)
                            .arg(limit)
                            .arg(encoding.isEmpty() ? "default" : encoding).toStdString(),
                                          tooling::MessageDirection::Out);
                        
                        m_lastRequestedProperty = m_selectedProperty;
                    } else {
                        property_facade.send_get_property_data(
                            m_selectedProperty.toStdString(),
                            "",  // res_id - empty for now
                            encoding.toStdString()
                        );
                        
                        m_repository->log(QString("Auto-requesting property data for: %1 (encoding=%2)")
                            .arg(m_selectedProperty)
                            .arg(encoding.isEmpty() ? "default" : encoding).toStdString(),
                                          tooling::MessageDirection::Out);
                        
                        m_lastRequestedProperty = m_selectedProperty;
                    }
                }
                break;
            }
        }
    }
}
}
