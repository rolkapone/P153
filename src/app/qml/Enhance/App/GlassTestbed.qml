// banc d'essai, hors design system (ENH-004 specs §6.6).
// Motif de test place sous la capsule avec --glass-testbed, pour rendre la refraction visible
// (QA, controle design). Seules couleurs et tailles « en dur » tolerees dans l'application.
import QtQuick
import QtQuick.Effects

Item {
    id: root

    implicitWidth: 196
    implicitHeight: 76
    Accessible.ignored: true

    Item {
        id: pattern
        anchors.fill: parent
        visible: false
        layer.enabled: true

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#F2B880" }
                GradientStop { position: 0.25; color: "#D9776A" }
                GradientStop { position: 0.5; color: "#7C83FF" }
                GradientStop { position: 0.75; color: "#3DD68C" }
                GradientStop { position: 1.0; color: "#9AD7F5" }
            }
        }
        // Rayures diagonales blanches a 55 % : 6 px pleins tous les 18 px, inclinees a 115 deg.
        Item {
            anchors.centerIn: parent
            width: Math.hypot(root.width, root.height)
            height: width
            rotation: 25
            Repeater {
                model: Math.ceil(parent.width / 18) + 1
                Rectangle {
                    required property int index
                    x: index * 18
                    width: 6
                    height: parent.height
                    color: Qt.rgba(1, 1, 1, 0.55)
                }
            }
        }
    }

    // Masque arrondi (rayon 16) : les rayures ne debordent pas des coins.
    Rectangle {
        id: mask
        anchors.fill: parent
        radius: 16
        visible: false
        layer.enabled: true
    }

    MultiEffect {
        anchors.fill: parent
        source: pattern
        maskEnabled: true
        maskSource: mask
    }
}
