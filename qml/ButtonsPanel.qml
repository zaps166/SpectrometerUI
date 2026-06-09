// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore
import SpectrometerUI_files

Pane {
    id: root

    property bool controlEnabled
    property bool saveEnabled

    signal openStoredMeasurements()
    signal loadFile(path: url)

    bottomPadding: AppQmlSingleton.isMobile ? 0 : topPadding

    implicitWidth: layout.implicitWidth + root.leftPadding + root.rightPadding
    implicitHeight: layout.implicitHeight + root.topPadding + root.bottomPadding

    RowLayout {
        id: layout

        anchors.fill: parent

        Button {
            id: singleButton

            enabled: !continuousButton.checked && root.controlEnabled
            text: qsTr("Single")

            onClicked: {
                SpectrometerBridge.start(true)
            }
        }

        Button {
            id: continuousButton

            enabled: root.controlEnabled
            checkable: true
            text: qsTr("Continuous")

            onEnabledChanged: {
                if (!enabled && checked) {
                    checked = false
                }
            }

            onToggled: {
                if (checked) {
                    SpectrometerBridge.start(false)
                } else {
                    SpectrometerBridge.stop()
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Button {
            id: loadButton

            enabled: !continuousButton.checked
            text: qsTr("Open")

            MyMenu {
                id: openMenu
                MenuItem {
                    text: deviceOpenDialog.headerText
                    onTriggered: {
                        deviceOpenDialog.open()
                    }
                }
                MenuSeparator {}
                MenuItem {
                    text: qsTr("Stored measurements")
                    onTriggered: {
                        root.openStoredMeasurements()
                    }
                }
                MenuItem {
                    text: qsTr("Open a file")
                    onTriggered: {
                        fileDialogLoader.fileMode =  FileDialog.OpenFile
                        fileDialogLoader.active = true
                    }
                }
                MenuSeparator {}
                MenuItem {
                    text: dIlluminantOpenDialog.headerText
                    onTriggered: {
                        dIlluminantOpenDialog.open()
                    }
                }
                MenuItem {
                    text: bbOpenDialog.headerText
                    onTriggered: {
                        bbOpenDialog.open()
                    }
                }
            }

            onClicked: {
                openMenu.popup()
            }
        }

        Button {
            id: saveButton

            enabled: root.saveEnabled
            text: qsTr("Save")

            MeasurementNameDialog {
                id: measurementNameDialog

                onAccepted: {
                    if (!SpectrometerBridge.storeDataInternal(measurementName)) {
                        errorDialog.open()
                    }
                }
            }

            MyMenu {
                id: saveMenu
                MenuItem {
                    text: qsTr("Store measurement")
                    onTriggered: {
                        measurementNameDialog.open()
                    }
                }
                MenuItem {
                    text: qsTr("Save to a file")
                    onTriggered: {
                        fileDialogLoader.fileMode =  FileDialog.SaveFile
                        fileDialogLoader.active = true
                    }
                }
            }

            onClicked: {
                saveMenu.popup()
            }
        }
    }

    BBOpenDialog {
        id: bbOpenDialog
    }

    DIlluminantOpenDialog {
        id: dIlluminantOpenDialog
    }

    DeviceOpenDialog {
        id: deviceOpenDialog
    }

    Loader {
        id: fileDialogLoader

        property int fileMode

        active: false

        sourceComponent: FileDialog {
            id: fileDialog

            fileMode: fileDialogLoader.fileMode
            nameFilters: AppQmlSingleton.isMobile ? [] : ["CSV files (*.csv)"]
            defaultSuffix: "csv"
            currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]

            Component.onCompleted: {
                open()
            }

            onAccepted: {
                if (fileMode === FileDialog.SaveFile) {
                    if (!SpectrometerBridge.storeData(selectedFile)) {
                        errorDialog.open()
                    }
                } else if (fileMode === FileDialog.OpenFile) {
                    root.loadFile(selectedFile)
                }
                fileDialogLoader.active = false
            }

            onRejected: {
                fileDialogLoader.active = false
            }
        }
    }

    MyMessageDialog {
        id: errorDialog
        title: qsTr("Measurement")
        text: qsTr("Unable to store the measurement")
    }
}
