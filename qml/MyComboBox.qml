// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick
import QtQuick.Controls

ComboBox {
    id: root

    wheelEnabled: true

    popup.contentItem: MyListView {
        clip: true
        implicitHeight: contentHeight
        model: root.delegateModel
        currentIndex: root.highlightedIndex
        highlightMoveDuration: 0
    }
}
