// Bouton de verre autonome, capsule de 44 px (design-system §9.1, ENH-004 specs §6).
// Le verre et son voile gonflent au ressort ; le libelle et l'anneau de focus ne sont jamais
// mis a l'echelle (texte net a toutes les echelles).
import QtQuick
import QtQuick.Templates as T

T.AbstractButton {
    id: control

    property Item backdrop: null
    property Item backdropItem: null
    readonly property bool motion: !Theme.reduceMotion
    readonly property bool hc: Theme.highContrast
    // Reflet : suit le pointeur au survol (verre complet, animations actives), sinon en haut a
    // gauche.
    readonly property bool tracking: Theme.glassMotion && hover.hovered
    property real lightX: tracking ? hover.point.position.x : height
    property real lightY: tracking ? hover.point.position.y : 0
    readonly property Item glassItem: glassLayer

    implicitHeight: Theme.size.glassButtonH
    implicitWidth: Math.round(Math.max(Theme.size.primaryButtonMinW,
                                       implicitContentWidth + leftPadding + rightPadding))
    leftPadding: Theme.size.glassButtonPadX
    rightPadding: Theme.size.glassButtonPadX
    topPadding: 0
    bottomPadding: 0
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    Accessible.role: Accessible.Button
    Accessible.name: text

    Behavior on lightX {
        enabled: Theme.glassMotion
        NumberAnimation { duration: Theme.dur.moveFast; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.ease.decelerate }
    }
    Behavior on lightY {
        enabled: Theme.glassMotion
        NumberAnimation { duration: Theme.dur.moveFast; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.ease.decelerate }
    }

    HoverHandler {
        id: hover
        enabled: Theme.glassMotion
    }

    background: Item {
        implicitWidth: Theme.size.primaryButtonMinW
        implicitHeight: Theme.size.glassButtonH

        Item {
            id: glassLayer
            objectName: "glassLayer"
            anchors.fill: parent

            GlassSurface {
                anchors.fill: parent
                radius: height / 2
                level: Theme.glass.regular
                bezel: Theme.glassBezelFor(Theme.glass.regular, height, Theme.typo.headline.lineHeight)
                backdrop: control.backdrop
                backdropItem: control.backdropItem
                lightPos: Qt.point(control.lightX, control.lightY)
                pressed: control.pressed
            }
            // Voile d'etat ; contraste eleve : lisere Highlight au survol, fond Highlight a la
            // pression.
            Rectangle {
                anchors.fill: parent
                radius: height / 2
                color: control.hc ? (control.pressed ? Theme.color.accent : "transparent")
                                  : (control.pressed ? Theme.color.fillControlPressed
                                                     : (control.hovered ? Theme.color.fillControlHover : "transparent"))
                border.width: control.hc && (control.hovered || control.pressed) ? Theme.borderWidth : 0
                border.color: Theme.color.accent
                Behavior on color {
                    ColorAnimation {
                        duration: control.pressed ? Theme.dur.press : Theme.dur.fast
                        easing.type: Easing.BezierSpline
                        easing.bezierCurve: Theme.ease.standard
                    }
                }
            }
        }

        FocusRing {
            objectName: "focusRing"
            target: control
        }
    }

    contentItem: ThemedText {
        text: control.text
        textStyle: Theme.typo.headline
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: control.hc && control.pressed ? Theme.color.textOnAccent : Theme.color.textPrimary
        Accessible.ignored: true
    }

    // ------------------------------------------------------------------ Ressort
    // Animators : executes sur le thread de rendu, fluides meme si l'interface est occupee.
    // Mouvement reduit : l'echelle n'est jamais modifiee (aucune animation lancee).
    ScaleAnimator {
        id: pressIn
        target: glassLayer
        to: Theme.glassGeo.pressScale
        duration: Theme.dur.movePress
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.ease.decelerate
    }
    ScaleAnimator {
        id: pressOut
        target: glassLayer
        to: 1 // echelle neutre
        duration: Theme.dur.moveMedium
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.ease.spring
    }
    SequentialAnimation {
        id: pulse
        ScaleAnimator {
            target: glassLayer
            to: Theme.glassGeo.pressScale
            duration: Theme.dur.movePress
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.ease.decelerate
        }
        ScaleAnimator {
            target: glassLayer
            to: 1 // echelle neutre
            duration: Theme.dur.moveMedium
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.ease.spring
        }
    }

    onPressedChanged: {
        if (!motion)
            return;
        pulse.stop();
        if (pressed) {
            pressOut.stop();
            pressIn.restart();
        } else {
            pressIn.stop();
            pressOut.restart();
        }
    }

    // Entree : impulsion complete + action (Espace est gere par AbstractButton).
    function activateByKey() {
        if (motion) {
            pressIn.stop();
            pressOut.stop();
            pulse.restart();
        }
        control.clicked();
    }
    Keys.onReturnPressed: activateByKey()
    Keys.onEnterPressed: activateByKey()
}
