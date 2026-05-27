// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

MyDialog {
    id: root

    headerText: qsTr("Daylight (D-illuminant)")

    onVisibleChanged: {
        if (visible) {
            cct.forceActiveFocus()
        }
    }

    ColumnLayout {
        RowLayout {
            Label {
                Layout.fillWidth: true
                text: qsTr("CCT:")
            }
            MyDoubleSpinBox {
                id: cct
                decimals: 0
                wheelEnabled: true
                from: 3000
                to: 40000
                value: 6500
                textFromValue: (value, decimals, locale) => {
                    return "%1 K".arg(Number(value).toLocaleString(locale, 'f', decimals))
                }
            }
        }

        RowLayout {
            Label {
                Layout.fillWidth: true
                text: qsTr("Normalize:")
            }
            MyDoubleSpinBox {
                id: normalize
                decimals: 2
                wheelEnabled: true
                stepSize: 0.1
                from: 0.1
                to: 10
                value: 1
            }
        }
    }

    onAccepted: {
        SpectrometerBridge.create("daylight:/?cct=%1&normalize=%2".arg(cct.value).arg(normalize.value))
    }
}
