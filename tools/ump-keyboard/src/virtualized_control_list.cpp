#include "virtualized_control_list.h"
#include <midicci/midicci.hpp>
#include <midicci/details/commonproperties/StandardProperties.hpp>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QScrollBar>
#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QThread>
#include <QEvent>

// ControlParameterWidget implementation
ControlParameterWidget::ControlParameterWidget(QWidget* parent)
    : QWidget(parent), m_controlIndex(-1), m_currentControl(nullptr), 
      m_midiMin(0), m_midiMax(4294967295U), m_needsScaling(false) {
    
    // Set fixed height for the widget
    setFixedHeight(35);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(8, 4, 8, 4);
    m_layout->setSpacing(8);
    
    // Name label takes remaining space
    m_titleLabel = new QLabel("Test Label");
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_titleLabel->setStyleSheet("QLabel { background-color: lightblue; color: black; padding: 2px; }");
    
    m_noteSpinBox = new QSpinBox();
    m_noteSpinBox->setRange(0, 127);
    m_noteSpinBox->setValue(60);  // Middle C
    m_noteSpinBox->setFixedWidth(60);
    m_noteSpinBox->setVisible(false);  // Hidden by default
    
    // SIMPLIFIED: Direct slider without complex container
    m_slider = new QSlider(Qt::Horizontal);
    m_slider->blockSignals(true);  // Block during initialization
    m_slider->setMinimum(0);
    m_slider->setMaximum(2147483647);  // QSlider uses int, so max is 2^31-1  
    m_slider->setValue(0);             // Start at minimum, will be updated when control is loaded
    m_slider->setFixedWidth(100);  // Fixed 100px width
    m_slider->setFixedHeight(20);   // Fixed height
    
    // Proper slider styling with value visualization
    m_slider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "    border: 1px solid #999999;"
        "    height: 8px;"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4);"
        "    margin: 2px 0;"
        "    border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f);"
        "    border: 1px solid #5c5c5c;"
        "    width: 14px;"
        "    margin: -2px 0;"
        "    border-radius: 3px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #66BB6A, stop:1 #4CAF50);"  // Green fill for value
        "    border: 1px solid #777;"
        "    height: 8px;"
        "    border-radius: 3px;"
        "}"
    );
    m_slider->setFocusPolicy(Qt::StrongFocus);  // Allow keyboard/mouse focus
    m_slider->blockSignals(false);  // Re-enable for user interaction
    
    // Simple value label next to slider (not overlaid)
    m_valueLabel = new QLabel("64");
    m_valueLabel->setFixedSize(40, 20);
    m_valueLabel->setAlignment(Qt::AlignCenter);
    m_valueLabel->setStyleSheet("QLabel { background-color: yellow; color: black; font-weight: bold; border: 1px solid black; }");
    m_valueLabel->setFocusPolicy(Qt::NoFocus);
    
    m_layout->addWidget(m_titleLabel);
    m_layout->addWidget(m_noteSpinBox);
    m_layout->addWidget(m_slider);
    m_layout->addWidget(m_valueLabel);
    
    connect(m_slider, &QSlider::valueChanged, this, &ControlParameterWidget::onSliderValueChanged);
    
    // DEBUG: Add event filter to slider to see if it receives mouse events
    m_slider->installEventFilter(this);
    
    // Ensure this widget doesn't interfere with mouse events
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setFocusPolicy(Qt::NoFocus);  // Don't steal focus from child widgets
    
    // Enhanced debug styling
    setStyleSheet("ControlParameterWidget { background-color: lightgray; border: 2px solid red; }");
}

