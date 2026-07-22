#include "Backend.h"
#include "../core/SliceExtractor.h"
#include <QtConcurrent/QtConcurrent>
#include <QUrl>
#include <filesystem>
#include <stdexcept>
#include <algorithm>

namespace fs = std::filesystem;

static const std::array<int, 4> LABEL_COLOR_R = {0, 255, 60, 255};
static const std::array<int, 4> LABEL_COLOR_G = {0, 60, 220, 220};
static const std::array<int, 4> LABEL_COLOR_B = {0, 60, 60, 40};

static std::string findFlairPath(const std::string& caseDir) {
    for (const auto& entry : fs::directory_iterator(caseDir)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();
        auto endsWith = [&](const std::string& s, const std::string& suf) {
            return s.size() >= suf.size() &&
                   s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
        };
        if (endsWith(filename, "_flair.nii") || endsWith(filename, "_flair.nii.gz")) {
            return entry.path().string();
        }
    }
    throw std::runtime_error("在case文件夹中找不到flair模态文件: " + caseDir);
}

Backend::Backend(SliceImageProvider* imageProvider, QObject* parent)
    : QObject(parent), m_imageProvider(imageProvider) {
    m_inferenceWatcher = new QFutureWatcher<LabelVolume>(this);
    connect(m_inferenceWatcher, &QFutureWatcher<LabelVolume>::finished,
            this, &Backend::onInferenceFinished);
}

int Backend::slice0Max() const { return m_hasCase ? static_cast<int>(m_displayVolumes[0].D) - 1 : 0; }
int Backend::slice1Max() const { return m_hasCase ? static_cast<int>(m_displayVolumes[0].H) - 1 : 0; }
int Backend::slice2Max() const { return m_hasCase ? static_cast<int>(m_displayVolumes[0].W) - 1 : 0; }

void Backend::setSlice0(int v) { if (m_slice0 != v) { m_slice0 = v; emit sliceIndexChanged(); updateAllImages(); } }
void Backend::setSlice1(int v) { if (m_slice1 != v) { m_slice1 = v; emit sliceIndexChanged(); updateAllImages(); } }
void Backend::setSlice2(int v) { if (m_slice2 != v) { m_slice2 = v; emit sliceIndexChanged(); updateAllImages(); } }

void Backend::setModalityIndex(int v) { if (m_modalityIndex != v) { m_modalityIndex = v; emit overlaySettingsChanged(); updateAllImages(); } }
void Backend::setShowNCR(bool v) { if (m_overlaySettings.showNCR != v) { m_overlaySettings.showNCR = v; emit overlaySettingsChanged(); updateAllImages(); } }
void Backend::setShowED(bool v) { if (m_overlaySettings.showED != v) { m_overlaySettings.showED = v; emit overlaySettingsChanged(); updateAllImages(); } }
void Backend::setShowET(bool v) { if (m_overlaySettings.showET != v) { m_overlaySettings.showET = v; emit overlaySettingsChanged(); updateAllImages(); } }
void Backend::setOpacity(qreal v) { if (m_overlaySettings.opacity != v) { m_overlaySettings.opacity = static_cast<float>(v); emit overlaySettingsChanged(); updateAllImages(); } }

