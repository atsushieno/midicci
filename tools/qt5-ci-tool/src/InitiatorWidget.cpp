#include "InitiatorWidget.hpp"
#include "CIToolRepository.hpp"
#include "CIDeviceModel.hpp"
#include "AppModel.hpp"
#include "midicci/properties/ObservablePropertyList.hpp"

#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QHeaderView>


InitiatorWidget::InitiatorWidget(ci_tool::CIToolRepository* repository, QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
    , m_selectedDeviceMUID(0)
    , m_lastConnectionCount(0)
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
    
    auto *propertyButtonLayout = new QHBoxLayout();
    m_refreshPropertyButton = new QPushButton("Refresh", this);
    m_subscribePropertyButton = new QPushButton("Subscribe", this);
    m_propertyEncodingSelector = new QComboBox(this);
    
    propertyButtonLayout->addWidget(m_refreshPropertyButton);
    propertyButtonLayout->addWidget(m_subscribePropertyButton);
    propertyButtonLayout->addWidget(new QLabel("Encoding:", this));
    propertyButtonLayout->addWidget(m_propertyEncodingSelector);
    propertyButtonLayout->addStretch();
    
    propertyDetailsLayout->addLayout(propertyButtonLayout);
    
    m_propertyValueEdit = new QTextEdit(this);
    m_propertyValueEdit->setReadOnly(true);
    propertyDetailsLayout->addWidget(m_propertyValueEdit);
    
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
            m_repository->log("Sending discovery inquiry", ci_tool::MessageDirection::Out);
        }
    }
}

void InitiatorWidget::onDeviceSelectionChanged(int index)
{
    if (index > 0) {
        m_selectedDeviceMUID = m_deviceSelector->itemData(index).toInt();
        updateConnectionInfo();
        updateProfileList();
        updatePropertyList();
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
        m_propertyValueEdit->setText(QString("Selected property: %1").arg(m_selectedProperty));
    } else {
        m_selectedProperty.clear();
        m_propertyValueEdit->clear();
    }
}

void InitiatorWidget::onRefreshProperty()
{
    if (!m_selectedProperty.isEmpty()) {
        m_repository->log(QString("Refreshing property: %1").arg(m_selectedProperty).toStdString(), ci_tool::MessageDirection::Out);
    }
}

void InitiatorWidget::onSubscribeProperty()
{
    if (!m_selectedProperty.isEmpty()) {
        bool isSubscribed = m_subscribePropertyButton->text() == "Unsubscribe";
        if (isSubscribed) {
            m_subscribePropertyButton->setText("Subscribe");
            m_repository->log(QString("Unsubscribing from property: %1").arg(m_selectedProperty).toStdString(), ci_tool::MessageDirection::Out);
        } else {
            m_subscribePropertyButton->setText("Unsubscribe");
            m_repository->log(QString("Subscribing to property: %1").arg(m_selectedProperty).toStdString(), ci_tool::MessageDirection::Out);
        }
    }
}

void InitiatorWidget::onRequestMidiMessageReport()
{
    if (m_selectedDeviceMUID != 0) {
        uint8_t address = m_midiReportAddressSelector->currentData().toUInt();
        m_repository->log(QString("Requesting MIDI Message Report for address %1").arg(address).toStdString(), ci_tool::MessageDirection::Out);
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
    std::shared_ptr<ci_tool::ClientConnectionModel> targetConnection = nullptr;
    
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
    std::shared_ptr<ci_tool::ClientConnectionModel> targetConnection = nullptr;
    
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
    std::shared_ptr<ci_tool::ClientConnectionModel> targetConnection = nullptr;
    
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
            m_propertyList->addItem(QString::fromStdString(property.id));
        }
        
        if (properties_vec.empty()) {
            auto metadata = targetConnection->get_metadata_list();
            for (const auto& meta : metadata) {
                m_propertyList->addItem(QString::fromStdString(meta->getResourceId()));
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
    
    deviceModel->add_properties_updated_callback([this]() {
        QMetaObject::invokeMethod(this, [this]() {
            if (m_selectedDeviceMUID != 0) {
                updatePropertyList();
            }
            emit propertiesUpdated(m_selectedDeviceMUID);
        }, Qt::QueuedConnection);
    });
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
                        observable_properties->addPropertyUpdatedCallback([this](const std::string& propertyId) {
                            QMetaObject::invokeMethod(this, [this]() {
                                updatePropertyList();
                            }, Qt::QueuedConnection);
                        });
                        
                        observable_properties->addPropertyCatalogUpdatedCallback([this]() {
                            QMetaObject::invokeMethod(this, [this]() {
                                updatePropertyList();
                            }, Qt::QueuedConnection);
                        });
                    }
                }
                break;
            }
        }
    }
}
