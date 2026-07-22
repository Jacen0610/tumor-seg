#include "SliceImageProvider.h"

SliceImageProvider::SliceImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage SliceImageProvider::requestImage(const QString& id, QSize* size, const QSize& /*requestedSize*/) {
    // id形如 "raw0/17"，版本号只用来让QML感知刷新，实际取图时忽略它，只用面板标识部分
    QString panelId = id.section('/', 0, 0);

    QMutexLocker locker(&m_mutex);
    QImage image = m_images.value(panelId);

    if (image.isNull()) {
        // 还没有数据时，返回一张占位的深灰色图，避免QML报"图片加载失败"
        image = QImage(64, 64, QImage::Format_RGB888);
        image.fill(Qt::darkGray);
    }

    if (size) {
        *size = image.size();
    }
    return image;
}

void SliceImageProvider::setImage(const QString& panelId, const QImage& image) {
    QMutexLocker locker(&m_mutex);
    m_images[panelId] = image;
}