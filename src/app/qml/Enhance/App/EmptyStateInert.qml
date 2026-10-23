// Etat vide W1 sans action (ENH-004 specs §4) : cadre, illustration, titre, sous-titre.
// Aucune interaction : ni survol, ni zone de depot (Windows affiche le curseur « interdit »).
import QtQuick
import Enhance.Design

Item {
    id: root

    DashedFrame {
        anchors.fill: parent
    }

    Column {
        id: stack
        // Largeur max 440 (maquette, .drop-stack) : pas de jeton dedie, 440 = toastMaxW - s10.
        readonly property real maxWidth: Theme.size.toastMaxW - Theme.space.s10
        width: Math.min(maxWidth, root.width - 2 * Theme.space.s6)
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)

        EmptyIllustration {
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Item {
            height: Theme.space.s3 + Theme.space.s2 // 12 + 8 jusqu'au titre
        }
        ThemedText {
            objectName: "emptyTitle"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            textStyle: Theme.typo.title1
            color: Theme.color.textPrimary
            text: qsTr("Déposez vos vidéos ici")
            Accessible.role: Accessible.Heading
            Accessible.name: text
        }
        Item {
            height: Theme.space.s2
        }
        ThemedText {
            objectName: "emptySubtitle"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            textStyle: Theme.typo.body
            color: Theme.color.textSecondary
            text: qsTr("Enhance les passe en 4K fluide, sur votre ordinateur. Rien n'est envoyé sur Internet.")
            Accessible.role: Accessible.StaticText
            Accessible.name: text
        }
    }
}
