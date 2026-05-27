// SPDX-FileCopyrightText: 2026 Błażej Szczygieł <mumei6102@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Shapes
import SpectrometerUI_files

Item {
    id: root

    readonly property color lineColor: Material.foreground
    readonly property real lineSmallSize: 8

    readonly property bool hasData: peakIrradiance > 0

    property alias drawOutline: priv.drawOutline

    property real peakIrradiance: 0
    property bool pointingNm: false
    property real pointedNm: 0

    implicitHeight: 200

    function setPointedNm(point) {
        pointedNm = priv.minNm + point.position.x / shape.width * priv.spanNm
        label.updateText()
    }

    SpectrumPriv {
        id: priv

        readonly property real spanNm: priv.maxNm - priv.minNm

        onDataUpdated: {
            root.peakIrradiance = priv.getPeakIrradiance()
            label.updateText()
        }
    }

    Label {
        id: label

        anchors {
            top: parent.top
            left: parent.left
            margins: 4
        }

        visible: root.pointingNm

        function updateText() {
            const val = priv.getIrradianceAt(root.pointedNm)
            label.text = val.wavelength + " nm\n" + val.irradiance + " W/m²/nm"
            pointerLine.graphOffset = Math.max(0, val.wavelength - priv.minNm)
        }
    }

    ShaderEffect {
        id: lightSpectrumFill

        // Add/subtract one more because "ClampToEdge" will spread the last (black) value
        readonly property real minNm: 360 - 1
        readonly property real maxNm: 830 + 1

        readonly property bool useP3: SpectrometerBridge.isP3

        vertexShader: "qrc:/qml/Default.vert.qsb"
        fragmentShader: "qrc:/qml/LightSpectrumFill.frag.qsb"
        visible: false

        width: 1
        height: 1
    }
    ShaderEffectSource {
        id: lightSpectrumFillScaled

        format: ShaderEffectSource.RGBA16F
        sourceItem: lightSpectrumFill
        wrapMode: ShaderEffectSource.ClampToEdge
        visible: false

        textureSize: Qt.size(lightSpectrumFill.maxNm - lightSpectrumFill.minNm + 1, 1)
    }

    Shape {
        id: shape

        readonly property real dpr: Window.window.devicePixelRatio

        anchors {
            fill: parent
            leftMargin: 70
            topMargin: 50
            rightMargin: 30
            bottomMargin: 30
        }

        preferredRendererType: Shape.CurveRenderer
        asynchronous: true

        ShapePath {
            id: graphFilled

            strokeColor: "transparent"

            fillItem: lightSpectrumFillScaled
            fillTransform: PlanarTransform.fromAffineMatrix(
                scale.width * shape.dpr,
                0,
                0,
                1,
                (lightSpectrumFill.minNm - priv.minNm - 0.5) * scale.width,
                0
            )

            scale: Qt.size(Math.max(1, shape.width) / Math.max(1, priv.spanNm), Math.max(1, shape.height))
            pathHints: ShapePath.PathLinear | ShapePath.PathNonIntersecting | ShapePath.PathSolid

            PathPolyline {
                id: graphPolyline
            }
        }

        ShapePath {
            strokeColor: root.lineColor
            strokeWidth: 1 / Screen.devicePixelRatio

            fillColor: "transparent"

            scale: graphFilled.scale
            pathHints: graphFilled.pathHints

            PathPolyline {
                id: graphOutlinePolyline
            }
        }

        ShapePath {
            id: pointerLine

            property real graphOffset: 0

            strokeColor: root.pointingNm ? "gray" : "transparent"
            scale: Qt.size(shape.width / Math.max(1, priv.spanNm), shape.height)

            startX: graphOffset
            startY: 0

            PathLine {
                x: pointerLine.graphOffset
                y: 1
            }
        }

        HoverHandler {
            id: hoverHandler

            enabled: !AppQmlSingleton.isMobile
            cursorShape: Qt.CrossCursor

            onHoveredChanged: {
                root.pointingNm = hovered
            }

            onPointChanged: {
                root.setPointedNm(point)
            }
        }

        TapHandler {
            id: tapHandler

            enabled: AppQmlSingleton.isMobile
            // acceptedDevices: PointerDevice.TouchScreen
            gesturePolicy: TapHandler.ReleaseWithinBounds

            onLongPressed: {
                AppQmlSingleton.spectrumPointingWavelength = true
                root.pointingNm = true
                root.setPointedNm(point)
            }

            onPressedChanged: {
                if (!pressed) {
                    AppQmlSingleton.spectrumPointingWavelength = false
                }
            }

            onPointChanged: {
                if (AppQmlSingleton.spectrumPointingWavelength) {
                    root.setPointedNm(point)
                }
            }

            onCanceled: {
                root.pointingNm = false
            }
        }

        Component.onCompleted: {
            priv.setPathObj(graphPolyline, graphOutlinePolyline)
        }
    }

    // Horizontal axis
    Rectangle {
        id: horizLine

        anchors {
            top: shape.bottom
            left: shape.left
            right: shape.right
        }

        height: 1
        color: root.lineColor
    }
    Repeater {
        id: horizLabels

        model: 12

        Label {
            id: horizLabel

            required property int index

            anchors {
                top: horizLine.bottom
                topMargin: root.lineSmallSize
            }

            visible: index === 0 || text !== "0"

            x: horizLine.x - width / 2 + (index * horizLine.width / (horizLabels.count - 1))

            text: Math.round(priv.minNm + priv.spanNm * (index / (horizLabels.count - 1)))

            font.pixelSize: 11

            Rectangle {
                anchors {
                    top: parent.top
                    left: parent.left
                    topMargin: -root.lineSmallSize
                    leftMargin: parent.width / 2 - (horizLabel.index === 0 || horizLabel.index === horizLabels.count - 1 ? 1 : 0.5)
                }

                width: 1
                height: root.lineSmallSize
                color: root.lineColor
            }
        }
    }

    // Vertical axis
    Rectangle {
        id: vertLine

        anchors {
            right: shape.left
            top: shape.top
            bottom: shape.bottom
        }

        width: 1
        color: root.lineColor
    }
    Repeater {
        id: vertLabels

        model: 8

        Label {
            id: vertLabel

            required property int index

            anchors {
                right: vertLine.left
                rightMargin: root.lineSmallSize
            }

            visible: index === vertLabels.count - 1 || text !== "0"

            y: vertLine.y - height / 2 + (index * vertLine.height / (vertLabels.count - 1))

            text: {
                const val = (root.peakIrradiance * (vertLabels.count - vertLabel.index - 1) / (vertLabels.count - 1));
                return (val === 0) ? 0 : val.toPrecision(2)
            }

            font.pixelSize: 11

            Rectangle {
                anchors {
                    top: parent.top
                    left: parent.right
                    topMargin: parent.height / 2
                }

                width: root.lineSmallSize + 1
                height: 1
                color: root.lineColor
            }
        }
    }

    // Title label
    Label {
        anchors {
            top: parent.top
            right: parent.right
            margins: 4
        }
        text: "Spectrum"
        font.bold: true
    }
}
