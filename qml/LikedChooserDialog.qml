// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import SpectrometerUI_files

MyDialog {
    id: root

    property string selectedId

    headerText: qsTr("Select parameter")

    Loader {
        active: root.visible

        anchors.fill: parent

        sourceComponent: MyListView {
            id: list

            anchors.fill: parent

            clip: true

            implicitWidth: Overlay?.overlay ? Overlay.overlay.width * 0.6 : 0
            implicitHeight: Overlay?.overlay ? Overlay.overlay.height * 0.5 : 0

            model: SpectrometerBridge

            delegate: CheckDelegate {
                required property string id
                required property string key

                width: list.width - list.rightMargin

                text: key

                checked: root.selectedId === id

                onToggled: {
                    root.selectedId = checked ? id : ""
                }
            }
        }
    }
}
