// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SpectrometerUI_files

MyListView {
    id: root

    spacing: 8

    delegate: RowLayout {
        id: delegate

        width: root.width - root.rightMargin

        required property int index
        required property string id
        required property string key
        required property string value

        spacing: 20

        MyMenu {
            id: menu
            MenuItem {
                text: qsTr("Copy")
                onTriggered: {
                    SpectrometerBridge.copyToClipboard(delegate.id)
                }
            }
        }

        Label {
            Layout.alignment: Qt.AlignLeft
            Layout.minimumWidth: AppQmlSingleton.measurementsViewFirstColWidth

            Component.onCompleted: {
                if (AppQmlSingleton.measurementsViewFirstColWidth < implicitWidth) {
                    AppQmlSingleton.measurementsViewFirstColWidth = implicitWidth
                }
            }

            text: delegate.index + 1
        }

        Label {
            Layout.fillWidth: true

            text: delegate.key
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.alignment: Qt.AlignRight

            text: delegate.value
            wrapMode: Text.WordWrap
        }

        TapHandler {
            onLongPressed: {
                if (AppQmlSingleton.isMobile) {
                    menu.popup()
                }
            }
        }

        ContextMenu.onRequested: {
            menu.popup()
        }
    }
}
