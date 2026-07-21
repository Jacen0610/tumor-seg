#pragma once
#include <QMainWindow>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QFutureWatcher>
#include <memory>
#include "SliceView.h"
#include "../core/CaseLoader.h"
#include "../core/TensorRTInferenceEngine.h"
#include "../core/DisplayTransform.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openCase();
    void runInference();
    void onInferenceFinished();
    void refreshViews();
    void showModelInfo();

private:
    void buildUi();
    OverlaySettings currentOverlaySettings() const;

    // 上排: 原始图 3视图, 下排: 叠加图 3视图
    SliceView* m_rawViews[3];
    SliceView* m_overlayViews[3];
    QSlider* m_sliceSliders[3];

    QComboBox* m_modalityCombo;
    QLabel* m_infoLabel;
    QPushButton* m_runInferenceBtn;
    QPushButton* m_modelInfoBtn;

    QCheckBox* m_checkboxNCR;
    QCheckBox* m_checkboxED;
    QCheckBox* m_checkboxET;
    QSlider* m_opacitySlider;

    bool m_hasCase = false;
    bool m_hasPrediction = false;
    CaseLoader::LoadedCase m_currentCase;
    LabelVolume m_predictedLabel;

    std::unique_ptr<TensorRTInferenceEngine> m_engine;
    QFutureWatcher<LabelVolume>* m_inferenceWatcher;

    static constexpr float kWindowMin = -2.0f;
    static constexpr float kWindowMax = 4.0f;

    // 模型和engine路径，按实际部署位置调整
    const std::string kEnginePath = "/home/jacen/workspace/tumor_seg/models/segresnet_v1/model.engine";

    // 在MainWindow.h的成员变量里加：
    DisplayTransform m_displayTransform;
    std::array<Volume, 4> m_displayVolumes;
    LabelVolume m_displayLabel;
};