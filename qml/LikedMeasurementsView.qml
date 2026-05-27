// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SpectrometerUI_files

GridLayout {
    id: root

    readonly property int length: 6
    property list<string> texts: Array(length).fill("")
    property list<string> ids: Array(length).fill("")

    columns: length / 2
    rows: length / 3

    uniformCellWidths: true
    uniformCellHeights: true

    rowSpacing: 2
    columnSpacing: 2

    function updateTexts() {
        const idRole = SpectrometerBridge.getIdRole()
        const likedPosRole = SpectrometerBridge.getLikedPosRole()
        const model = SpectrometerBridge
        const rowCount = model.rowCount()
        let likedPosVisited = Array(root.length).fill(false)
        for (let i = 0; i < rowCount; ++i) {
            const modelIndex = model.index(i, 0)

            const likedPos = model.data(modelIndex, likedPosRole)
            if (likedPos < 0 || likedPos >= root.length) {
                continue
            }

            root.texts[likedPos] = model.data(modelIndex)
            root.ids[likedPos] = model.data(modelIndex, idRole)
            likedPosVisited[likedPos] = true
        }
        for (let i = 0; i < likedPosVisited.length; ++i) {
            if (!likedPosVisited[i]) {
                root.texts[i] = ""
                root.ids[i] = ""
            }
        }
    }

    Connections {
        target: SpectrometerBridge

        function onModelReset() {
            root.updateTexts()
        }

        function onDataChanged() {
            root.updateTexts()
        }
    }

    Repeater {
        model: root.length

        ToolButton {
            id: button

            required property int index
            readonly property string id: root.ids[index]

            Layout.fillWidth: true
            Layout.fillHeight: true

            font.bold: false

            text: id === "" ? qsTr("Tap to add") : root.texts[index]

            onClicked: {
                popup.open()
            }

            LikedChooserDialog {
                id: popup

                onVisibleChanged: {
                    if (visible) {
                        selectedId = button.id
                    }
                }

                onAccepted: {
                    const initialId = button.id
                    if (initialId !== selectedId) {
                        if (initialId !== "") {
                            SpectrometerBridge.setLiked(initialId, -1)
                        }
                        if (selectedId !== "") {
                            SpectrometerBridge.setLiked(selectedId, button.index)
                        }
                    }
                }
            }
        }
    }
}
