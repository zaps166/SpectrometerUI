// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick
import QtQuick.Controls
import SpectrometerUI_files

ListView {
    ScrollBar.vertical: ScrollBar {
        interactive: !AppQmlSingleton.isMobile
        policy: ScrollBar.AlwaysOn
    }

    FlickableScrollBehavior {}

    focusPolicy: Qt.StrongFocus

    rightMargin: ScrollBar.vertical.width + (ScrollBar.vertical.interactive ? 2 : 0)
    reuseItems: true

    focus: true
}
