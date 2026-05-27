// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

DoubleSpinBox {
    id: root

    property real pageStep: stepSize * 10

    editable: true

    Keys.onPressed: (event) => {
        switch (event.key) {
            case Qt.Key_PageUp: {
                root.value = Math.min(root.to, root.value + pageStep)
                root.valueModified()
                event.accepted = true
                break
            }
            case Qt.Key_PageDown: {
                root.value = Math.max(root.from, root.value - pageStep)
                root.valueModified()
                event.accepted = true
                break
            }
        }
    }

    valueFromText: (text, locale) => {
        const match = text.replace(/\s+/g, "").match(/[\d\.,]+/)
        if (match) {
            return Number.fromLocaleString(locale, match[0])
        }
        return value
    }
}
