// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SpectrometerUI_files

Pane {
    id: root

    topPadding: 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: bar

            Layout.fillWidth: true

            // Prevent animation at start
            property real initialHighlightMoveDuration
            Component.onCompleted: {
                const contentListView = contentItem as ListView
                initialHighlightMoveDuration = contentListView.highlightMoveDuration
                contentListView.highlightMoveDuration = 0
            }
            onCurrentIndexChanged: {
                const contentListView = contentItem as ListView
                if (contentListView.highlightMoveDuration !== initialHighlightMoveDuration) {
                    contentListView.highlightMoveDuration = initialHighlightMoveDuration
                }
            }

            currentIndex: view.currentIndex
            wheelEnabled: true

            TabButton {
                text: qsTr("Like")
            }

            TabButton {
                text: qsTr("All")
            }

            TabButton {
                text: qsTr("Base")
            }

            TabButton {
                text: qsTr("PPFD")
            }

            TabButton {
                text: qsTr("BL")
            }
        }

        GroupBox {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            Layout.minimumHeight: 50

            topPadding: 3
            bottomPadding: 3
            leftPadding: 3
            rightPadding: 3

            MySwipeView {
                id: view

                anchors.fill: parent

                currentIndex: bar.currentIndex

                LikedMeasurementsView {
                    visible: view.shouldBeVisible(this)
                }

                MeasurementsView {
                    visible: view.shouldBeVisible(this)
                    model: SpectrometerBridge
                }

                MeasurementsView {
                    visible: view.shouldBeVisible(this)
                    model: SortFilterProxyModel {
                        model: SpectrometerBridge
                        filters: ValueFilter {
                            roleName: "base"
                            value: true
                        }
                    }
                }

                MeasurementsView {
                    visible: view.shouldBeVisible(this)
                    model: SortFilterProxyModel {
                        model: SpectrometerBridge
                        filters: ValueFilter {
                            roleName: "ppfd"
                            value: true
                        }
                    }
                }

                MeasurementsView {
                    visible: view.shouldBeVisible(this)
                    model: SortFilterProxyModel {
                        model: SpectrometerBridge
                        filters: ValueFilter {
                            roleName: "bl"
                            value: true
                        }
                    }
                }
            }
        }
    }
}
