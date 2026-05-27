// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls
import SpectrometerUI_files

ApplicationWindow {
    id: win

    Material.theme: Material.System

    property MainView mainView: null
    property bool canClose: false

    flags: Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint

    visible: true

    width: 550
    height: 600

    minimumWidth: AppQmlSingleton.isMobile ? 1 : stackView.currentItem.implicitWidth
    minimumHeight: AppQmlSingleton.isMobile ? 1 : stackView.currentItem.implicitHeight

    Component.onCompleted: {
        SpectrometerBridge.getWindowColorProfile(this)
        SpectrometerBridge.createDevice(false)
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: mainViewComponent
        focus: true

        Keys.onBackPressed: (event) => {
            if (depth > 1) {
                pop()
                event.accepted = true
            } else {
                event.accepted = false
            }
        }
    }

    Component {
        id: mainViewComponent

        MainView {
            Component.onCompleted: {
                win.mainView = this
            }
            Component.onDestruction: {
                win.mainView = null
            }

            onOpenStoredMeasurements: {
                stackView.push(storedMeasurementsComponent)
            }
        }
    }

    property real storedMeasurementsMinNm
    property real storedMeasurementsMaxNm
    Component {
        id: storedMeasurementsComponent

        StoredMeasurements {
            minNm: win.storedMeasurementsMinNm
            maxNm: win.storedMeasurementsMaxNm

            onClose: (path) => {
                if (path !== "") {
                    SpectrometerBridge.create("file://" + path + "?min=" + minNm + "&max=" + maxNm + "&b64=1")
                    win.storedMeasurementsMinNm = minNm
                    win.storedMeasurementsMaxNm = maxNm
                }
                stackView.pop()
            }
        }
    }

    Connections {
        target: SpectrometerBridge

        function onQuit() {
            win.canClose = true
            win.close()
        }
    }

    onClosing: (event) => {
        if (!canClose) {
            mainView.waiting = true
            SpectrometerBridge.requestQuit()
            event.accepted = false
        }
    }
}
