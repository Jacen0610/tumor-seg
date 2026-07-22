#pragma once
#include <QQuickImageProvider>
#include <QHash>
#include <QMutex>

// 把动态生成的切片图像(QImage)提供给QML的Image组件显示
// QML端通过 "image://sliceprovider/<panelId>/<version>" 这样的URL请求图片
class SliceImageProvider : public QQuickImageProvider {
public:
    SliceImageProvider();

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

    // 供Backend调用，更新某个面板(如"raw0"、"overlay1")对应的图像
    void setImage(const QString& panelId, const QImage& image);

private:
    QHash<QString, QImage> m_images;
    QMutex m_mutex;
};