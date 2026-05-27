// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import QtQuick.Effects
import SpectrometerUI_files

Pane {
    id: root

    implicitWidth: 100
    implicitHeight: 100

    DuvGraphPriv {
        id: priv
    }

    Item {
        id: graphArea

        x: parent.width  / 2 - shape.boundingRect.width  / 2 - shape.boundingRect.x
        y: parent.height / 2 - shape.boundingRect.height / 2 - shape.boundingRect.y
        width: Math.min(parent.width, parent.height)
        height: width

        ShaderEffect {
            id: colorFill

            readonly property bool useP3: SpectrometerBridge.isP3

            anchors.fill: parent

            vertexShader: "qrc:/qml/Default.vert.qsb"
            fragmentShader: "qrc:/qml/ChromaticityDiagramFill.frag.qsb"
            visible: false
        }

        MultiEffect {
            id: blurredColorFill
            anchors.fill: parent
            autoPaddingEnabled: false
            source: colorFill
            visible: false
            blurEnabled: true
            blurMax: 48
            blur: 1.0
        }

        Shape {
            id: shape
            anchors.fill: parent

            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                strokeColor: "#000000"
                strokeWidth: 2

                fillItem: ShaderEffectSource {
                    format: ShaderEffectSource.RGBA16F
                    sourceItem: blurredColorFill
                }

                scale: Qt.size(graphArea.width, graphArea.height)
                pathHints: ShapePath.PathLinear | ShapePath.PathNonIntersecting | ShapePath.PathSolid

                PathPolyline {
                    Component.onCompleted: {
                        priv.setSpectralLocus(this)
                    }
                }
            }

            ShapePath {
                strokeColor: "#000000"
                strokeWidth: 2

                fillColor: "transparent"

                scale: Qt.size(graphArea.width, graphArea.height)
                pathHints: ShapePath.PathLinear | ShapePath.PathNonIntersecting

                PathPolyline {
                    Component.onCompleted: {
                        priv.setPlanckianLocus(this)
                    }
                }
            }
        }

        Shape {
            anchors.fill: parent

            visible: priv.valid

            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                strokeColor: "#ffffff"
                strokeWidth: 1

                fillColor: "transparent"

                pathHints: ShapePath.PathLinear | ShapePath.PathNonIntersecting

                PathRectangle {
                    x: priv.currentPoint.x * graphArea.width - width / 2
                    y: priv.currentPoint.y * graphArea.height - height / 2
                    width: 5
                    height: width
                    radius: width / 2
                }
            }
        }
    }

    Label {
        anchors {
            top: parent.top
            right: parent.right
            topMargin: -root.topPadding + 4
            rightMargin: -root.rightPadding + 4
        }
        text: "CIE1931 Duv"
        font.bold: true
    }
}
