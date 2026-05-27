// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick
import QtQuick.Controls

SwipeView {
    implicitWidth: {
        let iw = 0
        for (const child of contentChildren) {
            iw = Math.max(iw, child.implicitWidth)
        }
        return iw
    }
    implicitHeight: {
        let ih = 0
        for (const child of contentChildren) {
            ih = Math.max(ih, child.implicitHeight)
        }
        return ih
    }

    clip: true

    Component.onCompleted: {
        contentItem.highlightMoveDuration = 0
    }

    function shouldBeVisible(item) {
        const flickable = item.SwipeView.view.contentItem as Flickable
        return item.SwipeView.isCurrentItem || ((item.SwipeView.isPreviousItem || item.SwipeView.isNextItem) && flickable.moving)
    }
}
