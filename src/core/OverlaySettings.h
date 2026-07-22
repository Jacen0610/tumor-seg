#pragma once

// 分割结果叠加显示的配置，纯数据结构，不依赖任何UI框架，
// Widgets版(ui_widgets)和QML版(ui_qml)共用同一份定义
struct OverlaySettings {
    bool showNCR = true;
    bool showED = true;
    bool showET = true;
    float opacity = 0.5f;
};