#include "ResponderWidget.hpp"
#include <midicci/tooling/CIToolRepository.hpp>
#include <midicci/tooling/CIDeviceModel.hpp>
#include "AppModel.hpp"
#include <midicci/midicci.hpp>
#include <algorithm>

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
    connect(m_addProfileTargetButton, &QPushButton::clicked, this, &ResponderWidget::onAddProfileTarget);
    
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
        if (!m_repository || !m_repository->get_ci_device_manager()) {
            return;
        }
        
        auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
        if (!deviceModel) {
            return;
        }
        
        // Parse the profile ID string (XX:XX:XX:XX:XX)
        QStringList parts = profileId.split(":");
        if (parts.size() != 5) {
            QMessageBox::warning(this, "Invalid Profile ID", "Profile ID must be in format XX:XX:XX:XX:XX (5 hex bytes)");
            return;
        }
        
        std::vector<uint8_t> profileBytes;
        for (const QString& part : parts) {
            bool valid;
            uint8_t byte = part.toUInt(&valid, 16);
            if (!valid || part.length() != 2) {
                QMessageBox::warning(this, "Invalid Profile ID", "Each byte must be 2 hex digits (00-FF)");
                return;
            }
            profileBytes.push_back(byte);
        }
        
        // Create MidiCIProfileId and MidiCIProfile
        auto midiProfileId = MidiCIProfileId(profileBytes);
        auto midiProfile = MidiCIProfile(midiProfileId, 0, 127, false, 1); // group=0, address=127 (function block), disabled, 1 channel
        
        // Add to device model (this will trigger UI update via callbacks)
        deviceModel->add_local_profile(midiProfile);
        
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

