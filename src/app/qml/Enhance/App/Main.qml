// Fenetre principale (ENH-004 specs §3). Seul endroit ou le C++ est branche sur Theme.
// La fenetre est affichee par le C++ (WindowController) : masquee jusqu'a sa premiere image.
import QtQuick
import Enhance.Design

Window {
    id: window

    title: qsTr("Enhance")
    width: Theme.size.windowDefaultW
    height: Theme.size.windowDefaultH
    minimumWidth: Theme.size.windowMinW
    minimumHeight: Theme.size.windowMinH
    // Mica : fond transparent (DWM dessine le Mica derriere) ; sinon fond opaque. Jamais blanc.
    color: Theme.materialsEnabled ? "transparent" : Theme.color.bgWindow
    visible: false

    // ------------------------------------------------------------------ Branchement C++ -> Theme
    Binding { target: Theme; property: "dark"; value: SystemAppearance.dark }
    Binding { target: Theme; property: "highContrast"; value: SystemAppearance.highContrast }
    Binding { target: Theme; property: "reduceMotion"; value: SystemAppearance.reduceMotion }
    Binding { target: Theme; property: "transparencyEnabled"; value: SystemAppearance.transparency && !RenderPolicy.micaUnavailable }
    Binding { target: Theme; property: "glassQualityRequested"; value: RenderPolicy.glassQuality }
    Binding { target: Theme; property: "devicePixelRatio"; value: window.screen ? window.screen.devicePixelRatio : 1 }
    // Couleur de la barre de titre en mode opaque (aucune couture avec le contenu).
    Binding { target: RenderPolicy; property: "captionColor"; value: Theme.color.bgWindow }

    AppContent {
        id: contentRoot
        objectName: "appContent"
        anchors.fill: parent
        focus: true
        testbed: RenderPolicy.glassTestbed
    }

    // ------------------------------------------------------------------ Bascule de theme (§3.4)
    Image {
        id: themeSnapshot
        anchors.fill: parent
        visible: false
        cache: false
        smooth: true

        OpacityAnimator {
            id: snapshotFade
            target: themeSnapshot
            from: 1
            to: 0
            duration: Theme.dur.themeSwap
            easing.type: Easing.BezierSpline
            easing.bezierCurve: Theme.ease.standard
            onFinished: {
                themeSnapshot.visible = false;
                themeSnapshot.source = "";
            }
        }
    }

    // Windows a change d'apparence : capture de l'image courante, bascule (Theme + barre de
    // titre dans la meme iteration, cote C++), puis fondu de la capture.
    function swapTheme() {
        if (Theme.dur.themeSwap === 0 || !window.visible) {
            SystemAppearance.commitPending();
            return;
        }
        const dpr = Math.max(1, Theme.devicePixelRatio);
        const started = contentRoot.grabToImage(function (result) {
            // Capture tardive : la bascule a deja ete validee (delai de securite du C++). Montrer
            // l'ancienne image maintenant ferait un flash de l'ancien theme.
            if (!SystemAppearance.changePending)
                return;
            themeSnapshot.source = result.url;
            themeSnapshot.opacity = 1;
            themeSnapshot.visible = true;
            SystemAppearance.commitPending();
            snapshotFade.restart();
        }, Qt.size(Math.ceil(contentRoot.width * dpr), Math.ceil(contentRoot.height * dpr)));
        if (!started)
            SystemAppearance.commitPending();
    }

    Connections {
        target: SystemAppearance
        function onChangePendingChanged() {
            if (SystemAppearance.changePending)
                window.swapTheme();
        }
    }

    // Ctrl+Q : quitte immediatement (aucun traitement possible dans ENH-004, donc aucune feuille).
    Shortcut {
        sequences: ["Ctrl+Q"]
        context: Qt.ApplicationShortcut
        onActivated: AppStatus.requestQuit()
    }
}
