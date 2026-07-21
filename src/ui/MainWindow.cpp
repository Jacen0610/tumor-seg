#include "MainWindow.h"
#include "../core/DisplayTransform.h"
#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QtConcurrent/QtConcurrent>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

// 在case文件夹里找到flair模态的nii文件路径，用于计算显示方向校正规则
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

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("BraTS2020 分割结果查看器 (C++版)");
    resize(1500, 950);

    m_inferenceWatcher = new QFutureWatcher<LabelVolume>(this);
    connect(m_inferenceWatcher, &QFutureWatcher<LabelVolume>::finished,
            this, &MainWindow::onInferenceFinished);

    buildUi();
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLayout = new QHBoxLayout(central);

    // ---------- 左侧: 6面板视图区 ----------
    auto* viewWidget = new QWidget(central);
    auto* gridLayout = new QGridLayout(viewWidget);

    const char* rawTitles[3] = {"原始图 - 轴0", "原始图 - 轴1", "原始图 - 轴2"};
    const char* overlayTitles[3] = {"叠加图 - 轴0", "叠加图 - 轴1", "叠加图 - 轴2"};

    for (int i = 0; i < 3; ++i) {
        m_rawViews[i] = new SliceView(rawTitles[i], viewWidget);
        m_overlayViews[i] = new SliceView(overlayTitles[i], viewWidget);
        m_sliceSliders[i] = new QSlider(Qt::Horizontal, viewWidget);
        connect(m_sliceSliders[i], &QSlider::valueChanged, this, &MainWindow::refreshViews);

        gridLayout->addWidget(m_rawViews[i], 0, i);
        gridLayout->addWidget(m_overlayViews[i], 1, i);
        gridLayout->addWidget(m_sliceSliders[i], 2, i);
    }

    mainLayout->addWidget(viewWidget, /*stretch=*/4);

    // ---------- 右侧: 控制面板 ----------
    auto* controlPanel = new QWidget(central);
    auto* controlLayout = new QVBoxLayout(controlPanel);

    auto* openBtn = new QPushButton("打开 Case 文件夹", controlPanel);
    connect(openBtn, &QPushButton::clicked, this, &MainWindow::openCase);
    controlLayout->addWidget(openBtn);

    m_infoLabel = new QLabel("未加载数据", controlPanel);
    m_infoLabel->setWordWrap(true);
    controlLayout->addWidget(m_infoLabel);

    m_runInferenceBtn = new QPushButton("运行 AI 分割", controlPanel);
    m_runInferenceBtn->setEnabled(false);
    connect(m_runInferenceBtn, &QPushButton::clicked, this, &MainWindow::runInference);
    controlLayout->addWidget(m_runInferenceBtn);

    m_modelInfoBtn = new QPushButton("模型信息", controlPanel);
    connect(m_modelInfoBtn, &QPushButton::clicked, this, &MainWindow::showModelInfo);
    controlLayout->addWidget(m_modelInfoBtn);

    // 模态选择
    auto* modalityGroup = new QGroupBox("显示模态", controlPanel);
    auto* modalityLayout = new QVBoxLayout(modalityGroup);
    m_modalityCombo = new QComboBox(modalityGroup);
    m_modalityCombo->addItems({"FLAIR", "T1", "T1ce", "T2"});
    connect(m_modalityCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refreshViews);
    modalityLayout->addWidget(m_modalityCombo);
    controlLayout->addWidget(modalityGroup);

    // 分割类别开关
    auto* labelGroup = new QGroupBox("分割区域显示", controlPanel);
    auto* labelLayout = new QVBoxLayout(labelGroup);
    m_checkboxNCR = new QCheckBox("坏死/非增强核心 (NCR/NET)", labelGroup);
    m_checkboxED = new QCheckBox("瘤周水肿 (ED)", labelGroup);
    m_checkboxET = new QCheckBox("增强肿瘤 (ET)", labelGroup);
    for (auto* cb : {m_checkboxNCR, m_checkboxED, m_checkboxET}) {
        cb->setChecked(true);
        connect(cb, &QCheckBox::stateChanged, this, &MainWindow::refreshViews);
    }
    labelLayout->addWidget(m_checkboxNCR);
    labelLayout->addWidget(m_checkboxED);
    labelLayout->addWidget(m_checkboxET);
    controlLayout->addWidget(labelGroup);

    // 叠加透明度
    auto* opacityGroup = new QGroupBox("叠加透明度", controlPanel);
    auto* opacityLayout = new QVBoxLayout(opacityGroup);
    m_opacitySlider = new QSlider(Qt::Horizontal, opacityGroup);
    m_opacitySlider->setRange(0, 100);
    m_opacitySlider->setValue(50);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &MainWindow::refreshViews);
    opacityLayout->addWidget(m_opacitySlider);
    controlLayout->addWidget(opacityGroup);

    controlLayout->addStretch();
    mainLayout->addWidget(controlPanel, /*stretch=*/1);
}

OverlaySettings MainWindow::currentOverlaySettings() const {
    OverlaySettings s;
    s.showNCR = m_checkboxNCR->isChecked();
    s.showED = m_checkboxED->isChecked();
    s.showET = m_checkboxET->isChecked();
    s.opacity = m_opacitySlider->value() / 100.0f;
    return s;
}