void Backend::openCase(const QString& folderPath) {
    QString localPath = QUrl(folderPath).toLocalFile();
    if (localPath.isEmpty()) localPath = folderPath; // 兼容直接传普通路径的情况

    try {
        m_currentCase = CaseLoader::loadCase(localPath.toStdString());
        m_hasCase = true;
        m_hasPrediction = false;

        std::string flairPath = findFlairPath(localPath.toStdString());
        m_displayTransform = computeDisplayTransform(flairPath);

        for (int i = 0; i < 4; ++i) {
            m_displayVolumes[i] = remapVolumeForDisplay(m_currentCase.modalityVolumes[i], m_displayTransform);
        }

        m_slice0 = static_cast<int>(m_displayVolumes[0].D) / 2;
        m_slice1 = static_cast<int>(m_displayVolumes[0].H) / 2;
        m_slice2 = static_cast<int>(m_displayVolumes[0].W) / 2;

        m_caseInfo = QString("当前case: %1\n尺寸: (%2, %3, %4)\n尚未运行分割")
            .arg(QString::fromStdString(m_currentCase.caseName))
            .arg(m_displayVolumes[0].D).arg(m_displayVolumes[0].H).arg(m_displayVolumes[0].W);

        emit hasCaseChanged();
        emit hasPredictionChanged();
        emit dimensionsChanged();
        emit sliceIndexChanged();
        emit caseInfoChanged();

        updateAllImages();

    } catch (const std::exception& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void Backend::runInference() {
    if (!m_hasCase || m_isInferring) return;

    m_isInferring = true;
    emit isInferringChanged();

    if (!m_engine) {
        try {
            m_engine = std::make_unique<TensorRTInferenceEngine>(kEnginePath);
        } catch (const std::exception& e) {
            emit errorOccurred(QString::fromStdString(e.what()));
            m_isInferring = false;
            emit isInferringChanged();
            return;
        }
    }

    auto future = QtConcurrent::run([this]() -> LabelVolume {
        return m_engine->runSlidingWindowInference(m_currentCase.modalityVolumes, 0.5f);
    });
    m_inferenceWatcher->setFuture(future);
}

void Backend::onInferenceFinished() {
    m_predictedLabel = m_inferenceWatcher->result();
    m_displayLabel = remapLabelVolumeForDisplay(m_predictedLabel, m_displayTransform);
    m_hasPrediction = true;
    m_isInferring = false;

    m_caseInfo = QString("当前case: %1\n尺寸: (%2, %3, %4)\n分割已完成")
        .arg(QString::fromStdString(m_currentCase.caseName))
        .arg(m_displayVolumes[0].D).arg(m_displayVolumes[0].H).arg(m_displayVolumes[0].W);

    emit hasPredictionChanged();
    emit isInferringChanged();
    emit caseInfoChanged();

    updateAllImages();
}

QString Backend::modelInfoText() const {
    return
        "架构: SegResNet (MONAI)\n"
        "训练数据: BraTS2020, 320 cases\n"
        "验证集: 49 cases (未参与训练)\n\n"
        "验证集性能 (滑窗推理，完整体积评估):\n"
        "  全肿瘤 (WT)    Dice: 0.9198\n"
        "  肿瘤核心 (TC)  Dice: 0.8379\n"
        "  增强肿瘤 (ET)  Dice: 0.7336";
}

QImage Backend::buildGrayImage(const Slice2D& slice, float windowMin, float windowMax) const {
    QImage image(static_cast<int>(slice.cols), static_cast<int>(slice.rows), QImage::Format_RGB888);
    float range = std::max(windowMax - windowMin, 1e-6f);

    for (size_t r = 0; r < slice.rows; ++r) {
        uchar* line = image.scanLine(static_cast<int>(r));
        for (size_t c = 0; c < slice.cols; ++c) {
            float v = std::clamp(slice.data[r * slice.cols + c], windowMin, windowMax);
            uchar gray = static_cast<uchar>((v - windowMin) / range * 255.0f);
            line[c * 3 + 0] = gray;
            line[c * 3 + 1] = gray;
            line[c * 3 + 2] = gray;
        }
    }
    return image;
}

QImage Backend::buildOverlayImage(const Slice2D& graySlice, const LabelSlice2D* labelSlice,
                                    float windowMin, float windowMax) const {
    QImage image(static_cast<int>(graySlice.cols), static_cast<int>(graySlice.rows), QImage::Format_RGB888);
    float range = std::max(windowMax - windowMin, 1e-6f);
    float opacity = m_overlaySettings.opacity;

    for (size_t r = 0; r < graySlice.rows; ++r) {
        uchar* line = image.scanLine(static_cast<int>(r));
        for (size_t c = 0; c < graySlice.cols; ++c) {
            float v = std::clamp(graySlice.data[r * graySlice.cols + c], windowMin, windowMax);
            float gray = (v - windowMin) / range * 255.0f;
            float rf = gray, gf = gray, bf = gray;

            if (labelSlice != nullptr) {
                uint8_t label = labelSlice->data[r * labelSlice->cols + c];
                bool show = (label == 1 && m_overlaySettings.showNCR) ||
                            (label == 2 && m_overlaySettings.showED) ||
                            (label == 3 && m_overlaySettings.showET);
                if (show) {
                    rf = gray * (1 - opacity) + LABEL_COLOR_R[label] * opacity;
                    gf = gray * (1 - opacity) + LABEL_COLOR_G[label] * opacity;
                    bf = gray * (1 - opacity) + LABEL_COLOR_B[label] * opacity;
                }
            }
            line[c * 3 + 0] = static_cast<uchar>(std::clamp(rf, 0.0f, 255.0f));
            line[c * 3 + 1] = static_cast<uchar>(std::clamp(gf, 0.0f, 255.0f));
            line[c * 3 + 2] = static_cast<uchar>(std::clamp(bf, 0.0f, 255.0f));
        }
    }
    return image;
}

void Backend::updateAllImages() {
    if (!m_hasCase) return;

    const Volume& volume = m_displayVolumes[m_modalityIndex];
    int indices[3] = {m_slice0, m_slice1, m_slice2};

    for (int axis = 0; axis < 3; ++axis) {
        Slice2D graySlice = SliceExtractor::extract(volume, axis, static_cast<size_t>(indices[axis]));
        graySlice = SliceExtractor::rotate90CCW(graySlice);

        QImage rawImg = buildGrayImage(graySlice, kWindowMin, kWindowMax);
        m_imageProvider->setImage(QString("raw%1").arg(axis), rawImg);

        QImage overlayImg;
        if (m_hasPrediction) {
            LabelSlice2D labelSlice = SliceExtractor::extract(m_displayLabel, axis, static_cast<size_t>(indices[axis]));
            labelSlice = SliceExtractor::rotate90CCW(labelSlice);
            overlayImg = buildOverlayImage(graySlice, &labelSlice, kWindowMin, kWindowMax);
        } else {
            overlayImg = buildOverlayImage(graySlice, nullptr, kWindowMin, kWindowMax);
        }
        m_imageProvider->setImage(QString("overlay%1").arg(axis), overlayImg);
    }

    m_imageRevision++;
    emit imagesUpdated();
}