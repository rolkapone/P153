// Contenu de la fenetre (ENH-004 specs §3.3 et §4.1), de bas en haut :
// capture (teinte + etat vide + motif de test), capture partagee du verre, demonstration,
// barre d'etat. Grille verticale : 52 / cadre / 24 / 44 + 8 + 16 / 24 / 32.
import QtQuick
import Enhance.Design

Item {
    id: root

    property bool testbed: false
    readonly property alias springDemo: demo
    readonly property alias backdropLayer: backdropLayer

    // ------------------------------------------------------------------ Grille
    readonly property int sideMargin: width < Theme.size.breakpointNarrow ? Theme.space.s4 : Theme.space.s6
    readonly property real columnWidth: Math.min(Theme.size.contentMaxW + 2 * Theme.space.s6, width)
    readonly property real columnX: Math.round((width - columnWidth) / 2)
    readonly property real innerX: columnX + sideMargin
    readonly property real innerWidth: columnWidth - 2 * sideMargin
    readonly property real demoHeight: Theme.size.glassButtonH + Theme.space.s2 + Theme.typo.caption.lineHeight
    readonly property real demoY: height - Theme.size.statusBarH - Theme.space.s6 - demoHeight
    readonly property real frameY: Theme.size.toolbarH
    readonly property real frameHeight: demoY - Theme.space.s6 - frameY

    // ------------------------------------------------------------------ Capture
    Item {
        id: backdropLayer
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            color: Theme.materialsEnabled ? Theme.color.windowTint : Theme.color.bgWindow
        }

        EmptyStateInert {
            objectName: "emptyState"
            x: root.innerX
            y: root.frameY
            width: root.innerWidth
            height: Math.max(0, root.frameHeight)
        }

        Loader {
            // Masque en mode opaque (transparence coupee, contraste eleve, --glass-quality=0).
            active: root.testbed && Theme.glassQuality !== Theme.glassOpaque
            sourceComponent: GlassTestbed {}
            x: Math.round(demo.x + demo.capsule.x + (demo.capsule.width - width) / 2)
            y: Math.round(demo.y + (demo.capsule.height - height) / 2)
        }
    }

    // Une seule capture par fenetre, a demi-resolution, avec mipmaps (flou du verre). Elle n'est
    // re-rendue que si backdropLayer change : aucun rendu au repos.
    ShaderEffectSource {
        id: backdrop
        objectName: "glassBackdrop"
        width: 0
        height: 0
        sourceItem: backdropLayer
        hideSource: false
        live: true
        recursive: false
        mipmap: true
        smooth: true
        textureSize: Qt.size(Math.ceil(root.width * Theme.devicePixelRatio * Theme.glassGeo.backdropScale),
                             Math.ceil(root.height * Theme.devicePixelRatio * Theme.glassGeo.backdropScale))
    }

    // ------------------------------------------------------------------ Au-dessus du verre
    ElevationBanner {
        visible: AppStatus.elevated
        x: Math.round((root.width - width) / 2)
        y: Math.round((Theme.size.toolbarH - height) / 2)
    }

    SpringDemo {
        id: demo
        objectName: "springDemo"
        x: root.innerX
        y: root.demoY
        width: root.innerWidth
        backdrop: Theme.glassQuality !== Theme.glassOpaque ? backdrop : null
        backdropItem: backdropLayer
    }

    AppStatusBar {
        objectName: "statusBar"
        x: 0
        y: root.height - height
        width: root.width
    }
}
