// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SpectrometerUI_files

MyDialog {
    id: deviceOpenDialog

    function validate() {
        validateMinMax(minWavelength.value, maxWavelength.value)
    }

    Component.onCompleted: {
        const params = SpectrometerBridge.getDeviceParams()
        const idx = device.find(params["dev"])
        if (idx >= 0) {
            device.currentIndex = idx
        }
        minWavelength.value = params["min"]
        maxWavelength.value = params["max"]
    }

    headerText: qsTr("Spectrometer device")

    onVisibleChanged: {
        if (visible) {
            minWavelength.forceActiveFocus()
            validate()
        }
    }

    ColumnLayout {
        RowLayout {
            visible: device.count > 0
            Label {
                Layout.fillWidth: true
                text: qsTr("Device:")
            }
            MyComboBox {
                id: device
                model: deviceOpenDialog.visible ? SpectrometerBridge.getDevices() : null
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
                    deviceOpenDialog.validate()
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
                    deviceOpenDialog.validate()
                }
            }
        }
    }

    onAccepted: {
        SpectrometerBridge.setDeviceParams(minWavelength.value, maxWavelength.value, device.currentText)
        SpectrometerBridge.createDevice(true)
    }
}
