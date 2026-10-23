// Pastille d'etat 8 px + halo (ENH-004 specs §4.5) ; contraste eleve : disque WindowText sans
// halo, toujours accompagne d'un texte.
import QtQuick

Item {
    id: dot

    property string accessibleName: ""

    implicitWidth: Theme.size.statusDot
    implicitHeight: Theme.size.statusDot
    Accessible.role: Accessible.Graphic
    Accessible.name: accessibleName

    // Halo de 3 px autour de la pastille (maquette : box-shadow 0 0 0 3px successTint).
    Rectangle {
        readonly property real diameter: Theme.size.statusDot + 2 * (Theme.space.s0_5 + Theme.borderWidth)
        visible: !Theme.highContrast
        width: diameter
        height: diameter
        radius: diameter / 2
        anchors.centerIn: parent
        color: Theme.color.successTint
    }
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.highContrast ? Theme.color.textPrimary : Theme.color.success
    }
}
