// Barre d'etat (ENH-004 specs §4.5) : filet d'un pixel physique, pastille + texte.
import QtQuick
import Enhance.Design

Item {
    id: root

    implicitHeight: Theme.size.statusBarH

    // Filet du haut : 1 pixel physique, aligne sur la grille physique (net a 125-175 %).
    Rectangle {
        readonly property real dpr: Math.max(1, Theme.devicePixelRatio)
        x: 0
        y: Math.round(root.y * dpr) / dpr - root.y
        width: root.width
        height: Theme.hairline
        color: Theme.color.borderHairline
    }

    Row {
        x: Theme.space.s4
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.space.s2

        StatusDot {
            anchors.verticalCenter: parent.verticalCenter
            accessibleName: AppStatus.gpuStatusAccessibleName
        }
        ThemedText {
            objectName: "statusText"
            anchors.verticalCenter: parent.verticalCenter
            textStyle: Theme.typo.caption
            color: Theme.color.textSecondary
            text: AppStatus.gpuStatusText
            Accessible.role: Accessible.StaticText
            Accessible.name: text
        }
    }
}
