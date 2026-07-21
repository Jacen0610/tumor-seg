#include "SliceView.h"
#include <QPixmap>
#include <algorithm>

// BraTS标签颜色约定(RGB)，和之前Python原型保持一致
// 1=NCR/NET(坏死) 红色, 2=ED(水肿) 绿色, 3=ET(增强肿瘤,对应原始标签4) 黄色
static const std::array<std::array<int, 3>, 4> LABEL_COLORS = {{
    {0, 0, 0},       // 0 背景，不使用
    {255, 60, 60},   // 1 NCR/NET
    {60, 220, 60},   // 2 ED
    {255, 220, 40},  // 3 ET
}};

SliceView::SliceView(const QString& placeholderText, QWidget* parent) : QLabel(parent) {
    setAlignment(Qt::AlignCenter);
    setStyleSheet("background-color: #101010; color: #888;");
    setMinimumSize(200, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setText(placeholderText);
}

void SliceView::setGraySlice(const Slice2D& slice, float windowMin, float windowMax) {
    if (slice.rows == 0 || slice.cols == 0) return;

    QImage image(static_cast<int>(slice.cols), static_cast<int>(slice.rows), QImage::Format_RGB888);

    float range = windowMax - windowMin;
    if (range < 1e-6f) range = 1e-6f;

    for (size_t r = 0; r < slice.rows; ++r) {
        uchar* line = image.scanLine(static_cast<int>(r));
        for (size_t c = 0; c < slice.cols; ++c) {
            float v = slice.data[r * slice.cols + c];
            float clipped = std::clamp(v, windowMin, windowMax);
            uchar gray = static_cast<uchar>((clipped - windowMin) / range * 255.0f);
            line[c * 3 + 0] = gray;
            line[c * 3 + 1] = gray;
            line[c * 3 + 2] = gray;
        }
    }

    m_currentImage = image;
    updateDisplay();
}

void SliceView::setOverlaySlice(const Slice2D& graySlice, const LabelSlice2D* labelSlice,
                                 float windowMin, float windowMax, const OverlaySettings& settings) {
    if (graySlice.rows == 0 || graySlice.cols == 0) return;

    QImage image(static_cast<int>(graySlice.cols), static_cast<int>(graySlice.rows), QImage::Format_RGB888);

    float range = windowMax - windowMin;
    if (range < 1e-6f) range = 1e-6f;

    for (size_t r = 0; r < graySlice.rows; ++r) {
        uchar* line = image.scanLine(static_cast<int>(r));
        for (size_t c = 0; c < graySlice.cols; ++c) {
            float v = graySlice.data[r * graySlice.cols + c];
            float clipped = std::clamp(v, windowMin, windowMax);
            float gray = (clipped - windowMin) / range * 255.0f;

            float rf = gray, gf = gray, bf = gray;

            if (labelSlice != nullptr) {
                uint8_t label = labelSlice->data[r * labelSlice->cols + c];
                bool shouldShow = (label == 1 && settings.showNCR) ||
                                   (label == 2 && settings.showED) ||
                                   (label == 3 && settings.showET);
                if (shouldShow) {
                    const auto& color = LABEL_COLORS[label];
                    rf = gray * (1.0f - settings.opacity) + color[0] * settings.opacity;
                    gf = gray * (1.0f - settings.opacity) + color[1] * settings.opacity;
                    bf = gray * (1.0f - settings.opacity) + color[2] * settings.opacity;
                }
            }

            line[c * 3 + 0] = static_cast<uchar>(std::clamp(rf, 0.0f, 255.0f));
            line[c * 3 + 1] = static_cast<uchar>(std::clamp(gf, 0.0f, 255.0f));
            line[c * 3 + 2] = static_cast<uchar>(std::clamp(bf, 0.0f, 255.0f));
        }
    }

    m_currentImage = image;
    updateDisplay();
}

void SliceView::updateDisplay() {
    if (m_currentImage.isNull()) return;
    QPixmap pixmap = QPixmap::fromImage(m_currentImage);
    setPixmap(pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void SliceView::resizeEvent(QResizeEvent* event) {
    QLabel::resizeEvent(event);
    updateDisplay();
}