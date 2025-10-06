// 可复用的设备面板组件
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: devicePanel
    width: parent.width
    // Layout.fillWidth: true
    background: Rectangle {
        color: "#ffffff"
        radius: 8
        border.color: "#3a3838"
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        // anchors.margins: 10

        // --- 第一行: 设备类型、坐标、指向 ---
        GridLayout {
            Layout.fillWidth: true
            columns: 6

            Label { text: "设备类型" }
            ComboBox {
                Layout.fillWidth: false
                // property alias deviceType: model.text
                model: ["通用设备", "发射机", "接收机", "收发一体机"]
            }

            Label { text: "偏移坐标" }
            // property alias deviceOffset: deviceOffsetField.text
            TextField { id: deviceOffsetField; placeholderText: "X坐标" }

            Label { text: "天线指向" }
            TextField { placeholderText: "Theta" }

            // 第二列的Y坐标和Phi，为了对齐放在这里
            Item { Layout.column: 1; Layout.row: 1 } // 占位
            Item { Layout.column: 1; Layout.row: 1 } // 占位
            Label { text: " "; Layout.column: 2; Layout.row: 1 } // 占位对齐
            TextField {
                Layout.column: 3
                Layout.row: 1
                placeholderText: "Y坐标"
            }

            Item { Layout.column: 4; Layout.row: 1 } // 占位
            TextField {
                Layout.column: 5
                Layout.row: 1
                placeholderText: "Phi"
            }
        }

        // --- 第二行: ID、增益、天线类型 ---
        GridLayout {
            Layout.fillWidth: true
            columns: 7

            Label { text: "设备ID" }
            TextField { Layout.fillWidth: true }

            Label { text: "发射/接收增益" }
            TextField { placeholderText: "dBm" }

            Label { text: "天线类型" }
            ComboBox {
                Layout.fillWidth: true
                model: ["全向天线", "定向天线"]
            }
            Button { text: "导入方向图" }
        }

        // --- 第三行: 天线/滤波器
        GridLayout {
            Layout.fillWidth: true
            columns: 4
            Label { text: "天线端口增益" }
            TextField { placeholderText: "dBm"; Layout.fillWidth: true }
            Label { text: "滤波器类型" }
            TextField { placeholderText: "dBm"; Layout.fillWidth: true }
        }

        // --- 第四行: 接收机/发射机参数 ---
        RowLayout {
            Layout.fillWidth: true

            // 接收机参数
            GroupBox {
                title: "接收机参数"
                Layout.fillWidth: true
                GridLayout {
                    columns: 2
                    width: parent.width

                    Label { text: "接收机灵敏度" }
                    TextField { placeholderText: "dBm"; Layout.fillWidth: true }
                    Label { text: "接收机带宽" }
                    TextField { placeholderText: "dBm"; Layout.fillWidth: true }
                    Label { text: "发送设备ID" }
                    TextField { Layout.fillWidth: true }
                    Label { text: "干扰裕度" }
                    TextField { placeholderText: "dBm"; Layout.fillWidth: true }
                    Label { text: "噪声系数" }
                    TextField { placeholderText: "dBm"; Layout.fillWidth: true }
                    Label { text: "信噪比阈值" }
                    TextField { placeholderText: "dBm"; Layout.fillWidth: true }
                }
            }

            // 发射机参数
            GroupBox {
                title: "发射机参数"
                Layout.fillWidth: true
                GridLayout {
                    columns: 2
                    width: parent.width

                    Label { text: "发射功率" }
                    TextField { placeholderText: "dBm"; Layout.fillWidth: true }
                    Label { text: "发射带宽" }
                    TextField { placeholderText: "dBm"; Layout.fillWidth: true }
                    Label { text: "端口损耗" }
                    TextField { placeholderText: "dBm"; Layout.fillWidth: true }
                }
            }
        }
    }
}
