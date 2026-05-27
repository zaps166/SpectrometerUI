// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick

Rectangle {
    color: "#66000000"

    Behavior on opacity {
        NumberAnimation {
            duration: 150
        }
    }
}
