// SEC-20 : bandeau discret si Enhance tourne avec un jeton administrateur (AppStatus.elevated).
// Place dans la reserve de la barre d'outils : rien d'autre ne bouge. Texte a valider par
// l'ux-designer (hors §5 de la note UX, cas exceptionnel).
import QtQuick
import Enhance.Design

Rectangle {
    id: root

    implicitWidth: label.implicitWidth + 2 * Theme.space.s4
    implicitHeight: Theme.size.badgeH + Theme.space.s2
    radius: height / 2
    color: Theme.color.warningTint
    border.width: Theme.highContrast ? Theme.borderWidth : 0
    border.color: Theme.color.warning
    Accessible.role: Accessible.AlertMessage
    Accessible.name: label.text

    ThemedText {
        id: label
        anchors.centerIn: parent
        textStyle: Theme.typo.caption
        color: Theme.color.textPrimary
        text: qsTr("Enhance est lancé en administrateur : ce n'est pas nécessaire. Relancez-le normalement.")
    }
}
