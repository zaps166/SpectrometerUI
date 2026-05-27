// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma Singleton

import QtQuick

QtObject {
    readonly property bool isMobile: Qt.platform.os === "android" || Qt.platform.os === "ios"

    property bool spectrumPointingWavelength: false

    property real measurementsViewFirstColWidth

    function validateMinMax(min: real, max: real): bool {
        if (min > 0 && max > 0) {
            const diff = max - min + 1
            return diff >= 2
        }
        return true
    }
}
