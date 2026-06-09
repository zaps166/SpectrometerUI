// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import SpectrometerUI_files

Menu {
    modal: true

    Overlay.modal: OverlayItem {}

    onVisibleChanged: {
        topMargin = SafeArea.margins.top
        leftMargin = SafeArea.margins.left
        bottomMargin = SafeArea.margins.bottom
        rightMargin = SafeArea.margins.right
    }
}
