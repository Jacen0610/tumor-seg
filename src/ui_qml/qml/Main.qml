import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import TumorSegUi


ApplicationWindow {
    id: root
    width: 1800
    height: 950
    visible: true
    title: "Brain Tumor查看器"


    color: "#ffffff"

    FolderDialog {
        id: folderDialog
        title: "选择文件夹"
        onAccepted: Backend.openCase(selectedFolder)
    }

    Dialog {
        id: modelInfoDialog
        title: "模型信息"
        standardButtons: Dialog.Ok
        anchors.centerIn: parent
        width: 640
        height: 300
        modal: true
        onAccepted: modelInfoDialog.close()

        RowLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 16

            // 左侧：文字信息区域（占宽度1/2）
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width / 2
                spacing: 12

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Label {
                        width: parent.width
                        text: Backend.modelInfoText()
                        font.family: "monospace"
                        font.pixelSize: 16
                        wrapMode: Label.Wrap
                        color: palette.text
                    }
                }
            }

            // 右侧：图片区域（占宽度1/2）
            Image {
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width / 2
                Layout.fillHeight: true
                source: "qrc:/qt/qml/TumorSegUi/assets/demo.png" // 替换成你的图路径
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
        }
    }

    Dialog {
        id: errorDialog
        title: "错误"
        standardButtons: Dialog.Ok
        anchors.centerIn: parent
        property string message: ""
        Label { text: errorDialog.message; wrapMode: Text.WordWrap }
    }

    Connections {
        target: Backend
        function onErrorOccurred(message) {
            errorDialog.message = message
            errorDialog.open()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // ---------- 左侧: 6面板视图区 ----------
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.78
            columns: 3
            rowSpacing: 8
            columnSpacing: 8

            Repeater {
                model: 3
                delegate: ColumnLayout {
                    required property int index
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 4

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#101010"
                        border.color: "#3a3a3a"
                        border.width: 1

                        Image {
                            anchors.fill: parent
                            anchors.margins: 2
                            fillMode: Image.PreserveAspectFit
                            source: "image://sliceprovider/raw" + index + "/" + Backend.imageRevision
                            cache: false
                            smooth: true
                        }

                        Rectangle {
                            anchors.top: parent.top
                            anchors.left: parent.left
                            color: "#80000000"
                            width: label.width + 8
                            height: label.height + 4
                            Label {
                                id: label
                                anchors.centerIn: parent
                                text: "原始图 - 轴" + index
                                color: "white"
                                font.pixelSize: 11
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#101010"
                        border.color: "#3a3a3a"
                        border.width: 1

                        Image {
                            anchors.fill: parent
                            anchors.margins: 2
                            fillMode: Image.PreserveAspectFit
                            source: "image://sliceprovider/overlay" + index + "/" + Backend.imageRevision
                            cache: false
                            smooth: true
                        }

                        Rectangle {
                            anchors.top: parent.top
                            anchors.left: parent.left
                            color: "#80000000"
                            width: label2.width + 8
                            height: label2.height + 4
                            Label {
                                id: label2
                                anchors.centerIn: parent
                                text: "叠加图 - 轴" + index
                                color: "white"
                                font.pixelSize: 11
                            }
                        }
                    }

                    Slider {
                        Layout.fillWidth: true
                        from: 0
                        to: {
                            if (index === 0) return Backend.slice0Max
                            if (index === 1) return Backend.slice1Max
                            return Backend.slice2Max
                        }
                        stepSize: 1
                        value: {
                            if (index === 0) return Backend.slice0
                            if (index === 1) return Backend.slice1
                            return Backend.slice2
                        }
                        onMoved: {
                            if (index === 0) Backend.slice0 = value
                            else if (index === 1) Backend.slice1 = value
                            else Backend.slice2 = value
                        }
                    }
                }
            }
        }

        // ---------- 右侧: 控制面板 ----------
        ColumnLayout {
            Layout.preferredWidth: parent.width * 0.20
            Layout.fillHeight: true
            spacing: 12

            Button {
                text: "打开文件夹"
                Layout.fillWidth: true
                onClicked: folderDialog.open()
            }

            Label {
                text: Backend.caseInfo
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                color: "#dddddd"
            }

            Button {
                text: Backend.isInferring ? "推理中..." : "运行 AI 分割"
                Layout.fillWidth: true
                enabled: Backend.hasCase && !Backend.isInferring
                onClicked: Backend.runInference()
            }

            Button {
                text: "模型信息"
                Layout.fillWidth: true
                onClicked: modelInfoDialog.open()
            }

            GroupBox {
                title: "显示模态"
                Layout.fillWidth: true
                ComboBox {
                    anchors.fill: parent
                    model: ["FLAIR", "T1", "T1ce", "T2"]
                    currentIndex: Backend.modalityIndex
                    onActivated: Backend.modalityIndex = currentIndex
                }
            }

            GroupBox {
                title: "分割区域显示"
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    CheckBox {
                        text: "坏死/非增强核心 (NCR/NET)"
                        checked: Backend.showNCR
                        onToggled: Backend.showNCR = checked
                    }
                    CheckBox {
                        text: "瘤周水肿 (ED)"
                        checked: Backend.showED
                        onToggled: Backend.showED = checked
                    }
                    CheckBox {
                        text: "增强肿瘤 (ET)"
                        checked: Backend.showET
                        onToggled: Backend.showET = checked
                    }
                }
            }

            GroupBox {
                title: "叠加透明度"
                Layout.fillWidth: true
                Slider {
                    anchors.fill: parent
                    from: 0.0
                    to: 1.0
                    value: Backend.opacity
                    onMoved: Backend.opacity = value
                }
            }

            Item { Layout.fillHeight: true } // 底部留白，撑开布局
        }
    }
}
