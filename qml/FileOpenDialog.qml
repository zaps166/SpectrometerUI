// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

MyDialog {
    id: fileOpenDialog

    property string path

    headerText: qsTr("Select irradiance column")

    function validate() {
        validateMinMax(minWavelength.value, maxWavelength.value)
    }

    onVisibleChanged: {
        if (visible) {
            colVal.forceActiveFocus()
            validate()
        }
    }

    ColumnLayout {
        RowLayout {
            Label {
                text: qsTr("Column:")
            }
            MyDoubleSpinBox {
                id: colVal
                from: 1
                to: 100
                decimals: 0
                wheelEnabled: true
            }
        }

        RowLayout {
            Label {
                Layout.fillWidth: true
                text: qsTr("Minimum wavelength:")
            }
            WavelengthSpinBox {
                id: minWavelength
                onValueChanged: {
                    fileOpenDialog.validate()
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
                onValueChanged: {
                    fileOpenDialog.validate()
                }
            }
        }
    }

    onAccepted: {
        SpectrometerBridge.create(path + "?col=" + colVal.value + "&min=" + minWavelength.value + "&max=" + maxWavelength.value)
        path = ""
    }
}