void ControlParameterWidget::updateFromControl(const midicci::commonproperties::MidiCIControl& control, int controlIndex, uint32_t currentValue) {
    m_controlIndex = controlIndex;
    m_currentControl = const_cast<midicci::commonproperties::MidiCIControl*>(&control);
    
    QString ctrlType = QString::fromStdString(control.ctrlType);
    QString title = QString::fromStdString(control.title);
    
    // Format title based on controller type
    QString displayText;
    if (ctrlType == "cc") {
        uint8_t ccNum = control.ctrlIndex.empty() ? 0 : control.ctrlIndex[0];
        displayText = QString("CC%1: %2").arg(ccNum).arg(title);
    } else if (ctrlType == "rpn" || ctrlType == "nrpn") {
        uint8_t msb = control.ctrlIndex.size() > 0 ? control.ctrlIndex[0] : 0;
        uint8_t lsb = control.ctrlIndex.size() > 1 ? control.ctrlIndex[1] : 0;
        uint16_t num = (msb << 7) | lsb;
        displayText = QString("%1 %2: %3").arg(ctrlType.toUpper()).arg(num).arg(title);
    } else if (ctrlType == "pnrc" || ctrlType == "pnac") {
        uint8_t ctrlNum = control.ctrlIndex.empty() ? 0 : control.ctrlIndex[0];
        displayText = QString("Key %1: %2").arg(ctrlNum).arg(title);
        m_noteSpinBox->setVisible(true);
    } else {
        displayText = QString("[%1] %2").arg(ctrlType.toUpper(), title);
    }
    
    // Show/hide note spinbox for per-note controls
    m_noteSpinBox->setVisible(ctrlType == "pnrc" || ctrlType == "pnac");
    
    m_titleLabel->setText(displayText);
    
    // Set slider range and update value label with JSON data
    // Block signals to prevent MIDI messages during initialization
    m_slider->blockSignals(true);
    
    m_midiMin = control.minMax.size() > 0 ? control.minMax[0] : 0;
    m_midiMax = control.minMax.size() > 1 ? control.minMax[1] : 4294967295U;  // 2^32-1
    uint32_t defaultVal = currentValue;  // Use stored current value instead of control's defaultValue

    // Handle conversion between 32-bit unsigned MIDI values and QSlider's signed int
    // QSlider max is 2^31-1 = 2147483647, but MIDI 2.0 max is 2^32-1 = 4294967295
    int sliderMin, sliderMax, sliderDefault;
    
    if (m_midiMax > 2147483647U) {
        // If max value exceeds int range, scale down to fit
        m_needsScaling = true;
        double scale = 2147483647.0 / static_cast<double>(m_midiMax);
        sliderMin = static_cast<int>(m_midiMin * scale);
        sliderMax = 2147483647;
        sliderDefault = static_cast<int>(defaultVal * scale);
    } else {
        // Values fit within int range
        m_needsScaling = false;
        sliderMin = static_cast<int>(m_midiMin);
        sliderMax = static_cast<int>(m_midiMax);
        sliderDefault = static_cast<int>(defaultVal);
    }
    
    m_slider->setRange(sliderMin, sliderMax);
    m_slider->setValue(sliderDefault);
    m_valueLabel->setText(QString::number(defaultVal));
    
    // Force UI updates to ensure initial state renders correctly
    m_slider->update();      // Update slider visual (green fill)
    m_valueLabel->update();  // Update value label display
    
    // Re-enable signals so user interactions work
    m_slider->blockSignals(false);
}

void ControlParameterWidget::setValueChangeCallback(std::function<void(int, const midicci::commonproperties::MidiCIControl&, uint32_t)> callback) {
    m_valueChangeCallback = callback;
}

void ControlParameterWidget::setValueUpdateCallback(std::function<void(int, uint32_t)> callback) {
    m_valueUpdateCallback = callback;
}

void ControlParameterWidget::updateValue(uint32_t value) {
    if (!m_slider || !m_valueLabel) {
        return;
    }
    
    // Block signals to prevent MIDI callback during programmatic update
    m_slider->blockSignals(true);
    
    // Update slider and label
    m_slider->setValue(static_cast<int>(value));
    m_valueLabel->setText(QString::number(value));
    
    // Force UI refresh
    m_slider->update();      // Update slider visual (green fill)
    m_valueLabel->update();  // Update value label display
    
    // Re-enable signals
    m_slider->blockSignals(false);
    
}

void ControlParameterWidget::mousePressEvent(QMouseEvent* event) {
    // SAFER: Just let the event bubble up naturally instead of manually forwarding
    // The slider should receive the event through normal Qt event propagation
    QWidget::mousePressEvent(event);
}

void ControlParameterWidget::mouseMoveEvent(QMouseEvent* event) {
    // SAFER: Let Qt handle event propagation naturally
    QWidget::mouseMoveEvent(event);
}

bool ControlParameterWidget::eventFilter(QObject* obj, QEvent* event) {
    // Don't filter the event, let it continue normally
    return QWidget::eventFilter(obj, event);
}

