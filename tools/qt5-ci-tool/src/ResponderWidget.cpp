#include "ResponderWidget.hpp"
#include "CIToolRepository.hpp"
#include "CIDeviceModel.hpp"
#include "AppModel.hpp"
#include "midi-ci/properties/PropertyManager.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>


ResponderWidget::ResponderWidget(ci_tool::CIToolRepository* repository, QWidget *parent)
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
        m_repository->log(QString("Added profile: %1").arg(profileId).toStdString(), ci_tool::MessageDirection::Out);
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
            m_repository->log(QString("Updated profile to: %1").arg(newProfileId).toStdString(), ci_tool::MessageDirection::Out);
        }
    }
}

void ResponderWidget::onDeleteProfile()
{
    auto *item = m_profileList->currentItem();
    if (item) {
        int ret = QMessageBox::question(this, "Delete Profile", QString("Delete profile '%1'?").arg(item->text()));
        if (ret == QMessageBox::Yes) {
            m_repository->log(QString("Deleted profile: %1").arg(item->text()).toStdString(), ci_tool::MessageDirection::Out);
            delete item;
        }
    }
}

void ResponderWidget::onAddTestProfiles()
{
    m_profileList->addItem("7E:00:00:00:01");
    m_profileList->addItem("7E:00:00:00:02");
    m_profileList->addItem("00:01:02:03:04");
    m_repository->log("Added test profile items", ci_tool::MessageDirection::Out);
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
        m_propertyList->addItem(propertyId);
        m_repository->log(QString("Added property: %1").arg(propertyId).toStdString(), ci_tool::MessageDirection::Out);
    }
}

void ResponderWidget::onDeleteProperty()
{
    auto *item = m_propertyList->currentItem();
    if (item) {
        int ret = QMessageBox::question(this, "Delete Property", QString("Delete property '%1'?").arg(item->text()));
        if (ret == QMessageBox::Yes) {
            m_repository->log(QString("Deleted property: %1").arg(item->text()).toStdString(), ci_tool::MessageDirection::Out);
            delete item;
        }
    }
}

void ResponderWidget::onUpdatePropertyValue()
{
    if (!m_selectedProperty.isEmpty()) {
        QString value = m_propertyValueEdit->toPlainText();
        m_repository->log(QString("Updated property value for: %1").arg(m_selectedProperty).toStdString(), ci_tool::MessageDirection::Out);
    }
}

void ResponderWidget::onUpdatePropertyMetadata()
{
    if (!m_selectedProperty.isEmpty()) {
        m_repository->log(QString("Updated property metadata for: %1").arg(m_selectedProperty).toStdString(), ci_tool::MessageDirection::Out);
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
    
    auto localProfiles = deviceModel->get_local_profile_states();
    for (const auto& profile : localProfiles) {
        if (profile) {
            auto profileId = profile->get_profile();
            QString profileText = QString("%1 (G%2 A%3)")
                .arg(QString::fromStdString(profileId.to_string()))
                .arg(profile->get_group())
                .arg(profile->get_address());
            m_profileList->addItem(profileText);
        }
    }
    
    if (localProfiles.empty()) {
        m_profileList->addItem("7E:00:00:00:01");
        m_profileList->addItem("00:01:02:03:04");
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
    
    if (!m_repository || !m_repository->get_ci_device_manager()) {
        m_propertyList->addItem("DeviceInfo");
        m_propertyList->addItem("ChannelList");
        m_propertyList->addItem("JSONSchema");
        return;
    }
    
    auto deviceModel = m_repository->get_ci_device_manager()->get_device_model();
    if (!deviceModel) {
        m_propertyList->addItem("DeviceInfo");
        m_propertyList->addItem("ChannelList");
        m_propertyList->addItem("JSONSchema");
        return;
    }
    
    m_propertyList->addItem("DeviceInfo");
    m_propertyList->addItem("ChannelList");
    m_propertyList->addItem("JSONSchema");
}

void ResponderWidget::updatePropertyDetails()
{
    if (!m_selectedProperty.isEmpty()) {
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
        
        m_subscriptionsList->clear();
        m_subscriptionsList->addItem("Client 1 (MUID: 0x12345678)");
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
