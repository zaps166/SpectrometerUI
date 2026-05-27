// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick
import QtQuick.Controls
import SpectrometerUI_files

MyDialog {
    id: root

    property alias measurementName: measurementTextField.text

    Component.onCompleted: {
        root.standardButton(Dialog.Ok).enabled = false
    }

    headerText: qsTr("Measurement name")

    implicitWidth: {
        if (Overlay && Overlay.overlay) {
            return Overlay.overlay.width * 0.7
        }
        return 0
    }

    TextField {
        id: measurementTextField

        anchors.fill: parent

        onTextChanged: {
            if (root.visible) {
                root.standardButton(Dialog.Ok).enabled = SpectrometerBridge.canStoreDataInternal(text)
            }
        }
    }

    onVisibleChanged: {
        if (visible) {
            measurementTextField.forceActiveFocus()
            SpectrometerBridge.setDataInternalCache(true)
        } else {
            SpectrometerBridge.setDataInternalCache(false)
            measurementTextField.text = ""
        }
    }
}