void ControlParameterWidget::onSliderValueChanged(int value) {
    // Convert slider value back to MIDI range
    uint32_t midiValue;
    if (m_needsScaling) {
        // Scale up from slider range to full MIDI range
        double scale = static_cast<double>(m_midiMax) / 2147483647.0;
        midiValue = static_cast<uint32_t>(value * scale);
        // Clamp to actual MIDI range
        if (midiValue < m_midiMin) midiValue = m_midiMin;
        if (midiValue > m_midiMax) midiValue = m_midiMax;
    } else {
        midiValue = static_cast<uint32_t>(value);
    }
    
    qDebug() << "Slider changed - Control:" << m_controlIndex << "SliderValue:" << value << "MIDIValue:" << midiValue;
    
    // THREAD-SAFE: Update UI on main thread only
    if (QThread::currentThread() != this->thread()) {
        qDebug() << "ERROR: onSliderValueChanged called from wrong thread!";
        return;
    }
    
    // NULL SAFETY: Check if widgets still exist
    if (!m_valueLabel || !m_currentControl) {
        qDebug() << "ERROR: widgets or control data is null!";
        return;
    }
    
    // Update UI with the actual MIDI value (not the scaled slider value)
    m_valueLabel->setText(QString::number(midiValue));
    m_valueLabel->update();  // Force immediate repaint of value label
    
    // Ensure slider visual state is updated (green fill should reflect new value)
    m_slider->update();  // Force immediate repaint of slider
    
    // Update the stored value in the parent list
    if (m_valueUpdateCallback) {
        m_valueUpdateCallback(m_controlIndex, midiValue);
    }
    
    // Send MIDI message based on control type
    if (m_valueChangeCallback && m_controlIndex >= 0) {
        try {
            QString ctrlType = QString::fromStdString(m_currentControl->ctrlType);
            int channel = m_currentControl->channel.value_or(0);
            
            qDebug() << "Sending MIDI -" << ctrlType << "on channel" << channel << "value" << midiValue;
            
            if (ctrlType == "cc") {
                // Control Change: send CC message
                uint8_t ccNum = m_currentControl->ctrlIndex.empty() ? 0 : m_currentControl->ctrlIndex[0];
                qDebug() << "Sending CC" << ccNum << "=" << midiValue;
                
            } else if (ctrlType == "rpn") {
                // Registered Parameter Number
                if (m_currentControl->ctrlIndex.size() >= 2) {
                    uint8_t msb = m_currentControl->ctrlIndex[0];
                    uint8_t lsb = m_currentControl->ctrlIndex[1];
                    qDebug() << "Sending RPN" << msb << ":" << lsb << "=" << midiValue;
                }
                
            } else if (ctrlType == "nrpn") {
                // Non-Registered Parameter Number  
                if (m_currentControl->ctrlIndex.size() >= 2) {
                    uint8_t msb = m_currentControl->ctrlIndex[0];
                    uint8_t lsb = m_currentControl->ctrlIndex[1];
                    qDebug() << "Sending NRPN" << msb << ":" << lsb << "=" << midiValue;
                }
                
            } else if (ctrlType == "pnrc") {
                // Per-Note Control Change (MIDI 2.0)
                uint8_t ctrlNum = m_currentControl->ctrlIndex.empty() ? 0 : m_currentControl->ctrlIndex[0];
                int note = m_noteSpinBox->isVisible() ? m_noteSpinBox->value() : 60;  // Use spinbox value or middle C
                qDebug() << "Sending Per-Note CC - Note" << note << "Controller" << ctrlNum << "=" << midiValue;
                
            } else if (ctrlType == "pnac") {
                // Per-Note Aftertouch (MIDI 2.0)
                int note = m_noteSpinBox->isVisible() ? m_noteSpinBox->value() : 60;  // Use spinbox value or middle C
                qDebug() << "Sending Per-Note Aftertouch - Note" << note << "=" << midiValue;
                
            } else {
                qDebug() << "Unknown control type:" << ctrlType;
            }
            
            // Call the callback to actually send the MIDI message with the converted value
            m_valueChangeCallback(m_controlIndex, *m_currentControl, midiValue);
            
        } catch (const std::exception& e) {
            qDebug() << "Exception in MIDI callback:" << e.what();
        } catch (...) {
            qDebug() << "Unknown exception in MIDI callback";
        }
    }
}

// VirtualizedControlList implementation
VirtualizedControlList::VirtualizedControlList(QWidget* parent)
    : QListWidget(parent) {
    
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // CRITICAL: Configure selection behavior to not interfere with widget interactions
    setSelectionMode(QAbstractItemView::NoSelection);  // Disable item selection entirely
    setFocusPolicy(Qt::NoFocus);  // Don't steal focus from child widgets
    
    // Alternative: if you want selection, use SingleSelection and ensure it doesn't block events
    // setSelectionMode(QAbstractItemView::SingleSelection);
    // setSelectionBehavior(QAbstractItemView::SelectRows);
    
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &VirtualizedControlList::updateVisibleItems);
}

