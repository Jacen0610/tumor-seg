#pragma once
#include <QObject>
#include <QFutureWatcher>
#include <memory>
#include "SliceImageProvider.h"
#include "../core/CaseLoader.h"
#include "../core/TensorRTInferenceEngine.h"
#include "../core/DisplayTransform.h"
#include "../core/OverlaySettings.h"
#include "src/core/SliceExtractor.h"

class Backend : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString caseInfo READ caseInfo NOTIFY caseInfoChanged)
    Q_PROPERTY(bool hasCase READ hasCase NOTIFY hasCaseChanged)
    Q_PROPERTY(bool hasPrediction READ hasPrediction NOTIFY hasPredictionChanged)
    Q_PROPERTY(bool isInferring READ isInferring NOTIFY isInferringChanged)

    Q_PROPERTY(int slice0Max READ slice0Max NOTIFY dimensionsChanged)
    Q_PROPERTY(int slice1Max READ slice1Max NOTIFY dimensionsChanged)
    Q_PROPERTY(int slice2Max READ slice2Max NOTIFY dimensionsChanged)

    Q_PROPERTY(int slice0 READ slice0 WRITE setSlice0 NOTIFY sliceIndexChanged)
    Q_PROPERTY(int slice1 READ slice1 WRITE setSlice1 NOTIFY sliceIndexChanged)
    Q_PROPERTY(int slice2 READ slice2 WRITE setSlice2 NOTIFY sliceIndexChanged)

    Q_PROPERTY(int modalityIndex READ modalityIndex WRITE setModalityIndex NOTIFY overlaySettingsChanged)
    Q_PROPERTY(bool showNCR READ showNCR WRITE setShowNCR NOTIFY overlaySettingsChanged)
    Q_PROPERTY(bool showED READ showED WRITE setShowED NOTIFY overlaySettingsChanged)
    Q_PROPERTY(bool showET READ showET WRITE setShowET NOTIFY overlaySettingsChanged)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY overlaySettingsChanged)

    Q_PROPERTY(int imageRevision READ imageRevision NOTIFY imagesUpdated)

public:
    explicit Backend(SliceImageProvider* imageProvider, QObject* parent = nullptr);

    QString caseInfo() const { return m_caseInfo; }
    bool hasCase() const { return m_hasCase; }
    bool hasPrediction() const { return m_hasPrediction; }
    bool isInferring() const { return m_isInferring; }

    int slice0Max() const;
    int slice1Max() const;
    int slice2Max() const;

    int slice0() const { return m_slice0; }
    int slice1() const { return m_slice1; }
    int slice2() const { return m_slice2; }
    void setSlice0(int v);
    void setSlice1(int v);
    void setSlice2(int v);

    int modalityIndex() const { return m_modalityIndex; }
    void setModalityIndex(int v);
    bool showNCR() const { return m_overlaySettings.showNCR; }
    void setShowNCR(bool v);
    bool showED() const { return m_overlaySettings.showED; }
    void setShowED(bool v);
    bool showET() const { return m_overlaySettings.showET; }
    void setShowET(bool v);
    qreal opacity() const { return m_overlaySettings.opacity; }
    void setOpacity(qreal v);

    int imageRevision() const { return m_imageRevision; }

public slots:
    void openCase(const QString& folderPath); // folderPath形如 "file:///home/xxx/case"
    void runInference();
    QString modelInfoText() const;

signals:
    void caseInfoChanged();
    void hasCaseChanged();
    void hasPredictionChanged();
    void isInferringChanged();
    void dimensionsChanged();
    void sliceIndexChanged();
    void overlaySettingsChanged();
    void imagesUpdated();
    void errorOccurred(const QString& message);

private slots:
    void onInferenceFinished();

private:
    void updateAllImages();
    QImage buildGrayImage(const Slice2D& slice, float windowMin, float windowMax) const;
    QImage buildOverlayImage(const Slice2D& graySlice, const LabelSlice2D* labelSlice,
                              float windowMin, float windowMax) const;

    SliceImageProvider* m_imageProvider;

    bool m_hasCase = false;
    bool m_hasPrediction = false;
    bool m_isInferring = false;
    QString m_caseInfo = "未加载数据";

    CaseLoader::LoadedCase m_currentCase;
    DisplayTransform m_displayTransform;
    std::array<Volume, 4> m_displayVolumes;
    LabelVolume m_predictedLabel;
    LabelVolume m_displayLabel;

    int m_slice0 = 0, m_slice1 = 0, m_slice2 = 0;
    int m_modalityIndex = 0;
    OverlaySettings m_overlaySettings;

    int m_imageRevision = 0;

    std::unique_ptr<TensorRTInferenceEngine> m_engine;
    QFutureWatcher<LabelVolume>* m_inferenceWatcher;

    static constexpr float kWindowMin = -2.0f;
    static constexpr float kWindowMax = 4.0f;
    const std::string kEnginePath = "/home/jacen/workspace/tumor_seg/models/segresnet_v1/model.engine";
};
