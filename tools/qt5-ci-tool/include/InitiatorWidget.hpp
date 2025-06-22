#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QSplitter>
#include <QTableWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>

#include <memory>
#include <midicci/tooling/MutableState.hpp>
#include <midicci/tooling/CIToolRepository.hpp>

using namespace midicci::tooling;
namespace midicci::tooling::qt5 {

class InitiatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InitiatorWidget(midicci::tooling::CIToolRepository* repository, QWidget *parent = nullptr);

signals:
    void deviceConnected(int muid);
    void deviceDisconnected(int muid);
    void deviceInfoUpdated(int muid);
    void profilesUpdated(int muid);
    void propertiesUpdated(int muid);

private slots:
    void onSendDiscovery();
    void onDeviceSelectionChanged(int index);
    void onProfileSelectionChanged();
    void onPropertySelectionChanged();
    void onRefreshProperty();
    void onSubscribeProperty();
    void onRequestMidiMessageReport();
    void onPropertyEditModeChanged(bool editing);
    void onPropertyCommitChanges();
    void onPropertyValueTextChanged();


private:
    void setupUI();
    void setupConnections();
    void setupEventBridge();
    void setupPropertyCallbacks();
    void updateDeviceList();
    void updateConnectionInfo();
    void updateProfileList();
    void updatePropertyList();
    void updateCurrentPropertyValue();
    void sendGetPropertyDataRequest();

    midicci::tooling::CIToolRepository* m_repository;
    
    QPushButton *m_sendDiscoveryButton;
    QComboBox *m_deviceSelector;
    
    QGroupBox *m_deviceInfoGroup;
    QLabel *m_muidLabel;
    QLabel *m_manufacturerLabel;
    QLabel *m_familyLabel;
    QLabel *m_modelLabel;
    QLabel *m_versionLabel;
    QLabel *m_serialLabel;
    QLabel *m_maxConnectionsLabel;
    
    QSplitter *m_profileSplitter;
    QListWidget *m_profileList;
    QWidget *m_profileDetailsWidget;
    QComboBox *m_profileAddressSelector;
    QLineEdit *m_profileTargetEdit;
    QPushButton *m_sendProfileDetailsButton;
    QTableWidget *m_profileConfigTable;
    
    QSplitter *m_propertySplitter;
    QListWidget *m_propertyList;
    QWidget *m_propertyDetailsWidget;
    QTextEdit *m_propertyValueEdit;
    QPushButton *m_refreshPropertyButton;
    QPushButton *m_subscribePropertyButton;
    QComboBox *m_propertyEncodingSelector;
    
    QGroupBox *m_processInquiryGroup;
    QComboBox *m_midiReportAddressSelector;
    QPushButton *m_requestMidiReportButton;
    
    int m_selectedDeviceMUID;
    QString m_selectedProfile;
    QString m_selectedProperty;
    
    size_t m_lastConnectionCount;
    bool m_propertyCallbacksSetup;
    QString m_lastRequestedProperty;
    
    MutableState<bool> m_propertyEditingMode;
    MutableState<QString> m_propertyValueText;
    MutableState<QString> m_propertyResId;
    MutableState<QString> m_propertySelectedEncoding;
    MutableState<QString> m_propertyPartialContent;
    MutableState<int> m_propertyPaginateOffset;
    MutableState<int> m_propertyPaginateLimit;
    
    QCheckBox *m_propertyEditCheckbox;
    QLineEdit *m_propertyResIdEdit;
    QTextEdit *m_propertyPartialEdit;
    QLineEdit *m_propertyPaginateOffsetEdit;
    QLineEdit *m_propertyPaginateLimitEdit;
    QPushButton *m_propertyCommitButton;
    QLabel *m_propertyMetadataLabel;
    QGroupBox *m_propertyPaginationGroup;
};

} // namespace
