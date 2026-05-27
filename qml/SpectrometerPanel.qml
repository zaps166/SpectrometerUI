// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import SpectrometerUI_files

Pane {
    id: root

    property bool hasExpositionControl: false
    property bool waiting: false

    readonly property bool isStaticData: staticDataName !== ""
    property string staticDataName

    implicitWidth: exposureLayout.implicitWidth + root.leftPadding + root.rightPadding
    implicitHeight: Math.max(exposureLayout.implicitHeight, staticDataLayout.implicitHeight) + root.topPadding + root.bottomPadding

    focusPolicy: Qt.ClickFocus

    topPadding: 0
    bottomPadding: 0

    function processExposure() {
        SpectrometerBridge.setExposure(autoExposure.checked, exposureTime.value)
    }

    MyMessageDialog {
        id: errorDialog
        title: qsTr("An error occured")
    }

    Connections {
        target: SpectrometerBridge

        function onCommandsStarted() {
            root.waiting = true
        }
        function onCommandsFinished() {
            root.waiting = false
        }
        function onErrorMessage(errStr: string) {
            errorDialog.text = errStr
            errorDialog.open()
        }

        function onExposureChanged(time, ae, aeInProgress, min, max) {
            exposureTime.from = min
            exposureTime.to = max
            exposureTime.value = time
            autoExposure.checked = ae
            aeInProgressIndicator.running = aeInProgress
            root.hasExpositionControl = (time > 0)
        }

        function onStaticDataNameChanged(name) {
            root.staticDataName = name
        }
    }

    RowLayout {
        id: exposureLayout

        enabled: root.hasExpositionControl
        visible: !root.isStaticData

        anchors.fill: parent

        MyDoubleSpinBox {
            id: exposureTime

            Layout.alignment: Qt.AlignLeft

            enabled: !autoExposure.checked

            decimals: 1
            stepSize: 10

            textFromValue: (value, decimals, locale) => {
                return qsTr("Exposure: %1 ms").arg(Number(value).toLocaleString(locale, 'f', decimals))
            }

            onValueModified: {
                root.processExposure()
            }

            BusyIndicator {
                id: aeInProgressIndicator
                anchors.fill: parent
                running: false
            }
        }

        Switch {
            id: autoExposure

            Layout.alignment: Qt.AlignRight

            text: qsTr("Auto")

            onToggled: {
                root.processExposure()
            }
        }
    }

    RowLayout {
        id: staticDataLayout

        visible: root.isStaticData

        anchors.fill: parent

        Label {
            Layout.maximumWidth: staticDataLayout.width

            text: root.staticDataName
            elide: Label.ElideMiddle
        }
    }
}
