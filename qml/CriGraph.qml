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

    readonly property list<color> rColors: [
        "#e8a7b0",
        "#ccb184",
        "#abc161",
        "#6fc59f",
        "#7fc2df",
        "#8fb6ff",
        "#cca5ff",
        "#eea3ef",
        "#e9214c",
        "#fff456",
        "#0cac8a",
        "#005bc0",
        "#ffe8da",
        "#6c7d4f",
        "#c39b7b",
    ]

    CriPriv {
        id: priv
    }

    Row {
        id: row

        anchors.fill: parent
        spacing: 8

        Repeater {
            model: priv.rValues

            Item {
                id: delegate

                required property int index
                required property real modelData

                width: (parent.width - (row.spacing * (priv.rValues.length - 1))) / priv.rValues.length
                height: parent.height

                Rectangle {
                    visible: priv.valid
                    width: parent.width
                    height: Math.max(0, delegate.modelData / 100) * (parent.height - 40)
                    color: root.rColors[delegate.index]
                    anchors.bottom: labelId.top
                    anchors.bottomMargin: 5

                    Label {
                        text: Math.round(delegate.modelData)
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.top
                        font.pixelSize: 10
                    }
                }

                Label {
                    id: labelId
                    text: "R" + (delegate.index + 1)
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: 11
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
        text: "CRI"
        font.bold: true
    }
}
