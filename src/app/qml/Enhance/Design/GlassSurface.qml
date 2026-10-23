// Surface de verre « liquide » (design-system §6, ENH-004 specs §5).
// Qualite 2 : refraction + flou + teinte adaptative + lisere + reflet (shaders/glass.frag) ;
// qualite 1 : sans refraction ni reflet mobile ; qualite 0 (et contraste eleve) : opaque, sans
// shader ni lecture de la capture. Passage d'une qualite a l'autre en fondu croise.
import QtQuick
import QtQuick.Effects

Item {
    id: surface

    property QtObject level: Theme.glass.regular
    property real radius: height / 2
    property real bezel: level.bezel
    property Item backdrop: null      // ShaderEffectSource partagee de la fenetre
    property Item backdropItem: null  // Item capture par `backdrop` (repere des coordonnees)
    property point lightPos: Qt.point(0, 0)
    property bool pressed: false

    readonly property int quality: Theme.glassQuality
    readonly property bool glassy: quality !== Theme.glassOpaque && backdrop !== null
                                   && backdropItem !== null

    // Ombre du verre (design-system §6.2 regle 6 ; maquette --shadow-glass : 0 8 28 + 0 1 3).
    // Pas de jeton dedie dans Theme : valeurs composees a partir de l'echelle d'espacement.
    readonly property real shadowNearY: Theme.size.border
    readonly property real shadowNearBlur: Theme.space.s0_5 + Theme.size.border
    readonly property real shadowFarY: Theme.space.s2
    readonly property real shadowFarBlur: Theme.space.s6 + Theme.space.s1

    // ------------------------------------------------------------------ Ombres
    Item {
        anchors.fill: parent
        opacity: surface.glassy ? 1 : 0
        visible: opacity > 0 && !Theme.highContrast
        Behavior on opacity {
            NumberAnimation { duration: Theme.dur.fast; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.ease.standard }
        }

        RectangularShadow {
            anchors.fill: parent
            radius: surface.radius
            offset.y: surface.shadowFarY
            blur: surface.shadowFarBlur
            color: Theme.color.glassShadow
        }
        RectangularShadow {
            anchors.fill: parent
            radius: surface.radius
            offset.y: surface.shadowNearY
            blur: surface.shadowNearBlur
            color: Theme.color.glassShadow
        }
    }
    // Repli opaque : ombre e2 (aucune ombre en contraste eleve).
    RectangularShadow {
        anchors.fill: parent
        radius: surface.radius
        offset.y: Theme.elevation.e2.y
        blur: Theme.elevation.e2.blur
        color: Theme.elevation.e2.color
        opacity: surface.glassy ? 0 : 1
        visible: opacity > 0 && !Theme.highContrast
        Behavior on opacity {
            NumberAnimation { duration: Theme.dur.fast; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.ease.standard }
        }
    }

    // ------------------------------------------------------------------ Face opaque
    Rectangle {
        id: opaqueFace
        objectName: "glassOpaqueFace"
        anchors.fill: parent
        radius: surface.radius
        color: Theme.highContrast ? Theme.color.fillControl : Theme.color.surfaceRaised
        border.width: Theme.borderWidth
        border.color: Theme.color.borderControl
        opacity: surface.glassy ? 0 : 1
        visible: opacity > 0
        Behavior on opacity {
            NumberAnimation { duration: Theme.dur.fast; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.ease.standard }
        }
    }

    // ------------------------------------------------------------------ Verre
    ShaderEffect {
        id: glass
        objectName: "glassShader"
        anchors.fill: parent
        opacity: surface.glassy ? 1 : 0
        visible: opacity > 0
        Behavior on opacity {
            NumberAnimation { duration: Theme.dur.fast; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.ease.standard }
        }

        readonly property real dpr: Math.max(1, Theme.devicePixelRatio)
        readonly property real lightRad: Theme.glassGeo.lightAngleDeg * Math.PI / 180

        property variant source: surface.backdrop
        // Position de la surface dans la capture ; recalculee quand la geometrie change (pas
        // pendant le ressort : l'echelle s'applique au rendu, pas a la zone lue).
        // mapToItem() n'est pas « observable » par le moteur QML : les lignes `void (a + b)`
        // lisent les tailles uniquement pour les declarer comme dependances de la liaison (elle
        // est reevaluee quand l'une d'elles change), sans utiliser la valeur.
        property point origin: {
            void (surface.width + surface.height + glass.dpr);
            if (!surface.backdropItem)
                return Qt.point(0, 0);
            void (surface.backdropItem.width + surface.backdropItem.height);
            return surface.mapToItem(surface.backdropItem, 0, 0);
        }
        property size itemSize: Qt.size(width, height)
        property size backdropSize: surface.backdropItem
                                    ? Qt.size(Math.max(1, surface.backdropItem.width), Math.max(1, surface.backdropItem.height))
                                    : Qt.size(1, 1)
        property real radius: Math.min(surface.radius, height / 2)
        property real bezel: surface.bezel
        property real refraction: surface.quality === Theme.glassFull ? surface.level.refraction : 0
        // Flou par mipmaps : niveau ~ log2(rayon en texels de la capture).
        property real blurLod: Math.max(0, Math.log(surface.level.blur * Theme.glassGeo.backdropScale * dpr) / Math.LN2 - 1)
        property real blurStep: surface.level.blur / 4
        property real saturation: surface.level.saturation
        property real tintScale: surface.level.tintScale
        property vector4d tintCurve: Qt.vector4d(Theme.glassTintCurve.alphaA, Theme.glassTintCurve.alphaB,
                                                 Theme.glassTintCurve.lumaA, Theme.glassTintCurve.lumaB)
        property color tint: Theme.color.glassTint
        property color rim: Theme.color.glassRim
        property color rimLow: Theme.color.glassRimLow
        property color specularColor: Theme.color.glassSpecular
        property real specular: surface.level.specular * (surface.pressed ? Theme.glassGeo.pressSpecularBoost : 1)
        property point light: surface.lightPos
        property point lightDir: Qt.point(-Math.cos(lightRad), Math.sin(lightRad))

        fragmentShader: "shaders/glass.frag.qsb"
    }
}