void VirtualizedControlList::setControls(const std::vector<midicci::commonproperties::MidiCIControl>& controls) {
    m_controls = controls;
    
    // Initialize control values with default values
    m_controlValues.resize(controls.size());
    for (size_t i = 0; i < controls.size(); ++i) {
        m_controlValues[i] = controls[i].defaultValue;
    }
    
    // Clear existing items
    clear();
    
    if (controls.empty()) {
        // Add placeholder for empty state
        addItem("No controls available");
        setEnabled(false);
        return;
    }
    
    setEnabled(true);
    
    // TEMP: Create widgets for ALL items (no virtualization) to test basic functionality
    for (size_t i = 0; i < controls.size(); ++i) {
        QListWidgetItem* item = new QListWidgetItem();
        addItem(item);
        
        ControlParameterWidget* widget = new ControlParameterWidget();
        widget->setValueChangeCallback(m_valueChangeCallback);
        widget->setValueUpdateCallback([this](int index, uint32_t value) {
            updateStoredValue(index, value);
        });
        widget->updateFromControl(controls[i], static_cast<int>(i), m_controlValues[i]);
        
        setItemWidget(item, widget);
        
        // Force the item size to match our widget
        item->setSizeHint(QSize(widget->width(), 35));
    }
}

void VirtualizedControlList::setValueChangeCallback(std::function<void(int, const midicci::commonproperties::MidiCIControl&, uint32_t)> callback) {
    m_valueChangeCallback = callback;
}

void VirtualizedControlList::resizeEvent(QResizeEvent* event) {
    QListWidget::resizeEvent(event);
    // Update visible items when widget is resized
    QApplication::processEvents();
    updateVisibleItems();
}

void VirtualizedControlList::scrollContentsBy(int dx, int dy) {
    QListWidget::scrollContentsBy(dx, dy);
    // Update visible items when scrolling
    updateVisibleItems();
}

void VirtualizedControlList::updateVisibleItems() {
    if (m_controls.empty()) {
        return;
    }
    
    int firstVisible = getFirstVisibleIndex();
    int visibleCount = getVisibleItemCount();
    
    // Add buffer items above and below
    int startIndex = qMax(0, firstVisible - BUFFER_ITEMS);
    int endIndex = qMin(static_cast<int>(m_controls.size()) - 1, firstVisible + visibleCount + BUFFER_ITEMS);
    
    // Clear widgets from items that are no longer visible
    for (int i = 0; i < count(); ++i) {
        if (i < startIndex || i > endIndex) {
            QListWidgetItem* item = this->item(i);
            if (item && itemWidget(item)) {
                setItemWidget(item, nullptr);
            }
        }
    }
    
    // Create/update widgets for visible items
    for (int i = startIndex; i <= endIndex; ++i) {
        QListWidgetItem* listItem = item(i);
        if (!listItem) continue;
        
        ControlParameterWidget* widget = qobject_cast<ControlParameterWidget*>(itemWidget(listItem));
        if (!widget) {
            widget = new ControlParameterWidget();
            widget->setValueChangeCallback(m_valueChangeCallback);
            widget->setValueUpdateCallback([this](int index, uint32_t value) {
                updateStoredValue(index, value);
            });
            setItemWidget(listItem, widget);
        }
        
        widget->updateFromControl(m_controls[i], i, m_controlValues[i]);
    }
}

int VirtualizedControlList::getVisibleItemCount() const {
    if (ITEM_HEIGHT <= 0) return 0;
    return (height() / ITEM_HEIGHT) + 2;  // +2 for partial items at top/bottom
}

int VirtualizedControlList::getFirstVisibleIndex() const {
    if (ITEM_HEIGHT <= 0) return 0;
    return verticalScrollBar()->value() / ITEM_HEIGHT;
}

uint32_t VirtualizedControlList::getControlValue(int controlIndex) const {
    if (controlIndex >= 0 && controlIndex < static_cast<int>(m_controlValues.size())) {
        return m_controlValues[controlIndex];
    }
    return 0;  // Default value if index is invalid
}

void VirtualizedControlList::updateStoredValue(int controlIndex, uint32_t value) {
    if (controlIndex >= 0 && controlIndex < static_cast<int>(m_controlValues.size())) {
        m_controlValues[controlIndex] = value;
    }
}

