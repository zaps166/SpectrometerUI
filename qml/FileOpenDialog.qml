// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

MyDialog {
    id: fileOpenDialog

    property string path

    headerText: qsTr("Loading spectrum from the file")

    function validate() {
        validateMinMax(minWavelength.value, maxWavelength.value)
    }

    onVisibleChanged: {
        if (visible) {
            colVal.forceActiveFocus()
            validate()
        }
    }

    GridLayout {
        columns: 2

        Label {
            text: qsTr("Separator:")
        }
        MyComboBox {
            id: separator
            textRole: "text"
            valueRole: "value"
            model: [
                { text: qsTr("Comma"), value: "," },
                { text: qsTr("Semicolon"), value: ";" },
                { text: qsTr("Space"), value: " " },
                { text: qsTr("Tab"), value: "\t" },
            ]
        }

        Label {
            text: qsTr("Irradiance column:")
        }
        MyDoubleSpinBox {
            id: colVal
            from: 1
            to: 100
            decimals: 0
            wheelEnabled: true
        }

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

    onAccepted: {
        SpectrometerBridge.create(path + "?col=" + colVal.value + "&min=" + minWavelength.value + "&max=" + maxWavelength.value + "&separator=" + separator.currentValue)
        path = ""
    }
}
