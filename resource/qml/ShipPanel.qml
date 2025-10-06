import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// shipPanel.qml
Frame {
    id: shipPanel
    width: parent.width
    // anchors.centerIn: parent
    // Layout.fillWidth: true
    // width: 580
    // height: 150
    background: Rectangle {
        color: "#ffffff"
        radius: 8
        border.color: "#3a3838"
        border.width: 1
    }

    // ---- 数据模型 ----
    // 用于动态存储设备ID列表
    ListModel {
        id: allDevicesModel
        ListElement {
            text: "设备A (发射机)"
        }
        ListElement {
            text: "设备B (接收机)"
        }
        ListElement {
            text: "设备C (天线)"
        }
        ListElement {
            text: "设备D (干扰源)"
        }
        ListElement {
            text: "设备E (控制器)"
        }
    }
    ListModel {
        id: selectedDevicesModel
        Component.onCompleted: {
            append({selectedDevice: "设备C (天线)"});
        }
    }
    // 用于动态存储天线ID列表
    ListModel {
        id: allAntennaModel
        ListElement {
            text: "设备A (发射机)"
        }
        ListElement {
            text: "设备B (接收机)"
        }
        ListElement {
            text: "设备C (天线)"
        }
        ListElement {
            text: "设备D (干扰源)"
        }
        ListElement {
            text: "设备E (控制器)"
        }
    }
    ListModel {
        id: selectedAntennaModel
        Component.onCompleted: {
            append({selectedAntenna: "设备C (天线)"});
        }
    }
    Frame {
        anchors.fill: parent
        RowLayout {
            anchors.fill: parent
            GridLayout {
                columns: 4
                Layout.fillWidth: true
                // Layout.fillWidth: true

                Label {
                    text: "X坐标"
                }
                TextField {
                    Layout.fillWidth: true
                }
                Label {
                    text: "朝向"
                }
                TextField {
                    Layout.fillWidth: true
                }
                Label {
                    text: "Y坐标"
                }
                TextField {
                    Layout.fillWidth: true
                }
                Label {
                    text: "舰速"
                }
                TextField {
                    Layout.fillWidth: true
                }
            }

            // 2. 中间：设备列表
            ColumnLayout {
                Layout.fillWidth: true
                ScrollView {
                    Label {
                        text: "搭载设备"; }
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5
                        Repeater {
                            id: deviceRepeater
                            model: selectedDeviceaModel
                            delegate: RowLayout {
                                Layout.fillWidth: true
                                Label {
                                    text: "设备ID"
                                }
                                ComboBox {
                                    id: deviceComboBox
                                    Layout.fillWidth: true
                                    model: allDevicesModel
                                }
                                Component.onCompleted: {
                                    if (model.selectedDevice) {
                                        currentIndex = find(model.selectedDevice);
                                    }
                                }
                                // onCurrentTextChanged: {
                                //     // model.setProperty 是最高效的单属性修改方式
                                //     // 'index' 由 Repeater 提供，指向当前项
                                //     selectedDevicesModel.setProperty(index, "selectedDevice", currentText);
                                // }
                            }
                            Button {
                                text: "-"
                                font.bold: true
                                // 5. 删除按钮的逻辑
                                onClicked: {
                                    // 'index' 由 Repeater 提供，直接删除模型中对应的项
                                    selectedDevicesModel.remove(index);
                                }
                            }
                        }
                    }
                }
            }
            // 3. 右侧：天线列表
            GroupBox {
                title: "天线"
                // Layout.preferredWidth: 3
                // Layout.fillHeight: true
                // Layout.fillWidth: true

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ColumnLayout {
                        id: antennaListLayout
                        Layout.fillWidth: true
                        spacing: 5

                        // Repeater会根据antennaModel动态创建天线项
                        Repeater {
                            model: antennaModel

                            RowLayout {
                                Label {
                                    text: "天线"
                                } // 自动编号
                                TextField {
                                    placeholderText: "天线ID"; Layout.fillWidth: true
                                }
                                Button {
                                    text: "-"
                                    Layout.alignment: Qt.AlignHCenter
                                    onClicked: {
                                        // 删除当前项
                                        antennaModel.remove(index)
                                    }
                                }
                            }
                        }
                    }


                    // 添加天线按钮
                    Button {
                        text: "+"
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: {
                            // 核心逻辑: 向模型添加一个空对象，Repeater会自动创建新UI
                            antennaModel.append({})
                        }
                    }
                }
            }
        }
    }
}