#pragma once
#include <QLabel>
#include <QResizeEvent>
#include <array>
#include "../core/SliceExtractor.h"
#include "../core/OverlaySettings.h"

class SliceView : public QLabel {
    Q_OBJECT
public:
    explicit SliceView(const QString& placeholderText, QWidget* parent = nullptr);

    // 只显示灰度图，不叠加(用于上排"原始图"面板)
    void setGraySlice(const Slice2D& slice, float windowMin, float windowMax);

    // 灰度图 + 分割结果叠加(用于下排"叠加"面板)，labelSlice传nullptr表示暂无分割结果(只显示灰度)
    void setOverlaySlice(const Slice2D& graySlice, const LabelSlice2D* labelSlice,
                         float windowMin, float windowMax, const OverlaySettings& settings);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QImage m_currentImage;
    void updateDisplay();
};