void ResponderWidget::onAddProfileTarget()
{
    if (m_selectedProfile.isEmpty()) {
        QMessageBox::information(this, "No Profile Selected", "Please select a profile first before adding a target.");
        return;
    }
    
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    // Parse the selected profile to get the profile ID
    QString profileIdStr = m_selectedProfile;
    
    QStringList parts = profileIdStr.split(":");
    if (parts.size() != 5) {
        QMessageBox::warning(this, "Invalid Profile", "Cannot parse selected profile ID");
        return;
    }
    
    std::vector<uint8_t> profileBytes;
    for (const QString& part : parts) {
        bool valid;
        uint8_t byte = part.toUInt(&valid, 16);
        if (!valid) {
            QMessageBox::warning(this, "Invalid Profile", "Cannot parse selected profile ID");
            return;
        }
        profileBytes.push_back(byte);
    }
    
    auto profileId = MidiCIProfileId(profileBytes);
    
    // Show dialog to get target parameters
    bool ok;
    QString addressStr = QInputDialog::getText(this, "Add Profile Target", "Target Address (0-127):", QLineEdit::Normal, "127", &ok);
    if (!ok || addressStr.isEmpty()) {
        return;
    }
    
    uint8_t address = addressStr.toUInt(&ok);
    if (!ok || address > 127) {
        QMessageBox::warning(this, "Invalid Address", "Address must be between 0 and 127");
        return;
    }
    
    QString channelsStr = QInputDialog::getText(this, "Add Profile Target", "Number of Channels (1-16):", QLineEdit::Normal, "1", &ok);
    if (!ok || channelsStr.isEmpty()) {
        return;
    }
    
    uint16_t numChannels = channelsStr.toUInt(&ok);
    if (!ok || numChannels < 1 || numChannels > 16) {
        QMessageBox::warning(this, "Invalid Channels", "Number of channels must be between 1 and 16");
        return;
    }
    
    // Preserve the current selection BEFORE adding (which will trigger callbacks)
    QString selectedProfile = m_selectedProfile;
    
    // Create and add the profile
    auto profile = MidiCIProfile(profileId, 0, address, false, numChannels); // group=0, disabled initially
    deviceModel->add_local_profile(profile);
    auto& profiles = deviceModel->get_local_profile_states();
    
    m_repository->log(QString("Added profile target: address=%1, channels=%2").arg(address).arg(numChannels).toStdString(), tooling::MessageDirection::Out);
    
    // Force immediate refresh of profile list (this may have already happened via callbacks)
    updateProfileList();
    
    // Restore the selection
    if (!selectedProfile.isEmpty()) {
        for (int i = 0; i < m_profileList->count(); ++i) {
            if (m_profileList->item(i)->text() == selectedProfile) {
                m_profileList->setCurrentRow(i);
                m_selectedProfile = selectedProfile; // Ensure the member variable is set
                break;
            }
        }
    }
    
    // Ensure m_selectedProfile is set to the preserved value if we had one
    if (!selectedProfile.isEmpty()) {
        m_selectedProfile = selectedProfile;
    }
    
    // Now refresh the details with the restored selection
    if (!selectedProfile.isEmpty()) {
        updateProfileDetails();
    }
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
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    // Create a new property with default metadata
    auto property = deviceModel->create_new_property();
    if (!property) {
        return;
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

        // Preserve the current selection BEFORE updating (which will trigger callbacks)
        QString selectedProperty = m_selectedProperty;

        // Get the new resource name from UI (user might have changed it)
        QString newPropertyId = m_resourceEdit->text();
        if (newPropertyId.isEmpty()) {
            newPropertyId = selectedProperty;
        }

        // Create new metadata with values from UI
        auto newMetadata = midicci::commonproperties::CommonRulesPropertyMetadata(newPropertyId.toStdString());

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
        deviceModel->update_property_metadata(selectedProperty.toStdString(), newMetadata);

        m_repository->log(QString("Updated property metadata for: %1").arg(selectedProperty).toStdString(), tooling::MessageDirection::Out);

        // Force immediate refresh of property list (this may have already happened via callbacks)
        updatePropertyList();

        // Restore the selection to the property (using the new property ID if it was renamed)
        if (!newPropertyId.isEmpty()) {
            for (int i = 0; i < m_propertyList->count(); ++i) {
                if (m_propertyList->item(i)->text() == newPropertyId) {
                    m_propertyList->setCurrentRow(i);
                    m_selectedProperty = newPropertyId;
                    break;
                }
            }
        }

        // Ensure m_selectedProperty is set to the new property ID
        if (!newPropertyId.isEmpty()) {
            m_selectedProperty = newPropertyId;
        }

        // Now refresh the details with the restored selection
        if (!m_selectedProperty.isEmpty()) {
            updatePropertyDetails();
        }
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
    
    // Get unique profile IDs while preserving order
    std::vector<std::string> uniqueProfiles;
    auto& localProfiles = deviceModel->get_local_profile_states();
    for (const auto& profile : localProfiles) {
        if (profile) {
            std::string profileId = profile->get_profile().to_string();
            if (std::find(uniqueProfiles.begin(), uniqueProfiles.end(), profileId) == uniqueProfiles.end()) {
                uniqueProfiles.push_back(profileId);
            }
        }
    }
    
    // Add each unique profile ID to the list
    for (const auto& profileId : uniqueProfiles) {
        m_profileList->addItem(QString::fromStdString(profileId));
    }
}

void ResponderWidget::updateProfileDetails()
{
    m_profileTargetsTable->setRowCount(0);
    
    if (m_selectedProfile.isEmpty()) {
        return;
    }
    
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        return;
    }
    
    // Parse the selected profile to get the profile ID
    QString profileIdStr = m_selectedProfile;
    
    QStringList parts = profileIdStr.split(":");
    if (parts.size() != 5) {
        return;
    }
    
    std::vector<uint8_t> profileBytes;
    for (const QString& part : parts) {
        bool valid;
        uint8_t byte = part.toUInt(&valid, 16);
        if (!valid) {
            return;
        }
        profileBytes.push_back(byte);
    }
    
    auto profileId = MidiCIProfileId(profileBytes);
    
    // Find all profile states that match this profile ID
    auto& localProfiles = deviceModel->get_local_profile_states();
    int row = 0;
    
    for (const auto& profileState : localProfiles) {
        if (profileState && profileState->get_profile().to_string() == profileId.to_string()) {
            m_profileTargetsTable->insertRow(row);
            
            // Column 0: Enabled checkbox
            auto* enabledCheckbox = new QCheckBox();
            enabledCheckbox->setChecked(profileState->enabled().get());
            
            // Connect checkbox to update the profile state
            connect(enabledCheckbox, &QCheckBox::toggled, [this, profileState, deviceModel](bool checked) {
                uint8_t group = profileState->group().get();
                uint8_t address = profileState->address().get();
                uint16_t numChannels = profileState->num_channels_requested().get();
                
                deviceModel->update_local_profile_target(profileState, address, checked, numChannels);
                m_repository->log(QString("Profile target %1: enabled=%2")
                    .arg(QString::fromStdString(profileState->get_profile().to_string()))
                    .arg(checked ? "true" : "false").toStdString(), tooling::MessageDirection::Out);
            });
            
            m_profileTargetsTable->setCellWidget(row, 0, enabledCheckbox);
            
            // Column 1: Group
            m_profileTargetsTable->setItem(row, 1, new QTableWidgetItem(QString::number(profileState->group().get())));
            
            // Column 2: Address (with meaningful display)
            uint8_t address = profileState->address().get();
            QString addressText;
            if (address == 0x7F) {
                addressText = "Function Block";
            } else if (address == 0x7E) {
                addressText = "Group";
            } else if (address <= 15) {
                addressText = QString("Ch. %1").arg(address + 1); // Display as 1-16 instead of 0-15
            } else {
                addressText = QString::number(address);
            }
            m_profileTargetsTable->setItem(row, 2, new QTableWidgetItem(addressText));
            
            // Column 3: Channels
            m_profileTargetsTable->setItem(row, 3, new QTableWidgetItem(QString::number(profileState->num_channels_requested().get())));
            
            // Column 4: Delete button
            auto* deleteButton = new QPushButton("Delete");
            connect(deleteButton, &QPushButton::clicked, [this, profileState, deviceModel]() {
                int ret = QMessageBox::question(this, "Delete Profile Target", 
                    QString("Delete profile target at address %1?")
                    .arg(profileState->address().get()));
                if (ret == QMessageBox::Yes) {
                    uint8_t group = profileState->group().get();
                    uint8_t address = profileState->address().get();
                    auto profileId = profileState->get_profile();
                    
                    deviceModel->remove_local_profile(group, address, profileId);
                    m_repository->log(QString("Deleted profile target: %1 at address %2")
                        .arg(QString::fromStdString(profileId.to_string()))
                        .arg(address).toStdString(), tooling::MessageDirection::Out);
                    
                    // Refresh the table
                    updateProfileDetails();
                }
            });
            
            m_profileTargetsTable->setCellWidget(row, 4, deleteButton);
            
            row++;
        }
    }
    
    // If no targets found, show a placeholder row
    if (row == 0) {
        m_profileTargetsTable->insertRow(0);
        m_profileTargetsTable->setItem(0, 0, new QTableWidgetItem("No targets configured"));
        m_profileTargetsTable->setSpan(0, 0, 1, 5); // Span across all columns
    }
}

void ResponderWidget::updatePropertyList()
{
    m_propertyList->clear();
    
    // Always show the predefined properties first
    m_propertyList->addItem("DeviceInfo");
    m_propertyList->addItem("ChannelList");
    m_propertyList->addItem("JSONSchema");
    
    // Add user-defined properties directly from PropertyHostFacade
    if (m_repository && m_repository->get_ci_device_manager()) {
        auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
        if (deviceModel && deviceModel->get_device()) {
            auto& propertyFacade = deviceModel->get_device()->get_property_host_facade();
            auto metadataList = propertyFacade.get_properties().getMetadataList();
            for (const auto& metadata : metadataList) {
                if (metadata) {
                    m_propertyList->addItem(QString::fromStdString(metadata->getPropertyId()));
                }
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
        // Also refresh profile details if we have a profile selected
        if (!m_selectedProfile.isEmpty()) {
            updateProfileDetails();
        }
    });
    
    deviceModel->add_profiles_updated_callback([this]() {
        QMetaObject::invokeMethod(this, [this]() {
            updateProfileList();
            // Also refresh profile details if we have a profile selected
            if (!m_selectedProfile.isEmpty()) {
                updateProfileDetails();
            }
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
