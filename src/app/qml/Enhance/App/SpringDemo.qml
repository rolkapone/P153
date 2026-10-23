// Demonstration technique (ENH-004 specs §6.5, PROVISOIRE : supprimee par ENH-016).
// Capsule « Tester le ressort » -> DemoCounter.increment() (C++) ; le compteur affiche le
// Q_PROPERTY DemoCounter.count. Aucune logique ici.
import QtQuick
import Enhance.Design

Item {
    id: root

    property Item backdrop: null
    property Item backdropItem: null
    readonly property alias capsule: capsule

    implicitHeight: Theme.size.glassButtonH + Theme.space.s2 + Theme.typo.caption.lineHeight

    GlassButton {
        id: capsule
        objectName: "springCapsule"
        x: Math.round((root.width - width) / 2)
        y: 0
        text: qsTr("Tester le ressort")
        backdrop: root.backdrop
        backdropItem: root.backdropItem
        onClicked: DemoCounter.increment()
    }

    // Ancre a droite de la capsule : il s'allonge vers la droite sans jamais la decaler.
    ThemedText {
        id: counter
        objectName: "springCounter"
        anchors.left: capsule.right
        anchors.leftMargin: Theme.space.s4
        anchors.verticalCenter: capsule.verticalCenter
        textStyle: Theme.typo.callout
        color: Theme.color.textSecondary
        font.features: { "tnum": 1 }
        text: qsTr("Essais : %1").arg(DemoCounter.count)
        Accessible.role: Accessible.StaticText
        Accessible.name: text
        transform: Translate { id: shift }

        Connections {
            target: DemoCounter
            function onCountChanged() {
                // Clics rapides : si le fondu est deja en cours, on ne relance que le glissement
                // (pas de retour a l opacite 0, donc aucun clignotement ; design-system §8.2).
                if (!fade.running)
                    fade.restart();
                rise.restart();
                // Region vive polie : annonce une fois par increment.
                counter.Accessible.announce(counter.text, Accessible.Polite);
            }
        }
        // Changement de valeur : fondu (fast) + glissement de 4 px (moveFast), deux animations
        // independantes pour pouvoir relancer le glissement seul.
        NumberAnimation {
            id: fade
            target: counter
            property: "opacity"
            from: 0
            to: 1
            duration: Theme.dur.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.ease.standard
        }
        NumberAnimation {
            id: rise
            target: shift
            property: "y"
            from: Theme.space.s1
            to: 0
            duration: Theme.dur.moveFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.ease.standard
        }
    }

    ThemedText {
        objectName: "springMention"
        anchors.top: capsule.bottom
        anchors.topMargin: Theme.space.s2
        anchors.horizontalCenter: capsule.horizontalCenter
        textStyle: Theme.typo.caption
        color: Theme.color.textSecondary
        text: qsTr("Démonstration technique")
        Accessible.role: Accessible.StaticText
        Accessible.name: text
    }
}
