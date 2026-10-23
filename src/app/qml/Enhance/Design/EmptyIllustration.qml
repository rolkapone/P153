// Illustration de l'etat vide (ENH-004 specs §4.3), decorative : ignoree du Narrateur.
// Traces recopies de la maquette enh-004-squelette.html (repere 120 x 96).
import QtQuick
import QtQuick.Shapes

Item {
    id: art

    // Repere de dessin de la maquette (viewBox 0 0 120 96).
    readonly property size artboard: Qt.size(120, 96)
    readonly property real stroke: Theme.size.dropZoneBorder

    implicitWidth: artboard.width
    implicitHeight: artboard.height
    Accessible.ignored: true

    // Cadre arriere, tourne de -8 degres autour de son centre : rect(24, 14, 74, 50, r 10).
    Shape {
        x: 24
        y: 14
        width: 74
        height: 50
        rotation: -8
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            fillColor: Theme.color.fillControl
            strokeColor: Theme.color.borderStrong
            strokeWidth: art.stroke
            PathRectangle { width: 74; height: 50; radius: 10 }
        }
    }
    // Cadre avant : rect(16, 26, 84, 56, r 10).
    Shape {
        x: 16
        y: 26
        width: 84
        height: 56
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            fillColor: Theme.color.surfaceCard
            strokeColor: Theme.color.borderStrong
            strokeWidth: art.stroke
            PathRectangle { width: 84; height: 56; radius: 10 }
        }
    }
    // Triangle « lecture », pastille d'accent et fleche.
    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            fillColor: Theme.color.accentText
            strokeColor: "transparent"
            PathSvg { path: "M52 45.5v16a1.2 1.2 0 0 0 1.8 1l13-8a1.2 1.2 0 0 0 0-2l-13-8a1.2 1.2 0 0 0-1.8 1z" }
        }
        ShapePath {
            fillColor: Theme.color.accent
            strokeColor: "transparent"
            PathAngleArc { centerX: 96; centerY: 26; radiusX: 14; radiusY: 14; startAngle: 0; sweepAngle: 360 }
        }
        ShapePath {
            fillColor: "transparent"
            strokeColor: Theme.color.textOnAccent
            strokeWidth: 2.25
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: "M96 33V19.5M90.5 25L96 19.5l5.5 5.5" }
        }
    }
}
