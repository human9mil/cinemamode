import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import CinemaMode

Kirigami.ApplicationWindow {
    id: root

    title: qsTr("Cinema Mode")
    width: Kirigami.Units.gridUnit * 22
    height: Kirigami.Units.gridUnit * 30
    minimumWidth: Kirigami.Units.gridUnit * 18
    minimumHeight: Kirigami.Units.gridUnit * 24

    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None

    // Cinema palette — deliberately overrides the system color scheme so the
    // app reads as "movie theatre" regardless of the desktop's light/dark setting.
    readonly property color screenBlack: "#0a0a0d"
    readonly property color velvetRed: "#a6192e"
    readonly property color marqueeGold: "#d8b86a"
    readonly property color reelMetal: "#26262d"
    readonly property color reelMetalLight: "#3a3a44"

    // A repeating strip of film-reel sprocket holes, used as a top/bottom border.
    component FilmStrip: Item {
        id: strip
        implicitHeight: Kirigami.Units.gridUnit * 0.9
        clip: true

        Rectangle { anchors.fill: parent; color: root.screenBlack }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: strip.height * 0.55
            Repeater {
                model: Math.ceil(strip.width / (strip.height * 1.55)) + 2
                Rectangle {
                    width: strip.height * 0.5
                    height: width
                    radius: width * 0.2
                    color: root.reelMetal
                    border.width: 1
                    border.color: Qt.darker(root.marqueeGold, 1.4)
                    opacity: 0.85
                }
            }
        }
    }

    pageStack.initialPage: Kirigami.Page {
        title: qsTr("Cinema Mode")

        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        Kirigami.Theme.backgroundColor: root.screenBlack
        Kirigami.Theme.textColor: root.marqueeGold
        Kirigami.Theme.highlightColor: root.velvetRed
        Kirigami.Theme.highlightedTextColor: root.marqueeGold
        Kirigami.Theme.disabledTextColor: Qt.darker(root.marqueeGold, 1.8)

        padding: 0

        FilmStrip { id: topStrip; anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right }
        FilmStrip { id: bottomStrip; anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right }

        ColumnLayout {
            anchors.top: topStrip.bottom
            anchors.bottom: bottomStrip.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Kirigami.Units.largeSpacing * 2
            spacing: Kirigami.Units.largeSpacing * 1.5

            Item { Layout.fillHeight: true; Layout.minimumHeight: 0 }

            // Marquee title
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: Kirigami.Units.smallSpacing

                Controls.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("CINEMA MODE")
                    color: root.marqueeGold
                    font.pixelSize: Kirigami.Units.gridUnit * 1.3
                    font.bold: true
                    font.letterSpacing: 4
                }

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: Kirigami.Units.gridUnit * 6
                    height: 2
                    color: root.velvetRed
                }
            }

            // Film-reel toggle button
            Item {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.largeSpacing
                implicitWidth: Kirigami.Units.gridUnit * 11
                implicitHeight: implicitWidth

                // Projector-light glow behind the reel when cinema mode is active
                Rectangle {
                    id: glowRing
                    anchors.centerIn: parent
                    width: parent.width * 1.18
                    height: width
                    radius: width / 2
                    color: "transparent"
                    border.width: Kirigami.Units.gridUnit * 0.6
                    border.color: root.velvetRed

                    property real pulseOpacity: 0.3
                    opacity: MonitorController.cinemaActive ? pulseOpacity : 0
                    Behavior on opacity { NumberAnimation { duration: Kirigami.Units.longDuration } }

                    SequentialAnimation on pulseOpacity {
                        running: MonitorController.cinemaActive
                        loops: Animation.Infinite
                        NumberAnimation { from: 0.15; to: 0.45; duration: 1100; easing.type: Easing.InOutSine }
                        NumberAnimation { from: 0.45; to: 0.15; duration: 1100; easing.type: Easing.InOutSine }
                    }
                }

                Controls.AbstractButton {
                    id: toggleButton
                    anchors.fill: parent

                    onClicked: {
                        reelSpin.angleTo += 130
                        MonitorController.toggleCinema()
                    }

                    background: Item {
                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.0; color: root.reelMetalLight }
                                GradientStop { position: 1.0; color: root.reelMetal }
                            }
                            border.width: 3
                            border.color: MonitorController.cinemaActive ? root.velvetRed : root.marqueeGold
                            Behavior on border.color { ColorAnimation { duration: Kirigami.Units.longDuration } }
                        }

                        // Reel spokes + sprocket holes
                        Item {
                            id: reelSpin
                            property real angleTo: 0
                            anchors.fill: parent
                            rotation: angleTo
                            Behavior on rotation { NumberAnimation { duration: 650; easing.type: Easing.OutCubic } }

                            Repeater {
                                model: 6
                                Rectangle {
                                    property real angle: index * (Math.PI * 2 / 6)
                                    width: reelSpin.width * 0.22
                                    height: width
                                    radius: width / 2
                                    color: root.screenBlack
                                    border.width: 2
                                    border.color: Qt.darker(root.marqueeGold, 1.3)
                                    x: reelSpin.width / 2 + Math.cos(angle) * reelSpin.width * 0.29 - width / 2
                                    y: reelSpin.height / 2 + Math.sin(angle) * reelSpin.height * 0.29 - height / 2
                                }
                            }

                            Rectangle {
                                anchors.centerIn: parent
                                width: reelSpin.width * 0.26
                                height: width
                                radius: width / 2
                                color: MonitorController.cinemaActive ? root.velvetRed : root.marqueeGold
                                Behavior on color { ColorAnimation { duration: Kirigami.Units.longDuration } }
                            }
                        }

                        scale: toggleButton.pressed ? 0.95 : 1.0
                        Behavior on scale { NumberAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.OutQuad } }
                    }
                }
            }

            Controls.Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Kirigami.Units.smallSpacing
                text: MonitorController.cinemaActive
                    ? qsTr("NOW SHOWING")
                    : qsTr("STANDING BY")
                color: MonitorController.cinemaActive ? root.velvetRed : root.marqueeGold
                font.pixelSize: Kirigami.Units.gridUnit * 1.05
                font.bold: true
                font.letterSpacing: 2
            }

            Controls.Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Click the reel to blank every monitor except your main one")
                color: root.marqueeGold
                opacity: 0.6
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                Layout.maximumWidth: parent.width
            }

            Item { Layout.preferredHeight: Kirigami.Units.largeSpacing }

            Kirigami.FormLayout {
                Layout.fillWidth: true

                Controls.ComboBox {
                    Kirigami.FormData.label: qsTr("Keep this monitor on:")
                    model: MonitorController.outputs.map(o => o.name)
                    currentIndex: model.indexOf(MonitorController.mainOutput)
                    onActivated: MonitorController.mainOutput = currentText
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing / 2

                Repeater {
                    model: MonitorController.outputs

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: rowContent.implicitHeight + Kirigami.Units.smallSpacing
                        required property var modelData
                        color: Qt.rgba(root.marqueeGold.r, root.marqueeGold.g, root.marqueeGold.b, 0.06)
                        radius: 4
                        border.width: 1
                        border.color: Qt.rgba(root.marqueeGold.r, root.marqueeGold.g, root.marqueeGold.b, 0.2)

                        RowLayout {
                            id: rowContent
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.margins: Kirigami.Units.smallSpacing

                            Rectangle {
                                width: Kirigami.Units.iconSizes.small * 0.5
                                height: width
                                radius: width / 2
                                color: modelData.enabled ? "#4caf50" : root.velvetRed
                            }

                            Controls.Label {
                                text: modelData.name + (modelData.name === MonitorController.mainOutput ? qsTr(" · main") : "")
                                color: root.marqueeGold
                            }

                            Item { Layout.fillWidth: true }

                            Controls.Label {
                                text: modelData.enabled ? qsTr("on") : qsTr("off")
                                color: root.marqueeGold
                                opacity: 0.6
                            }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true; Layout.minimumHeight: 0 }
        }
    }
}
