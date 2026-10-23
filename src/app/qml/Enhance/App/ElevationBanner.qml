// SEC-20 : bandeau discret si Enhance tourne avec un jeton administrateur (AppStatus.elevated).
// Place dans la reserve de la barre d'outils : rien d'autre ne bouge. Texte valide par
// l'ux-designer (2026-10-23), une ligne a 760 px et 100 %.
import QtQuick
import Enhance.Design

Rectangle {
    id: root

    // Contour warning a 30 % en mode normal (controle design ENH-004) ; plein en contraste eleve.
    readonly property real outlineAlpha: 0.3

    implicitWidth: label.implicitWidth + 2 * Theme.space.s4
    implicitHeight: Theme.size.badgeH + Theme.space.s2
    radius: height / 2
    color: Theme.color.warningTint
    border.width: Theme.borderWidth
    border.color: Theme.highContrast ? Theme.color.warning
                                     : Theme.withAlpha(Theme.color.warning, outlineAlpha)
    Accessible.role: Accessible.AlertMessage
    Accessible.name: label.text

    ThemedText {
        id: label
        anchors.centerIn: parent
        textStyle: Theme.typo.caption
        color: Theme.color.textPrimary
        text: qsTr("Enhance est lancé en tant qu'administrateur. Ce n'est pas nécessaire : relancez-le normalement.")
    }
}
