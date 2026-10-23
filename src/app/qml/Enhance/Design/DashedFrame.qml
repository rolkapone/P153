// Cadre arrondi de l'etat vide (ENH-004 specs §4.2) : pointilles 6 / 6 de 1,5 px ;
// contraste eleve : trait plein de 2 px (WindowText). Purement decoratif.
import QtQuick
import QtQuick.Shapes

Shape {
    id: frame

    readonly property bool solid: Theme.highContrast
    readonly property real strokeWidth: solid ? Theme.borderWidth : Theme.size.dropZoneBorder
    // Trace inscrit : le trait reste entierement dans le cadre.
    readonly property real inset: strokeWidth / 2

    preferredRendererType: Shape.CurveRenderer
    Accessible.ignored: true

    ShapePath {
        strokeColor: Theme.color.borderStrong
        strokeWidth: frame.strokeWidth
        fillColor: "transparent"
        strokeStyle: frame.solid ? ShapePath.SolidLine : ShapePath.DashLine
        // Unites = largeur de trait : 4 x 1,5 = 6 px de trait, 6 px d'espace (maquette).
        dashPattern: [4, 4]
        capStyle: ShapePath.FlatCap
        joinStyle: ShapePath.RoundJoin

        PathRectangle {
            x: frame.inset
            y: frame.inset
            width: Math.max(0, frame.width - 2 * frame.inset)
            height: Math.max(0, frame.height - 2 * frame.inset)
            // Coins concentriques au bord du cadre (rayon xl) ; HC : 15 = 16 - 1.
            radius: frame.solid ? Theme.radius.xl - frame.inset : Theme.radius.xl
        }
    }
}
