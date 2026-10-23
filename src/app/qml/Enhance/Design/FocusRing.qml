// Anneau de focus clavier (ENH-004 specs §6.4) : 2 px, a 2 px de la cible, forme de capsule.
// Visible seulement quand le focus vient du clavier (visualFocus) ; jamais mis a l'echelle.
// A placer dans un Item qui couvre exactement la cible (son parent).
import QtQuick

Rectangle {
    id: ring

    property Item target: null
    readonly property bool shown: target !== null && target.visualFocus
    // Entree : l'anneau glisse de 4 a 2 px de la cible.
    property real gap: shown ? Theme.size.focusRingOffset : Theme.size.focusRingOffset * 2

    anchors.fill: parent
    anchors.margins: -(gap + Theme.size.focusRingW)
    radius: height / 2
    color: "transparent"
    border.width: Theme.size.focusRingW
    border.color: Theme.color.focusRing
    opacity: shown ? 1 : 0
    visible: opacity > 0
    Accessible.ignored: true

    Behavior on opacity {
        NumberAnimation {
            duration: Theme.dur.fast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.ease.decelerate
        }
    }
    Behavior on gap {
        NumberAnimation {
            duration: Theme.dur.moveFast
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.ease.decelerate
        }
    }
}