void MainWindow::openCase() {
    QString dir = QFileDialog::getExistingDirectory(this, "选择Case文件夹");
    if (dir.isEmpty()) return;

    try {
        // 1. 按原有逻辑加载(裁剪+归一化)，这份数据将来喂给模型推理，完全不做任何方向改动
        m_currentCase = CaseLoader::loadCase(dir.toStdString());
        m_hasCase = true;
        m_hasPrediction = false;

        // 2. 计算显示方向校正规则(基于flair模态的nii头信息)
        std::string flairPath = findFlairPath(dir.toStdString());
        m_displayTransform = computeDisplayTransform(flairPath);

        // 3. 生成仅用于界面显示的体积副本(4个模态)，模型推理路径不受任何影响
        for (int i = 0; i < 4; ++i) {
            m_displayVolumes[i] = remapVolumeForDisplay(m_currentCase.modalityVolumes[i], m_displayTransform);
        }

        const Volume& displayRef = m_displayVolumes[0];
        m_sliceSliders[0]->setRange(0, static_cast<int>(displayRef.D) - 1);
        m_sliceSliders[0]->setValue(static_cast<int>(displayRef.D) / 2);
        m_sliceSliders[1]->setRange(0, static_cast<int>(displayRef.H) - 1);
        m_sliceSliders[1]->setValue(static_cast<int>(displayRef.H) / 2);
        m_sliceSliders[2]->setRange(0, static_cast<int>(displayRef.W) - 1);
        m_sliceSliders[2]->setValue(static_cast<int>(displayRef.W) / 2);

        m_infoLabel->setText(QString("当前case: %1\n尺寸: (%2, %3, %4)\n尚未运行分割")
            .arg(QString::fromStdString(m_currentCase.caseName))
            .arg(displayRef.D).arg(displayRef.H).arg(displayRef.W));

        m_runInferenceBtn->setEnabled(true);
        refreshViews();

    } catch (const std::exception& e) {
        QMessageBox::warning(this, "加载失败", QString::fromStdString(e.what()));
    }
}

void MainWindow::runInference() {
    if (!m_hasCase) return;

    m_runInferenceBtn->setEnabled(false);
    m_runInferenceBtn->setText("推理中...");

    // 懒加载引擎(第一次点击时才加载，避免程序启动就占用GPU显存)
    if (!m_engine) {
        try {
            m_engine = std::make_unique<TensorRTInferenceEngine>(kEnginePath);
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "引擎加载失败", QString::fromStdString(e.what()));
            m_runInferenceBtn->setEnabled(true);
            m_runInferenceBtn->setText("运行 AI 分割");
            return;
        }
    }

    // 异步执行推理，输入依然是原始(未做显示校正)的数据，保证和训练/验证时完全一致
    auto future = QtConcurrent::run([this]() -> LabelVolume {
        return m_engine->runSlidingWindowInference(m_currentCase.modalityVolumes, 0.5f);
    });
    m_inferenceWatcher->setFuture(future);
}

void MainWindow::onInferenceFinished() {
    m_predictedLabel = m_inferenceWatcher->result();
    m_hasPrediction = true;

    // 把推理结果(模型坐标空间)映射到显示坐标空间，和m_displayVolumes对齐
    m_displayLabel = remapLabelVolumeForDisplay(m_predictedLabel, m_displayTransform);

    m_runInferenceBtn->setEnabled(true);
    m_runInferenceBtn->setText("运行 AI 分割");

    const Volume& displayRef = m_displayVolumes[0];
    m_infoLabel->setText(QString("当前case: %1\n尺寸: (%2, %3, %4)\n分割已完成")
        .arg(QString::fromStdString(m_currentCase.caseName))
        .arg(displayRef.D).arg(displayRef.H).arg(displayRef.W));

    refreshViews();
}

void MainWindow::showModelInfo() {
    QString info =
        "模型信息\n"
        "─────────────────────\n"
        "架构: SegResNet (MONAI)\n"
        "训练数据: BraTS2020, 320 cases\n"
        "验证集: 49 cases (未参与训练)\n\n"
        "验证集性能 (滑窗推理，完整体积评估):\n"
        "  全肿瘤 (WT)    Dice: 0.9198\n"
        "  肿瘤核心 (TC)  Dice: 0.8379\n"
        "  增强肿瘤 (ET)  Dice: 0.7336";

    QMessageBox::information(this, "模型信息", info);
}

void MainWindow::refreshViews() {
    if (!m_hasCase) return;

    int modalityIdx = m_modalityCombo->currentIndex();
    const Volume& volume = m_displayVolumes[modalityIdx];
    OverlaySettings settings = currentOverlaySettings();

    for (int axis = 0; axis < 3; ++axis) {
        size_t index = static_cast<size_t>(m_sliceSliders[axis]->value());
        Slice2D graySlice = SliceExtractor::extract(volume, axis, index);
        graySlice = SliceExtractor::rotate90CCW(graySlice);

        m_rawViews[axis]->setGraySlice(graySlice, kWindowMin, kWindowMax);

        if (m_hasPrediction) {
            LabelSlice2D labelSlice = SliceExtractor::extract(m_displayLabel, axis, index);
            labelSlice = SliceExtractor::rotate90CCW(labelSlice);
            m_overlayViews[axis]->setOverlaySlice(graySlice, &labelSlice, kWindowMin, kWindowMax, settings);
        } else {
            m_overlayViews[axis]->setOverlaySlice(graySlice, nullptr, kWindowMin, kWindowMax, settings);
        }
    }
}