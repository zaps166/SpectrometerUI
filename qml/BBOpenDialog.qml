// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

MyDialog {
    id: root

    function validate() {
        validateMinMax(minWavelength.value, maxWavelength.value)
    }

    headerText: qsTr("Black body radiation")

    onVisibleChanged: {
        if (visible) {
            temperature.forceActiveFocus()
        }
    }

    ColumnLayout {
        RowLayout {
            Label {
                Layout.fillWidth: true
                text: qsTr("Temperature:")
            }
            MyDoubleSpinBox {
                id: temperature
                decimals: 0
                wheelEnabled: true
                from: 500
                to: 1000000
                value: 4000
                textFromValue: (value, decimals, locale) => {
                    return "%1 K".arg(Number(value).toLocaleString(locale, 'f', decimals))
                }
            }
        }

        RowLayout {
            Label {
                Layout.fillWidth: true
                text: qsTr("Minimum wavelength:")
            }
            WavelengthSpinBox {
                id: minWavelength
                from: 1
                value: 360
                onValueChanged: {
                    root.validate()
                }
            }
        }

        RowLayout {
            Label {
                Layout.fillWidth: true
                text: qsTr("Maximum wavelength:")
            }
            WavelengthSpinBox {
                id: maxWavelength
                from: 1
                value: 830
                onValueChanged: {
                    root.validate()
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
        SpectrometerBridge.create("bb:/?temperature=%1&min=%2&max=%3&normalize=%4"
                                  .arg(temperature.value)
                                  .arg(minWavelength.value)
                                  .arg(maxWavelength.value)
                                  .arg(normalize.value))
    }
}
