#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QSplitter>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <memory>

namespace ci_tool {
    class CIToolRepository;
    class CIDeviceModel;
}

class ResponderWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ResponderWidget(ci_tool::CIToolRepository* repository, QWidget *parent = nullptr);

signals:
    void localProfilesChanged();
    void localPropertiesChanged();
    void subscriptionsUpdated();

private slots:
    void onProfileSelectionChanged();
    void onAddProfile();
    void onEditProfile();
    void onDeleteProfile();
    void onAddTestProfiles();
    void onPropertySelectionChanged();
    void onAddProperty();
    void onDeleteProperty();
    void onUpdatePropertyValue();
    void onUpdatePropertyMetadata();
    void checkForLocalUpdates();

private:
    void setupUI();
    void setupConnections();
    void updateProfileList();
    void updateProfileDetails();
    void updatePropertyList();
    void updatePropertyDetails();

    ci_tool::CIToolRepository* m_repository;
    
    QSplitter *m_profileSplitter;
    QListWidget *m_profileList;
    QWidget *m_profileDetailsWidget;
    QPushButton *m_addProfileButton;
    QPushButton *m_editProfileButton;
    QPushButton *m_deleteProfileButton;
    QPushButton *m_addTestProfilesButton;
    QTableWidget *m_profileTargetsTable;
    QPushButton *m_addProfileTargetButton;
    
    QSplitter *m_propertySplitter;
    QListWidget *m_propertyList;
    QWidget *m_propertyDetailsWidget;
    QPushButton *m_addPropertyButton;
    QPushButton *m_deletePropertyButton;
    
    QGroupBox *m_propertyValueGroup;
    QTextEdit *m_propertyValueEdit;
    QPushButton *m_updatePropertyValueButton;
    
    QGroupBox *m_propertyMetadataGroup;
    QLineEdit *m_resourceEdit;
    QCheckBox *m_canGetCheck;
    QComboBox *m_canSetCombo;
    QCheckBox *m_canSubscribeCheck;
    QCheckBox *m_requireResIdCheck;
    QTextEdit *m_mediaTypesEdit;
    QTextEdit *m_encodingsEdit;
    QTextEdit *m_schemaEdit;
    QCheckBox *m_canPaginateCheck;
    QPushButton *m_updateMetadataButton;
    
    QGroupBox *m_subscriptionsGroup;
    QListWidget *m_subscriptionsList;
    QPushButton *m_unsubscribeButton;
    
    QString m_selectedProfile;
    QString m_selectedProperty;
    
    QTimer* m_updateTimer;
    size_t m_lastProfileCount;
    size_t m_lastPropertyCount;
};
