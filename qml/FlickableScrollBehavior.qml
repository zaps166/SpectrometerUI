// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick

Connections {
    function onDragStarted() {
        // Touch behavior
        const flickable = target as Flickable
        if (flickable.boundsBehavior !== Flickable.DragOverBounds) {
            flickable.boundsBehavior = Flickable.DragOverBounds
        }
    }

    function onMovementStarted() {
        // Scrolling on mouse wheel
        const flickable = target as Flickable
        if (!flickable.dragging && flickable.boundsBehavior !== Flickable.StopAtBounds) {
            flickable.boundsBehavior = Flickable.StopAtBounds
        }
    }
}
