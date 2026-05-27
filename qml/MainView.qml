// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SpectrometerUI_files

Item {
    id: root

    property bool waiting: false

    signal openStoredMeasurements()

    implicitWidth: splitter.implicitWidth
    implicitHeight: splitter.implicitHeight

    Component.onCompleted: {
        splitter.restoreState(SpectrometerBridge.restoreSplitView())
    }
    Component.onDestruction: {
        SpectrometerBridge.storeSplitView(splitter.saveState())
    }

    Drawer {
        id: drawer

        width: 240
        height: parent.height
        edge: Qt.LeftEdge
        interactive: root.visible

        Overlay.modal: OverlayItem {}

        Column {
            anchors.fill: parent
            padding: 10
            spacing: 10
            Label {
                text: Application.displayName + " - v" + Application.version
                font.bold: true
            }
            CheckBox {
                id: drawOutlineCheckBox
                text: qsTr("Spectrum outline")
                checked: spectrumGraph.drawOutline
            }
        }
    }

    FileOpenDialog {
        id: fileOpenDialog
    }

    DropArea {
        anchors.fill: parent

        onEntered: (drag) => {
            if (drag.hasUrls && drag.urls.length === 1) {
                drag.acceptProposedAction()
            } else {
                drag.accepted = false
            }
        }

        onDropped: (drop) => {
            if (drop.hasUrls && drop.urls.length === 1) {
                fileOpenDialog.path = drop.urls[0]
                fileOpenDialog.open()
                drop.accept()
            }
        }
    }

    SplitView {
        id: splitter

        anchors.fill: parent
        orientation: Qt.Vertical

        implicitWidth: Math.max(topLayout.implicitWidth, bottomLayout.implicitWidth)
        implicitHeight: topLayout.implicitHeight + bottomLayout.implicitHeight

        ColumnLayout {
            id: topLayout

            SplitView.minimumHeight: topLayout.implicitHeight
            SplitView.fillHeight: true

            spacing: 0

            MySwipeView {
                id: view

                Layout.fillWidth: true
                Layout.fillHeight: true

                interactive: !AppQmlSingleton.spectrumPointingWavelength

                currentIndex: indicator.currentIndex

                Spectrum {
                    id: spectrumGraph

                    visible: view.shouldBeVisible(this)

                    drawOutline: drawOutlineCheckBox.checked
                }

                CriGraph {
                    id: criGraph

                    visible: view.shouldBeVisible(this)
                }

                Tm30Graph {
                    id: tm30Graph

                    visible: view.shouldBeVisible(this)
                }

                DuvGraph {
                    id: duvGraph

                    visible: view.shouldBeVisible(this)
                }
            }

            PageIndicator {
                id: indicator

                Layout.alignment: Qt.AlignCenter

                interactive: true

                count: view.count
                currentIndex: view.currentIndex

                MouseArea {
                    anchors.fill: parent

                    acceptedButtons: Qt.NoButton

                    onWheel: (wheel) => {
                        if (wheel.angleDelta.y >= 120) {
                            view.setCurrentIndex(Math.max(0, view.currentIndex - 1))
                        } else if (wheel.angleDelta.y <= -120) {
                            view.setCurrentIndex(Math.min(view.count - 1, view.currentIndex + 1))
                        }
                    }
                }
            }
        }

        ColumnLayout {
            id: bottomLayout

            SplitView.minimumHeight: bottomLayout.implicitHeight

            spacing: 0

            MeasurementsPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            SpectrometerPanel {
                id: spectrometerPanel

                Layout.fillWidth: true
            }

            ButtonsPanel {
                Layout.fillWidth: true

                controlEnabled: spectrometerPanel.hasExpositionControl
                saveEnabled: spectrumGraph.hasData

                onOpenStoredMeasurements: {
                    root.openStoredMeasurements()
                }

                onLoadFile: (path) => {
                    fileOpenDialog.path = path
                    fileOpenDialog.open()
                }
            }
        }
    }

    // Busy overlay
    OverlayItem {
        parent: Overlay.overlay
        anchors.fill: parent
        focus: visible
        visible: opacity > 0
        opacity: busyIndicator.running ? 1 : 0

        Keys.onPressed: (event) => {
            event.accepted = true
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            preventStealing: true
            acceptedButtons: Qt.AllButtons
        }

        BusyIndicator {
            id: busyIndicator

            anchors.fill: parent
            anchors.margins: Math.min(Overlay.overlay.width, Overlay.overlay.height) * 0.4

            running: spectrometerPanel.waiting || root.waiting
        }
    }
}
