// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick

MyDoubleSpinBox {
    id: root

    wheelEnabled: true

    decimals: 0

    from: 0
    to: 10000

    textFromValue: (value, decimals, locale) => {
        return value === 0 ? qsTr("No limit") : "%1 nm".arg(Number(value).toLocaleString(locale, 'f', decimals))
    }
}
