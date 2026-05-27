// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick
import QtQuick.Controls

MyDialog {
    id: root

    property alias text: label.text

    messageDialogMode: true

    headerText: title

    standardButtons: Dialog.Ok

    onOpened: {
        standardButton(Dialog.Ok)?.forceActiveFocus()
        standardButton(Dialog.No)?.forceActiveFocus()
    }

    Label {
        id: label

        anchors.fill: parent
    }
}
