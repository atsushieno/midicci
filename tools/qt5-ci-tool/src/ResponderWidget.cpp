#include "ResponderWidget.hpp"
#include <midicci/tooling/CIToolRepository.hpp>
#include <midicci/tooling/CIDeviceModel.hpp>
#include "AppModel.hpp"
#include <midicci/ObservablePropertyList.hpp>
#include <midicci/PropertyHostFacade.hpp>
#include <midicci/commonproperties/CommonRulesPropertyMetadata.hpp>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>

namespace midicci::tooling::qt5 {

ResponderWidget::ResponderWidget(midicci::tooling::CIToolRepository* repository, QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
    , m_lastProfileCount(0)
    , m_lastPropertyCount(0)
{
    setupUI();
    setupConnections();
    updateProfileList();
    updatePropertyList();
}

void ResponderWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    
    auto *profileGroup = new QGroupBox("Profiles", this);
    auto *profileLayout = new QVBoxLayout(profileGroup);
    
    m_profileSplitter = new QSplitter(Qt::Horizontal, this);
    
    auto *profileListWidget = new QWidget(this);
    auto *profileListLayout = new QVBoxLayout(profileListWidget);
    
    m_profileList = new QListWidget(this);
    profileListLayout->addWidget(m_profileList);
    
    auto *profileButtonLayout = new QHBoxLayout();
    m_addProfileButton = new QPushButton("Add", this);
    m_editProfileButton = new QPushButton("Edit", this);
    m_deleteProfileButton = new QPushButton("Delete", this);
    m_addTestProfilesButton = new QPushButton("Add Test Items", this);
    
    profileButtonLayout->addWidget(m_addProfileButton);
    profileButtonLayout->addWidget(m_editProfileButton);
    profileButtonLayout->addWidget(m_deleteProfileButton);
    profileButtonLayout->addWidget(m_addTestProfilesButton);
    
    profileListLayout->addLayout(profileButtonLayout);
    profileListWidget->setMaximumWidth(300);
    m_profileSplitter->addWidget(profileListWidget);
    
    m_profileDetailsWidget = new QWidget(this);
    auto *profileDetailsLayout = new QVBoxLayout(m_profileDetailsWidget);
    
    profileDetailsLayout->addWidget(new QLabel("Profile Targets:", this));
    
    m_profileTargetsTable = new QTableWidget(0, 5, this);
    m_profileTargetsTable->setHorizontalHeaderLabels({"Enabled", "Group", "Address", "Channels", "Actions"});
    m_profileTargetsTable->horizontalHeader()->setStretchLastSection(true);
    profileDetailsLayout->addWidget(m_profileTargetsTable);
    
    m_addProfileTargetButton = new QPushButton("Add Target", this);
    profileDetailsLayout->addWidget(m_addProfileTargetButton);
    
    m_profileSplitter->addWidget(m_profileDetailsWidget);
    profileLayout->addWidget(m_profileSplitter);
    
    mainLayout->addWidget(profileGroup);
    
    auto *propertyGroup = new QGroupBox("Properties", this);
    auto *propertyLayout = new QVBoxLayout(propertyGroup);
    
    m_propertySplitter = new QSplitter(Qt::Horizontal, this);
    
    auto *propertyListWidget = new QWidget(this);
    auto *propertyListLayout = new QVBoxLayout(propertyListWidget);
    
    m_propertyList = new QListWidget(this);
    propertyListLayout->addWidget(m_propertyList);
    
    auto *propertyButtonLayout = new QHBoxLayout();
    m_addPropertyButton = new QPushButton("Add", this);
    m_deletePropertyButton = new QPushButton("Delete", this);
    
    propertyButtonLayout->addWidget(m_addPropertyButton);
    propertyButtonLayout->addWidget(m_deletePropertyButton);
    
    propertyListLayout->addLayout(propertyButtonLayout);
    propertyListWidget->setMaximumWidth(300);
    m_propertySplitter->addWidget(propertyListWidget);
    
    m_propertyDetailsWidget = new QWidget(this);
    auto *propertyDetailsLayout = new QVBoxLayout(m_propertyDetailsWidget);
    
    m_propertyValueGroup = new QGroupBox("Property Value", this);
    auto *propertyValueLayout = new QVBoxLayout(m_propertyValueGroup);
    
    m_propertyValueEdit = new QTextEdit(this);
    propertyValueLayout->addWidget(m_propertyValueEdit);
    
    m_updatePropertyValueButton = new QPushButton("Update Value", this);
    propertyValueLayout->addWidget(m_updatePropertyValueButton);
    
    propertyDetailsLayout->addWidget(m_propertyValueGroup);
    
    m_propertyMetadataGroup = new QGroupBox("Property Metadata", this);
    auto *metadataLayout = new QGridLayout(m_propertyMetadataGroup);
    
    metadataLayout->addWidget(new QLabel("Resource:"), 0, 0);
    m_resourceEdit = new QLineEdit(this);
    metadataLayout->addWidget(m_resourceEdit, 0, 1);
    
    m_canGetCheck = new QCheckBox("Can Get", this);
    metadataLayout->addWidget(m_canGetCheck, 1, 0);
    
    metadataLayout->addWidget(new QLabel("Can Set:"), 1, 1);
    m_canSetCombo = new QComboBox(this);
    m_canSetCombo->addItems({"none", "full", "partial"});
    metadataLayout->addWidget(m_canSetCombo, 1, 2);
    
    m_canSubscribeCheck = new QCheckBox("Can Subscribe", this);
    metadataLayout->addWidget(m_canSubscribeCheck, 2, 0);
    
    m_requireResIdCheck = new QCheckBox("Require ResId", this);
    metadataLayout->addWidget(m_requireResIdCheck, 2, 1);
    
    m_canPaginateCheck = new QCheckBox("Can Paginate", this);
    metadataLayout->addWidget(m_canPaginateCheck, 2, 2);
    
    metadataLayout->addWidget(new QLabel("Media Types:"), 3, 0);
    m_mediaTypesEdit = new QTextEdit(this);
    m_mediaTypesEdit->setMaximumHeight(60);
    metadataLayout->addWidget(m_mediaTypesEdit, 3, 1, 1, 2);
    
    metadataLayout->addWidget(new QLabel("Encodings:"), 4, 0);
    m_encodingsEdit = new QTextEdit(this);
    m_encodingsEdit->setMaximumHeight(60);
    metadataLayout->addWidget(m_encodingsEdit, 4, 1, 1, 2);
    
    metadataLayout->addWidget(new QLabel("Schema:"), 5, 0);
    m_schemaEdit = new QTextEdit(this);
    m_schemaEdit->setMaximumHeight(100);
    metadataLayout->addWidget(m_schemaEdit, 5, 1, 1, 2);
    
    m_updateMetadataButton = new QPushButton("Update Metadata", this);
    metadataLayout->addWidget(m_updateMetadataButton, 6, 0, 1, 3);
    
    propertyDetailsLayout->addWidget(m_propertyMetadataGroup);
    
    m_subscriptionsGroup = new QGroupBox("Subscribed Clients", this);
    auto *subscriptionsLayout = new QVBoxLayout(m_subscriptionsGroup);
    
    m_subscriptionsList = new QListWidget(this);
    subscriptionsLayout->addWidget(m_subscriptionsList);
    
    m_unsubscribeButton = new QPushButton("Unsubscribe Selected", this);
    subscriptionsLayout->addWidget(m_unsubscribeButton);
    
    propertyDetailsLayout->addWidget(m_subscriptionsGroup);
    
    m_propertySplitter->addWidget(m_propertyDetailsWidget);
    propertyLayout->addWidget(m_propertySplitter);
    
    mainLayout->addWidget(propertyGroup);
}

void ResponderWidget::setupConnections()
{
    connect(m_profileList, &QListWidget::currentRowChanged, this, &ResponderWidget::onProfileSelectionChanged);
    connect(m_addProfileButton, &QPushButton::clicked, this, &ResponderWidget::onAddProfile);
    connect(m_editProfileButton, &QPushButton::clicked, this, &ResponderWidget::onEditProfile);
    connect(m_deleteProfileButton, &QPushButton::clicked, this, &ResponderWidget::onDeleteProfile);
    connect(m_addTestProfilesButton, &QPushButton::clicked, this, &ResponderWidget::onAddTestProfiles);
    
    connect(m_propertyList, &QListWidget::currentRowChanged, this, &ResponderWidget::onPropertySelectionChanged);
    connect(m_addPropertyButton, &QPushButton::clicked, this, &ResponderWidget::onAddProperty);
    connect(m_deletePropertyButton, &QPushButton::clicked, this, &ResponderWidget::onDeleteProperty);
    connect(m_updatePropertyValueButton, &QPushButton::clicked, this, &ResponderWidget::onUpdatePropertyValue);
    connect(m_updateMetadataButton, &QPushButton::clicked, this, &ResponderWidget::onUpdatePropertyMetadata);
    

    
    connect(this, &ResponderWidget::localProfilesChanged, this, [this]() {
        updateProfileList();
    });
    
    connect(this, &ResponderWidget::localPropertiesChanged, this, [this]() {
        updatePropertyList();
    });
    
    connect(this, &ResponderWidget::subscriptionsUpdated, this, [this]() {
        updatePropertyDetails();
    });
    
    setupEventBridge();
}

void ResponderWidget::onProfileSelectionChanged()
{
    auto *item = m_profileList->currentItem();
    if (item) {
        m_selectedProfile = item->text();
        updateProfileDetails();
    } else {
        m_selectedProfile.clear();
    }
}

void ResponderWidget::onAddProfile()
{
    bool ok;
    QString profileId = QInputDialog::getText(this, "Add Profile", "Profile ID (format: XX:XX:XX:XX:XX):", QLineEdit::Normal, "00:00:00:00:00", &ok);
    if (ok && !profileId.isEmpty()) {
        m_profileList->addItem(profileId);
        m_repository->log(QString("Added profile: %1").arg(profileId).toStdString(), tooling::MessageDirection::Out);
    }
}

void ResponderWidget::onEditProfile()
{
    auto *item = m_profileList->currentItem();
    if (item) {
        bool ok;
        QString newProfileId = QInputDialog::getText(this, "Edit Profile", "Profile ID:", QLineEdit::Normal, item->text(), &ok);
        if (ok && !newProfileId.isEmpty()) {
            item->setText(newProfileId);
            m_repository->log(QString("Updated profile to: %1").arg(newProfileId).toStdString(), tooling::MessageDirection::Out);
        }
    }
}

void ResponderWidget::onDeleteProfile()
{
    auto *item = m_profileList->currentItem();
    if (item) {
        int ret = QMessageBox::question(this, "Delete Profile", QString("Delete profile '%1'?").arg(item->text()));
        if (ret == QMessageBox::Yes) {
            m_repository->log(QString("Deleted profile: %1").arg(item->text()).toStdString(), tooling::MessageDirection::Out);
            delete item;
        }
    }
}

void ResponderWidget::onAddTestProfiles()
{
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    deviceModel->add_test_profile_items();
    m_repository->log("Added test profile items", tooling::MessageDirection::Out);
}

void ResponderWidget::onPropertySelectionChanged()
{
    auto *item = m_propertyList->currentItem();
    if (item) {
        m_selectedProperty = item->text();
        updatePropertyDetails();
    } else {
        m_selectedProperty.clear();
    }
}

void ResponderWidget::onAddProperty()
{
    bool ok;
    QString propertyId = QInputDialog::getText(this, "Add Property", "Property ID:", QLineEdit::Normal, QString("X-%1").arg(rand() % 10000), &ok);
    if (ok && !propertyId.isEmpty()) {
        if (!m_repository || !m_repository->get_ci_device_manager()) {
            return;
        }
        
        auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
        if (!deviceModel) {
            return;
        }
        
        // Create a new property with default metadata
        auto property = midicci::commonproperties::CommonRulesPropertyMetadata(propertyId.toStdString());
        property.canGet = true;
        property.canSet = "full";
        property.canSubscribe = true;
        property.requireResId = false;
        property.canPaginate = false;
        property.mediaTypes = {"application/json"};
        property.encodings = {"ASCII"};
        property.schema = "{}";
        
        // Set initial empty data
        std::vector<uint8_t> initialData = {};
        property.setData(initialData);
        
        // Add to the business logic layer
        deviceModel->add_local_property(property);
        
        // Update the UI to reflect the new property
        updatePropertyList();
        
        m_repository->log(QString("Added property: %1").arg(propertyId).toStdString(), tooling::MessageDirection::Out);
    }
}

void ResponderWidget::onDeleteProperty()
{
    auto *item = m_propertyList->currentItem();
    if (item) {
        QString propertyId = item->text();
        
        // Don't allow deletion of predefined properties
        if (propertyId == "DeviceInfo" || propertyId == "ChannelList" || propertyId == "JSONSchema") {
            QMessageBox::information(this, "Cannot Delete", "Cannot delete predefined system properties.");
            return;
        }
        
        int ret = QMessageBox::question(this, "Delete Property", QString("Delete property '%1'?").arg(propertyId));
        if (ret == QMessageBox::Yes) {
            if (!m_repository || !m_repository->get_ci_device_manager()) {
                return;
            }
            
            auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
            if (!deviceModel) {
                return;
            }
            
            // Remove from business logic layer
            deviceModel->remove_local_property(propertyId.toStdString());
            
            // Update the UI
            updatePropertyList();
            
            // Clear selected property if it was the deleted one
            if (m_selectedProperty == propertyId) {
                m_selectedProperty.clear();
                updatePropertyDetails();
            }
            
            m_repository->log(QString("Deleted property: %1").arg(propertyId).toStdString(), tooling::MessageDirection::Out);
        }
    }
}

void ResponderWidget::onUpdatePropertyValue()
{
    if (!m_selectedProperty.isEmpty()) {
        // Don't allow updating predefined properties
        if (m_selectedProperty == "DeviceInfo" || m_selectedProperty == "ChannelList" || m_selectedProperty == "JSONSchema") {
            m_repository->log(QString("Updated property value for: %1 (simulated for predefined property)").arg(m_selectedProperty).toStdString(), tooling::MessageDirection::Out);
            return;
        }
        
        if (!m_repository || !m_repository->get_ci_device_manager()) {
            return;
        }
        
        auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
        if (!deviceModel) {
            return;
        }
        
        QString value = m_propertyValueEdit->toPlainText();
        
        // Convert QString to bytes (UTF-8)
        QByteArray byteArray = value.toUtf8();
        std::vector<uint8_t> data(byteArray.begin(), byteArray.end());
        
        // Update in business logic layer
        deviceModel->update_property_value(m_selectedProperty.toStdString(), "", data);
        
        // Refresh the UI to show the updated property value
        updatePropertyDetails();
        
        m_repository->log(QString("Updated property value for: %1").arg(m_selectedProperty).toStdString(), tooling::MessageDirection::Out);
    }
}

void ResponderWidget::onUpdatePropertyMetadata()
{
    if (!m_selectedProperty.isEmpty()) {
        // Don't allow updating predefined properties
        if (m_selectedProperty == "DeviceInfo" || m_selectedProperty == "ChannelList" || m_selectedProperty == "JSONSchema") {
            m_repository->log(QString("Updated property metadata for: %1 (simulated for predefined property)").arg(m_selectedProperty).toStdString(), tooling::MessageDirection::Out);
            return;
        }
        
        if (!m_repository || !m_repository->get_ci_device_manager()) {
            return;
        }
        
        auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
        if (!deviceModel) {
            return;
        }
        
        // Create new metadata with values from UI
        auto newMetadata = midicci::commonproperties::CommonRulesPropertyMetadata(m_selectedProperty.toStdString());
        
        // Set metadata from UI fields
        newMetadata.canGet = m_canGetCheck->isChecked();
        newMetadata.canSet = m_canSetCombo->currentText().toStdString();
        newMetadata.canSubscribe = m_canSubscribeCheck->isChecked();
        newMetadata.requireResId = m_requireResIdCheck->isChecked();
        newMetadata.canPaginate = m_canPaginateCheck->isChecked();
        
        // Parse media types (comma-separated)
        QStringList mediaTypesList = m_mediaTypesEdit->toPlainText().split(",", Qt::SkipEmptyParts);
        newMetadata.mediaTypes.clear();
        for (const QString& type : mediaTypesList) {
            newMetadata.mediaTypes.push_back(type.trimmed().toStdString());
        }
        if (newMetadata.mediaTypes.empty()) {
            newMetadata.mediaTypes.push_back("application/json");
        }
        
        // Parse encodings (comma-separated)
        QStringList encodingsList = m_encodingsEdit->toPlainText().split(",", Qt::SkipEmptyParts);
        newMetadata.encodings.clear();
        for (const QString& encoding : encodingsList) {
            newMetadata.encodings.push_back(encoding.trimmed().toStdString());
        }
        if (newMetadata.encodings.empty()) {
            newMetadata.encodings.push_back("ASCII");
        }
        
        newMetadata.schema = m_schemaEdit->toPlainText().toStdString();
        if (newMetadata.schema.empty()) {
            newMetadata.schema = "{}";
        }
        
        // Update the property metadata in business logic
        deviceModel->update_property_metadata(m_selectedProperty.toStdString(), newMetadata);
        
        m_repository->log(QString("Updated property metadata for: %1").arg(m_selectedProperty).toStdString(), tooling::MessageDirection::Out);
    }
}

void ResponderWidget::updateProfileList()
{
    m_profileList->clear();
    
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    auto& localProfiles = deviceModel->get_local_profile_states();
    for (const auto& profile : localProfiles) {
        if (profile) {
            auto profileId = profile->get_profile();
            QString profileText = QString("%1 (G%2 A%3)")
                .arg(QString::fromStdString(profileId.to_string()))
                .arg(profile->group().get())
                .arg(profile->address().get());
            m_profileList->addItem(profileText);
        }
    }
}

void ResponderWidget::updateProfileDetails()
{
    m_profileTargetsTable->setRowCount(0);
    if (!m_selectedProfile.isEmpty()) {
        m_profileTargetsTable->insertRow(0);
        m_profileTargetsTable->setItem(0, 0, new QTableWidgetItem("Enabled"));
        m_profileTargetsTable->setItem(0, 1, new QTableWidgetItem("0"));
        m_profileTargetsTable->setItem(0, 2, new QTableWidgetItem("Function Block"));
        m_profileTargetsTable->setItem(0, 3, new QTableWidgetItem("1"));
        m_profileTargetsTable->setItem(0, 4, new QTableWidgetItem("Delete"));
    }
}

void ResponderWidget::updatePropertyList()
{
    m_propertyList->clear();
    
    // Always show the predefined properties first
    m_propertyList->addItem("DeviceInfo");
    m_propertyList->addItem("ChannelList");
    m_propertyList->addItem("JSONSchema");
    
    // Add user-defined properties from PropertyHostFacade if available
    if (m_repository && m_repository->get_ci_device_manager()) {
        auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
        if (deviceModel) {
            auto propertyIds = deviceModel->get_local_property_ids();
            for (const auto& propertyId : propertyIds) {
                m_propertyList->addItem(QString::fromStdString(propertyId));
            }
        }
    }
}

void ResponderWidget::updatePropertyDetails()
{
    if (!m_selectedProperty.isEmpty()) {
        // Handle predefined properties with default values
        if (m_selectedProperty == "DeviceInfo" || m_selectedProperty == "ChannelList" || m_selectedProperty == "JSONSchema") {
            m_propertyValueEdit->setText(QString("{\n  \"property\": \"%1\",\n  \"value\": \"sample data\"\n}").arg(m_selectedProperty));
            m_resourceEdit->setText(m_selectedProperty);
            m_canGetCheck->setChecked(true);
            m_canSetCombo->setCurrentText("full");
            m_canSubscribeCheck->setChecked(true);
            m_requireResIdCheck->setChecked(false);
            m_canPaginateCheck->setChecked(false);
            m_mediaTypesEdit->setText("application/json");
            m_encodingsEdit->setText("ascii");
            m_schemaEdit->setText("{}");
        } else {
            // Handle user-defined properties from PropertyHostFacade
            if (!m_repository || !m_repository->get_ci_device_manager()) {
                return;
            }
            
            auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
            if (!deviceModel) {
                return;
            }
            
            // Get actual property metadata
            auto* metadata = deviceModel->get_local_property_metadata(m_selectedProperty.toStdString());
            if (metadata) {
            // Display actual property value
            auto data = metadata->getData();
            QString valueText = QString::fromUtf8(reinterpret_cast<const char*>(data.data()), data.size());
            m_propertyValueEdit->setText(valueText);
            
            // Set resource ID
            m_resourceEdit->setText(QString::fromStdString(metadata->getPropertyId()));
            
            // Try to cast to CommonRulesPropertyMetadata to access extended properties
            auto* commonRules = dynamic_cast<const midicci::commonproperties::CommonRulesPropertyMetadata*>(metadata);
            if (commonRules) {
                m_canGetCheck->setChecked(commonRules->canGet);
                m_canSetCombo->setCurrentText(QString::fromStdString(commonRules->canSet));
                m_canSubscribeCheck->setChecked(commonRules->canSubscribe);
                m_requireResIdCheck->setChecked(commonRules->requireResId);
                m_canPaginateCheck->setChecked(commonRules->canPaginate);
                
                // Join media types and encodings
                QStringList mediaTypes;
                for (const auto& type : commonRules->mediaTypes) {
                    mediaTypes << QString::fromStdString(type);
                }
                m_mediaTypesEdit->setText(mediaTypes.join(", "));
                
                QStringList encodings;
                for (const auto& encoding : commonRules->encodings) {
                    encodings << QString::fromStdString(encoding);
                }
                m_encodingsEdit->setText(encodings.join(", "));
                
                m_schemaEdit->setText(QString::fromStdString(commonRules->schema));
            } else {
                // Default values for non-CommonRules metadata
                m_canGetCheck->setChecked(true);
                m_canSetCombo->setCurrentText("full");
                m_canSubscribeCheck->setChecked(false);
                m_requireResIdCheck->setChecked(false);
                m_canPaginateCheck->setChecked(false);
                m_mediaTypesEdit->setText("application/json");
                m_encodingsEdit->setText("ASCII");
                m_schemaEdit->setText("{}");
            }
        } else {
            // Property not found, clear fields
            m_propertyValueEdit->clear();
            m_resourceEdit->clear();
            m_canGetCheck->setChecked(false);
            m_canSetCombo->setCurrentText("none");
            m_canSubscribeCheck->setChecked(false);
            m_requireResIdCheck->setChecked(false);
            m_canPaginateCheck->setChecked(false);
            m_mediaTypesEdit->clear();
            m_encodingsEdit->clear();
            m_schemaEdit->clear();
            }
        }
        
        m_subscriptionsList->clear();
        
        // Display real subscriptions from PropertyHostFacade
        if (m_repository && m_repository->get_ci_device_manager()) {
            auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
            if (deviceModel && deviceModel->get_device()) {
                auto& propertyFacade = deviceModel->get_device()->get_property_host_facade();
                auto subscriptions = propertyFacade.get_subscriptions();
                
                for (const auto& sub : subscriptions) {
                    QString itemText = QString("Client MUID: 0x%1 (Subscription: %2)")
                        .arg(sub.subscriber_muid, 8, 16, QLatin1Char('0'))
                        .arg(QString::fromStdString(sub.subscription_id));
                    m_subscriptionsList->addItem(itemText);
                }
                
                if (subscriptions.empty()) {
                    m_subscriptionsList->addItem("No active subscriptions");
                }
            }
        }
    }
}

void ResponderWidget::setupEventBridge()
{
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }

    auto profileStates = &deviceModel->get_local_profile_states();
    profileStates->set_collection_changed_handler([this](auto action, auto& item) {
        updateProfileList();
    });
    
    deviceModel->add_profiles_updated_callback([this]() {
        QMetaObject::invokeMethod(this, [this]() {
            updateProfileList();
            emit localProfilesChanged();
        }, Qt::QueuedConnection);
    });
    
    deviceModel->add_properties_updated_callback([this]() {
        QMetaObject::invokeMethod(this, [this]() {
            updatePropertyList();
            emit localPropertiesChanged();
        }, Qt::QueuedConnection);
    });
    
    QMetaObject::invokeMethod(this, [this]() {
        updateProfileList();
        updatePropertyList();
    }, Qt::QueuedConnection);
}
}
