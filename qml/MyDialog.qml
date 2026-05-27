// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick
import QtQuick.Controls
import SpectrometerUI_files

Dialog {
    id: root

    property alias headerText: headerLabel.text

    property bool messageDialogMode: false

    readonly property bool inputMethodVisible: AppQmlSingleton.isMobile && Qt.inputMethod.visible
    property bool inputMethodWasVisible: false
    onInputMethodVisibleChanged: {
        inputMethodWasVisible = true
    }
    onVisibleChanged: {
        if (visible) {
            inputMethodWasVisible = false
        }
    }

    function validateMinMax(min: real, max: real) {
        standardButton(Dialog.Ok).enabled = AppQmlSingleton.validateMinMax(min, max)
    }

    parent: Overlay.overlay
    x: (parent.width - width) / 2
    y: (parent.height - (inputMethodVisible ? Qt.inputMethod.keyboardRectangle.height / 2 : 0) - height) / 2

    Behavior on y {
        enabled: root.inputMethodWasVisible

        NumberAnimation {
            duration: 250
            easing.type: Easing.OutCubic
        }
    }

    standardButtons: Dialog.Ok | Dialog.Cancel
    closePolicy: Popup.CloseOnEscape

    function clickAccept() {
        if (standardButtons & Dialog.Ok) {
            standardButton(Dialog.Ok).click()
        } else if (standardButtons & Dialog.Yes) {
            standardButton(Dialog.Yes).click()
        }
    }

    contentItem.Keys.onReturnPressed: clickAccept()
    contentItem.Keys.onEnterPressed: clickAccept()

    Overlay.modal: OverlayItem {}

    header: Label {
        id: headerLabel
        padding: root.padding
        font.bold: true
        font.pixelSize: root.messageDialogMode ? 16 : font.pixelSize
        horizontalAlignment: root.messageDialogMode ? Qt.AlignLeft : Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
    }
}
