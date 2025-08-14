#pragma once

#include <QListWidget>
#include <QScrollBar>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QWidget>
#include <vector>
#include <functional>

namespace midicci::commonproperties {
    struct MidiCIControl;
}

class ControlParameterWidget : public QWidget {
    Q_OBJECT

public:
    explicit ControlParameterWidget(QWidget* parent = nullptr);
    
    void updateFromControl(const midicci::commonproperties::MidiCIControl& control, int controlIndex, uint32_t currentValue);
    void setValueChangeCallback(std::function<void(int, const midicci::commonproperties::MidiCIControl&, uint32_t)> callback);
    void setValueUpdateCallback(std::function<void(int, uint32_t)> callback);  // Callback to update stored values
    void updateValue(uint32_t value);  // Update slider value without triggering MIDI callback

signals:
    void valueChanged(int controlIndex, uint32_t value);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onSliderValueChanged(int value);

private:
    QLabel* m_titleLabel;
    QSlider* m_slider;
    QLabel* m_valueLabel;
    QSpinBox* m_noteSpinBox;  // For per-note controls
    QHBoxLayout* m_layout;
    
    int m_controlIndex;
    midicci::commonproperties::MidiCIControl* m_currentControl;
    std::function<void(int, const midicci::commonproperties::MidiCIControl&, uint32_t)> m_valueChangeCallback;
    std::function<void(int, uint32_t)> m_valueUpdateCallback;  // Callback to update stored values
    
    // Store original MIDI range for conversion
    uint32_t m_midiMin, m_midiMax;
    bool m_needsScaling;
};

class VirtualizedControlList : public QListWidget {
    Q_OBJECT

public:
    explicit VirtualizedControlList(QWidget* parent = nullptr);
    
    void setControls(const std::vector<midicci::commonproperties::MidiCIControl>& controls);
    void setValueChangeCallback(std::function<void(int, const midicci::commonproperties::MidiCIControl&, uint32_t)> callback);
    uint32_t getControlValue(int controlIndex) const;  // Get stored value for a control

protected:
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;

private slots:
    void updateVisibleItems();

private:
    void ensureVisibleItemsExist();
    int getVisibleItemCount() const;
    int getFirstVisibleIndex() const;
    void updateStoredValue(int controlIndex, uint32_t value);  // Update stored value for a control
    
    std::vector<midicci::commonproperties::MidiCIControl> m_controls;
    std::vector<uint32_t> m_controlValues;  // Store current values for each control
    std::function<void(int, const midicci::commonproperties::MidiCIControl&, uint32_t)> m_valueChangeCallback;
    
    static constexpr int ITEM_HEIGHT = 35;
    static constexpr int BUFFER_ITEMS = 5;  // Extra items to render above/below visible area
};
