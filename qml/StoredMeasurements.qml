// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SpectrometerUI_files

Page {
    id: root

    property alias minNm: minNmSpinBox.value
    property alias maxNm: maxNmSpinBox.value

    signal close(path: string)

    Component.onCompleted: {
        buttons.standardButton(DialogButtonBox.Close).forceActiveFocus()
    }

    title: qsTr("Select the stored measurement")

    header: Label {
        padding: 4
        font.bold: true
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        text: root.title
    }

    function updateMeasurementList() {
        list.model = SpectrometerBridge.getStoredMeasurements()
    }

    function validate() {
        list.enabled = AppQmlSingleton.validateMinMax(minNm, maxNm)
    }

    onFocusChanged: {
        if (focus) {
            list.forceActiveFocus()
        }
    }

    MyMessageDialog {
        id: deleteDialog
        title: qsTr("Stored measurements")
        text: qsTr("Do you want to delete: \"%1\"?").arg(menu.name)
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: {
            if (SpectrometerBridge.deleteStoredMeasurement(menu.name)) {
                root.updateMeasurementList()
            }
        }
    }

    MyMessageDialog {
        id: errorDialog
        title: qsTr("Measurement")
        text: qsTr("Unable to rename the measurement")
    }

    MeasurementNameDialog {
        id: measurementNameDialog

        property string initialMeasurementName

        onAccepted: {
            if (SpectrometerBridge.renameDataInternal(initialMeasurementName, measurementName)) {
                root.updateMeasurementList()
            } else {
                errorDialog.open()
            }
        }

        onVisibleChanged: {
            if (!visible) {
                initialMeasurementName = ""
            }
        }
    }

    Menu {
        id: menu
        property string name
        property string path
        modal: true
        Overlay.modal: OverlayItem {}
        MenuItem {
            text: qsTr("Open")
            onTriggered: {
                root.close(menu.path)
            }
        }
        MenuItem {
            text: qsTr("Rename")
            onTriggered: {
                measurementNameDialog.initialMeasurementName = menu.name
                measurementNameDialog.measurementName = menu.name
                measurementNameDialog.open()
            }
        }
        MenuItem {
            text: qsTr("Delete")
            onTriggered: {
                deleteDialog.open()
            }
        }
    }

    MyListView {
        id: list

        anchors.fill: parent
        clip: true

        model: SpectrometerBridge.getStoredMeasurements()

        delegate: ItemDelegate {
            id: delegate

            required property string name
            required property string date
            required property string path

            implicitWidth: list.width - list.rightMargin

            text: name

            highlighted: ListView.isCurrentItem

            Label {
                anchors {
                    top: delegate.top
                    right: delegate.right
                    bottom: delegate.bottom
                    rightMargin: delegate.leftPadding
                }
                text: delegate.date
                verticalAlignment: Qt.AlignVCenter
                font.pixelSize: 10
            }

            TapHandler {
                onTapped: {
                    root.close(delegate.path)
                }
                onLongPressed: {
                    if (AppQmlSingleton.isMobile) {
                        delegate.prepareMenu()
                        menu.popup(point.scenePosition.x, point.scenePosition.y - root.header.height)
                    }
                }
            }

            Keys.onReturnPressed: {
                root.close(delegate.path)
            }

            ContextMenu.onRequested: {
                prepareMenu()
                menu.popup()
            }

            function prepareMenu() {
                menu.name = name
                menu.path = path
            }
        }
    }

    footer: RowLayout {
        Pane {
            RowLayout {
                ColumnLayout {
                    Label {
                        Layout.fillHeight: true
                        text: qsTr("Minimum wavelength:")
                        verticalAlignment: Qt.AlignVCenter
                    }
                    Label {
                        Layout.fillHeight: true
                        text: qsTr("Maximum wavelength:")
                        verticalAlignment: Qt.AlignVCenter
                    }
                }
                ColumnLayout {
                    WavelengthSpinBox {
                        id: minNmSpinBox
                        onValueChanged: {
                            root.validate()
                        }
                    }

                    WavelengthSpinBox {
                        id: maxNmSpinBox
                        onValueChanged: {
                            root.validate()
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        DialogButtonBox {
            id: buttons

            Layout.alignment: Qt.AlignBottom

            standardButtons: DialogButtonBox.Close

            onRejected: {
                root.close("")
            }

            background: Item {}
        }
    }
}
