// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import SpectrometerUI_files

Pane {
    id: root

    implicitWidth: 380
    implicitHeight: 120

    readonly property list<color> binColors: [
        "#e7423f", "#e9633a", "#ec8531", "#efa72b",
        "#f2ca26", "#bfc832", "#6dc04f", "#00b76d",
        "#00ab92", "#009eb5", "#0090ce", "#1a74d2",
        "#5b55c5", "#9747a8", "#c63e8b", "#e03e6a",
    ]

    Tm30GraphPriv {
        id: priv
    }

    Row {
        id: row

        anchors.fill: parent
        spacing: 4

        Repeater {
            model: priv.rfValues

            Item {
                id: delegate

                required property int index
                required property real modelData

                width: (parent.width - (row.spacing * (priv.rfValues.length - 1))) / priv.rfValues.length
                height: parent.height

                Rectangle {
                    visible: priv.valid
                    width: parent.width
                    height: Math.max(0, delegate.modelData / 100) * (parent.height - 40)
                    color: root.binColors[delegate.index]
                    anchors.bottom: labelId.top
                    anchors.bottomMargin: 5

                    Label {
                        text: Math.round(delegate.modelData)
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.top
                        font.pixelSize: 9
                    }
                }

                Label {
                    id: labelId
                    text: "h" + (delegate.index + 1)
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: 10
                }
            }
        }
    }

    Label {
        anchors {
            top: parent.top
            right: parent.right
            topMargin: -root.topPadding + 4
            rightMargin: -root.rightPadding + 4
        }
        text: "TM-30 Rf"
        font.bold: true
    }
}
