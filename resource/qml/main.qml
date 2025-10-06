import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// import "Electricity" 1.0
// import Electromagnetic_Compatibility 1.0
ApplicationWindow {
    id: window
    visible: true
    width: 1000
    height: 800
    title: "无人船舰队电磁兼容预测"
    color: "#ffffff" // 窗口背景色

    // ---- 数据模型 ----
    // 使用ListModel来动态存储设备列表
    // 点击按钮时，我们只需要向这个model里添加一个空对象即可
    ListModel {
        id: deviceModel
        // 初始时可以有一个或多个设备
        Component.onCompleted: { append({}) }
    }
    ListModel {
        id: shipModel
        Component.onCompleted: { append({}) } // 初始时有一个船舶
    }
    // ---- 主界面布局 ----
    RowLayout {
        anchors.fill: parent
        // 1. 左侧结构树区域
        Frame {
            Layout.preferredWidth: 200
            Layout.fillWidth: true
            Layout.fillHeight: true
            //// TODO : 创建结构树
            Label {
                text: "结构树"
                // anchors.centerIn: parent
            }
        }

        // 2. 右侧主内容区域
        Frame {
            Layout.preferredWidth: 800
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent
                spacing: 6
                TabBar {
                    id: mainTabBar
                    Layout.fillWidth: true
                    Layout.minimumHeight: 50
                    Layout.fillHeight: true
                    TabButton {text: "创建设备"}
                    TabButton {text: "创建舰队"}
                    TabButton {text: "开始仿真"}
                }

                // 2.2 Tab页面切换区域
                StackLayout {
                    id: mainStack
                    Layout.fillWidth: true
                    Layout.minimumHeight: 100
                    Layout.fillHeight: true
                    currentIndex: mainTabBar.currentIndex

                    // "创建设备" 页面
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        id: createDevicePage
                        clip: false

                        ColumnLayout {
                            spacing: 10
                            anchors.fill: parent
                            Repeater {
                                Layout.fillWidth: true
                                model: deviceModel
                                delegate: DevicePanel {}
                            }

                            Button {
                                text: "添加设备"
                                Layout.alignment: Qt.AlignRight
                                onClicked: { deviceModel.append({}) }
                            }
                            Item {Layout.preferredHeight: 10}
                        }
                    }

                    // "创建舰队" 页面
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        id: createFleetPage
                        clip: false

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10

                            Repeater {
                                Layout.fillWidth: true
                                model: shipModel
                                delegate: ShipPanel {}
                            }
                            Button {
                                text: "添加船舶"
                                Layout.alignment: Qt.AlignRight
                                onClicked: { shipModel.append({}) }
                            }
                            Item {Layout.preferredHeight: 10}
                        }
                    }

                    // "开始仿真" 页面 (占位)
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        id: simulationPage
                        Label {
                            text: "开始仿真页面"
                            anchors.centerIn: parent
                        }
                    }
                }

                Frame {
                    Layout.preferredHeight: 150
                    Layout.fillWidth: true
                    TabBar {
                        id: logTabBar
                        TabButton { text: "问题" }
                        TabButton { text: "输出" }
                    }
                    StackLayout {
                        anchors.fill: parent
                        currentIndex: logTabBar.currentIndex
                    }
                }
            }
        }
    }
}
