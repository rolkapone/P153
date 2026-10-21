// Theme.qml — Enhance design tokens pour Qt Quick (Qt 6.8+).
// Traduction exacte de docs/design/ui/tokens.json (source unique) ; mêmes valeurs que tokens.css.
// Aucune logique métier : uniquement des valeurs visuelles et de petites fonctions de présentation.
//
// Branchement (fait par la présentation C++ / Main.qml, pas par les composants) :
//   Theme.dark             <- SystemAppearance.dark       (réglage Apparence : Système / Sombre / Clair)
//   Theme.highContrast     <- SystemAppearance.highContrast (SPI_GETHIGHCONTRAST)
//   Theme.reduceMotion     <- SystemAppearance.reduceMotion (SPI_GETCLIENTAREAANIMATION == FALSE)
//   Theme.devicePixelRatio <- Screen.devicePixelRatio de la fenêtre principale
//   Theme.transparencyEnabled   <- SystemAppearance.transparency (« Effets de transparence » Windows)
//   Theme.glassQualityRequested <- RenderPolicy.glassQuality (budget GPU mesuré, voir design-system.md §6)
//
// Exemples d'usage :
//   color: Theme.color.textSecondary
//   font: Theme.typo.callout.font ; lineHeight: Theme.typo.callout.lineHeight ; lineHeightMode: Text.FixedHeight
//   anchors.margins: Theme.space.s4 ; radius: Theme.radius.lg
//   Behavior on opacity { NumberAnimation { duration: Theme.dur.fast
//                         easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.ease.standard } }
//   Behavior on y       { NumberAnimation { duration: Theme.dur.moveMedium        // 0 ms si mouvement réduit
//                         easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.ease.spring } }
//
// Chiffres tabulaires (%, i/s, durées) : sur le Text concerné, font.features: { "tnum": 1 } (Qt >= 6.6).

pragma Singleton
import QtQuick

QtObject {
    id: theme

    // ------------------------------------------------------------------ Entrées
    property bool dark: true
    property bool highContrast: false
    property bool reduceMotion: false
    property real devicePixelRatio: 1.0
    property bool transparencyEnabled: true   // <- SystemAppearance.transparency (HKCU\...\Personalize\EnableTransparency)
    property int glassQualityRequested: 2     // <- RenderPolicy.glassQuality (2 complète, 1 allégée, 0 opaque)

    // 1 pixel physique, pour les séparateurs nets à 125 / 150 / 175 %.
    readonly property real hairline: 1 / Math.max(1, devicePixelRatio)
    // Bordures des contrôles et cartes : 2 px en contraste élevé.
    readonly property int borderWidth: highContrast ? 2 : 1

    function rgba(r, g, b, a) { return Qt.rgba(r / 255, g / 255, b / 255, a) }
    function withAlpha(c, a) { return Qt.rgba(c.r, c.g, c.b, a) }

    // ------------------------------------------------------------------ Couleurs
    // Nommé ColorPalette (et non Palette) pour ne pas masquer le type QtQuick « Palette ».
    component ColorPalette: QtObject {
        property color bgWindow
        property color windowTint
        property color surfaceCard
        property color surfaceCardHover
        property color surfaceCardSelected
        property color surfaceRaised
        property color surfaceSheet
        property color surfaceSheetOpaque
        property color surfaceTooltip
        property color thumbPlaceholder
        property color borderHairline
        property color borderCard
        property color borderControl
        property color borderStrong
        property color fillControl
        property color fillControlHover
        property color fillControlPressed
        property color segmentThumb
        property color segmentSelectedText
        property color scrim
        property color textPrimary
        property color textSecondary
        property color textTertiary
        property color textDisabled
        property color textOnAccent
        property color accent
        property color accentHover
        property color accentPressed
        property color accentText
        property color accentTint
        property color progress
        property color focusRing
        property color success
        property color successTint
        property color onSuccess
        property color warning
        property color warningTint
        property color danger
        property color dangerTint
        property color skeleton
        property color skeletonShine
        property color shadow1
        property color shadow2
        property color shadow3
        property color glassTint
        property color glassRim
        property color glassRimLow
        property color glassSpecular
        property color glassShadow
    }

    readonly property ColorPalette darkPalette: ColorPalette {
        bgWindow: "#1B1B1E"
        windowTint: theme.rgba(27, 27, 30, 0.80)
        surfaceCard: "#242428"
        surfaceCardHover: "#2A2A2F"
        surfaceCardSelected: "#2C2D44"
        surfaceRaised: "#2C2C31"
        surfaceSheet: theme.rgba(42, 42, 47, 0.88)
        surfaceSheetOpaque: "#2A2A2F"
        surfaceTooltip: "#3A3A40"
        thumbPlaceholder: "#303036"
        borderHairline: theme.rgba(255, 255, 255, 0.08)
        borderCard: theme.rgba(255, 255, 255, 0.06)
        borderControl: theme.rgba(255, 255, 255, 0.12)
        borderStrong: theme.rgba(255, 255, 255, 0.24)
        fillControl: theme.rgba(255, 255, 255, 0.07)
        fillControlHover: theme.rgba(255, 255, 255, 0.10)
        fillControlPressed: theme.rgba(255, 255, 255, 0.05)
        segmentThumb: theme.rgba(255, 255, 255, 0.16)
        segmentSelectedText: "#F4F4F6"
        scrim: theme.rgba(0, 0, 0, 0.40)
        textPrimary: "#F4F4F6"
        textSecondary: "#A9A9B2"
        textTertiary: "#9696A0"
        textDisabled: "#5F5F67"
        textOnAccent: "#FFFFFF"
        accent: "#535BE8"
        accentHover: "#5B63F0"
        accentPressed: "#4A52D9"
        accentText: "#9AA0FF"
        accentTint: theme.rgba(91, 99, 240, 0.14)
        progress: "#7178FF"
        focusRing: "#9AA0FF"
        success: "#3DD68C"
        successTint: theme.rgba(61, 214, 140, 0.14)
        onSuccess: "#1B1B1E"
        warning: "#F5B43C"
        warningTint: theme.rgba(245, 180, 60, 0.14)
        danger: "#FF6B6B"
        dangerTint: theme.rgba(255, 107, 107, 0.14)
        skeleton: theme.rgba(255, 255, 255, 0.07)
        skeletonShine: theme.rgba(255, 255, 255, 0.10)
        shadow1: theme.rgba(0, 0, 0, 0.30)
        shadow2: theme.rgba(0, 0, 0, 0.32)
        shadow3: theme.rgba(0, 0, 0, 0.48)
        glassTint: "#1E1E23"
        glassRim: theme.rgba(255, 255, 255, 0.22)
        glassRimLow: theme.rgba(255, 255, 255, 0.06)
        glassSpecular: theme.rgba(255, 255, 255, 0.55)
        glassShadow: theme.rgba(0, 0, 0, 0.36)
    }

    readonly property ColorPalette lightPalette: ColorPalette {
        bgWindow: "#F2F2F5"
        windowTint: theme.rgba(242, 242, 245, 0.78)
        surfaceCard: "#FFFFFF"
        surfaceCardHover: "#F7F7F9"
        surfaceCardSelected: "#F2F3FD"
        surfaceRaised: "#FFFFFF"
        surfaceSheet: theme.rgba(252, 252, 253, 0.88)
        surfaceSheetOpaque: "#FCFCFD"
        surfaceTooltip: "#FFFFFF"
        thumbPlaceholder: "#E4E4E9"
        borderHairline: theme.rgba(0, 0, 0, 0.08)
        borderCard: theme.rgba(0, 0, 0, 0.06)
        borderControl: theme.rgba(0, 0, 0, 0.12)
        borderStrong: theme.rgba(0, 0, 0, 0.22)
        fillControl: theme.rgba(0, 0, 0, 0.05)
        fillControlHover: theme.rgba(0, 0, 0, 0.08)
        fillControlPressed: theme.rgba(0, 0, 0, 0.11)
        segmentThumb: "#FFFFFF"
        segmentSelectedText: "#1D1D21"
        scrim: theme.rgba(0, 0, 0, 0.20)
        textPrimary: "#1D1D21"
        textSecondary: "#5E5E66"
        textTertiary: "#6E6E77"
        textDisabled: "#A6A6AD"
        textOnAccent: "#FFFFFF"
        accent: "#4B53E0"
        accentHover: "#434BD6"
        accentPressed: "#3A42C4"
        accentText: "#4B53E0"
        accentTint: theme.rgba(75, 83, 224, 0.08)
        progress: "#4B53E0"
        focusRing: "#4B53E0"
        success: "#1A7F45"
        successTint: theme.rgba(26, 127, 69, 0.10)
        onSuccess: "#FFFFFF"
        warning: "#9A5A00"
        warningTint: theme.rgba(154, 90, 0, 0.10)
        danger: "#C9302C"
        dangerTint: theme.rgba(201, 48, 44, 0.08)
        skeleton: theme.rgba(0, 0, 0, 0.06)
        skeletonShine: theme.rgba(255, 255, 255, 0.75)
        shadow1: theme.rgba(0, 0, 0, 0.06)
        shadow2: theme.rgba(0, 0, 0, 0.10)
        shadow3: theme.rgba(0, 0, 0, 0.18)
        glassTint: "#F8F8FA"
        glassRim: theme.rgba(255, 255, 255, 0.85)
        glassRimLow: theme.rgba(0, 0, 0, 0.08)
        glassSpecular: theme.rgba(255, 255, 255, 0.90)
        glassShadow: theme.rgba(0, 0, 0, 0.14)
    }

    // Contraste élevé : couleurs système. Si la palette Qt ne reflète pas fidèlement
    // GetSysColor (à vérifier dans ENH-004), SystemAppearance (C++) expose ces couleurs
    // et on remplace les liaisons _sys.* par SystemAppearance.* — même structure.
    readonly property SystemPalette _sys: SystemPalette { colorGroup: SystemPalette.Active }
    readonly property SystemPalette _sysDisabled: SystemPalette { colorGroup: SystemPalette.Disabled }

    readonly property ColorPalette highContrastPalette: ColorPalette {
        bgWindow: theme._sys.window
        windowTint: theme._sys.window
        surfaceCard: theme._sys.window
        surfaceCardHover: theme._sys.window
        surfaceCardSelected: theme._sys.window
        surfaceRaised: theme._sys.window
        surfaceSheet: theme._sys.window
        surfaceSheetOpaque: theme._sys.window
        surfaceTooltip: theme._sys.window
        thumbPlaceholder: theme._sys.window
        borderHairline: theme._sys.windowText
        borderCard: theme._sys.windowText
        borderControl: theme._sys.buttonText
        borderStrong: theme._sys.windowText
        fillControl: theme._sys.button
        fillControlHover: theme._sys.button
        fillControlPressed: theme._sys.button
        segmentThumb: theme._sys.highlight
        segmentSelectedText: theme._sys.highlightedText
        scrim: "transparent"
        textPrimary: theme._sys.windowText
        textSecondary: theme._sys.windowText
        textTertiary: theme._sys.windowText
        textDisabled: theme._sysDisabled.windowText
        textOnAccent: theme._sys.highlightedText
        accent: theme._sys.highlight
        accentHover: theme._sys.highlight
        accentPressed: theme._sys.highlight
        accentText: theme._sys.highlight
        accentTint: "transparent"
        progress: theme._sys.highlight
        focusRing: theme._sys.highlight
        success: theme._sys.windowText
        successTint: "transparent"
        onSuccess: theme._sys.window
        warning: theme._sys.windowText
        warningTint: "transparent"
        danger: theme._sys.windowText
        dangerTint: "transparent"
        skeleton: theme._sysDisabled.windowText
        skeletonShine: theme._sysDisabled.windowText
        shadow1: "transparent"
        shadow2: "transparent"
        shadow3: "transparent"
        glassTint: theme._sys.window
        glassRim: theme._sys.windowText
        glassRimLow: theme._sys.windowText
        glassSpecular: "transparent"
        glassShadow: "transparent"
    }

    readonly property ColorPalette color: highContrast ? highContrastPalette
                                                 : (dark ? darkPalette : lightPalette)

    // Matières : translucidité désactivée en contraste élevé ou si Windows
    // « Effets de transparence » est désactivé.
    readonly property bool materialsEnabled: !highContrast && transparencyEnabled

    // ------------------------------------------------------------------ Verre « liquide »
    // Rendu par le composant GlassSurface (ShaderEffect + glass.frag.qsb), qui échantillonne
    // une ShaderEffectSource du contenu (demi-résolution, mipmaps). Voir design-system.md §6.
    // Qualité : 2 = complète, 1 = allégée (flou + teinte), 0 = opaque.
    // glassQualityRequested est posé par RenderPolicy (C++) selon le budget GPU mesuré.
    readonly property int glassFull: 2
    readonly property int glassLite: 1
    readonly property int glassOpaque: 0
    readonly property int glassQuality: materialsEnabled ? glassQualityRequested : glassOpaque
    // Pas de fusion animée ni de reflet qui suit le pointeur en mouvement réduit.
    readonly property bool glassMotion: !reduceMotion && glassQuality === glassFull

    component GlassLevel: QtObject {
        property real blur          // px logiques (rayon de flou équivalent)
        property real saturation    // multiplicateur
        property real refraction    // déplacement max en px logiques au bord
        property real bezel         // largeur de la zone réfractante (px) : aucun texte dedans
        property real specular      // intensité max du reflet (0..1)
        property real tintScale     // multiplie l'alpha de teinte adaptative
    }
    component GlassLevels: QtObject {
        property GlassLevel regular
        property GlassLevel clear
        property GlassLevel thick
    }
    readonly property GlassLevels glass: GlassLevels {
        regular: GlassLevel { blur: 24; saturation: 1.35; refraction: 12; bezel: 14; specular: 0.35; tintScale: 1.0 }
        clear:   GlassLevel { blur: 8;  saturation: 1.2;  refraction: 16; bezel: 16; specular: 0.45; tintScale: 0.55 }
        thick:   GlassLevel { blur: 40; saturation: 1.2;  refraction: 8;  bezel: 18; specular: 0.25; tintScale: 1.0 }
    }

    // Teinte adaptative : alpha = mix(alphaA, alphaB, smoothstep(lumaA, lumaB, Lb)),
    // Lb = luminance relative du fond flouté sous le pixel (calcul dans le shader).
    component AdaptiveTint: QtObject {
        property real alphaA
        property real alphaB
        property real lumaA
        property real lumaB
    }
    readonly property AdaptiveTint glassTintCurve: dark
        ? darkTintCurve : lightTintCurve
    readonly property AdaptiveTint darkTintCurve: AdaptiveTint { alphaA: 0.62; alphaB: 0.88; lumaA: 0.05; lumaB: 0.50 }
    readonly property AdaptiveTint lightTintCurve: AdaptiveTint { alphaA: 0.90; alphaB: 0.62; lumaA: 0.10; lumaB: 0.60 }

    component GlassGeometry: QtObject {
        readonly property int bar: 22
        readonly property int capsule: 999
        readonly property int sheet: 20
        readonly property int toast: 18
        readonly property real fusionK: 16       // smooth-min (px)
        readonly property int fusionMaxShapes: 4
        readonly property real lightAngleDeg: -50
        readonly property real pressScale: 1.03
        readonly property real pressSpecularBoost: 1.6
        readonly property real segmentStretch: 1.12
        // Budget : rendu de l'interface sur la RTX, par image affichée.
        readonly property real budgetGpuMs: 1.0
        readonly property real budgetGpuMsMax: 2.0
        readonly property real backdropScale: 0.5
        readonly property int backdropMaxHzDuringProcessing: 10
    }
    readonly property GlassGeometry glassGeo: GlassGeometry {}

    // Biseau effectif d'une surface de verre : jamais de texte dans le biseau.
    // Ex. capsule 44 px avec un libellé headline (interligne 20) : min(14, 12) = 12.
    function glassBezelFor(level, height, contentLineHeight) {
        return Math.max(0, Math.min(level.bezel, Math.floor((height - contentLineHeight) / 2)))
    }

    // ------------------------------------------------------------------ Typographie
    readonly property string familyDisplay: "Segoe UI Variable Display"
    readonly property string familyText: "Segoe UI Variable Text"
    readonly property string familySmall: "Segoe UI Variable Small"
    readonly property string familyMono: "Cascadia Mono"
    readonly property string familyFallback: "Segoe UI"       // si la variante optique est absente
    readonly property string familyMonoFallback: "Consolas"

    component TypeStyle: QtObject {
        property string family
        property int size
        property int lineHeight
        property int weight: Font.Normal
        property real letterSpacing: 0
        readonly property font font: Qt.font({
            family: family,
            pixelSize: size,
            weight: weight,
            letterSpacing: letterSpacing,
            hintingPreference: Font.PreferNoHinting
        })
    }

    component TypeScale: QtObject {
        property TypeStyle display
        property TypeStyle title1
        property TypeStyle title2
        property TypeStyle headline
        property TypeStyle body
        property TypeStyle callout
        property TypeStyle caption
        property TypeStyle captionStrong
        property TypeStyle badge
        property TypeStyle mono
    }

    readonly property TypeScale typo: TypeScale {
        display:       TypeStyle { family: theme.familyDisplay; size: 28; lineHeight: 34; weight: Font.DemiBold; letterSpacing: -0.2 }
        title1:        TypeStyle { family: theme.familyDisplay; size: 22; lineHeight: 28; weight: Font.DemiBold }
        title2:        TypeStyle { family: theme.familyText;    size: 17; lineHeight: 22; weight: Font.DemiBold }
        headline:      TypeStyle { family: theme.familyText;    size: 14; lineHeight: 20; weight: Font.DemiBold }
        body:          TypeStyle { family: theme.familyText;    size: 14; lineHeight: 20; weight: Font.Normal }
        callout:       TypeStyle { family: theme.familyText;    size: 13; lineHeight: 18; weight: Font.Normal }
        caption:       TypeStyle { family: theme.familySmall;   size: 12; lineHeight: 16; weight: Font.Normal }
        captionStrong: TypeStyle { family: theme.familySmall;   size: 12; lineHeight: 16; weight: Font.DemiBold }
        badge:         TypeStyle { family: theme.familySmall;   size: 11; lineHeight: 14; weight: Font.DemiBold; letterSpacing: 0.2 }
        mono:          TypeStyle { family: theme.familyMono;    size: 12; lineHeight: 18; weight: Font.Normal }
    }

    // Rendu du texte : NativeRendering (DirectWrite) par défaut, net à 100–150 %.
    // Ne jamais mettre à l'échelle un Text pendant une animation (la zone de dépôt
    // agrandit l'illustration et la bordure, pas le texte).
    readonly property int textRenderType: Text.NativeRendering

    // ------------------------------------------------------------------ Espacements (grille 4 px)
    component Spacing: QtObject {
        readonly property int s0_5: 2
        readonly property int s1: 4
        readonly property int s2: 8
        readonly property int s3: 12
        readonly property int s4: 16
        readonly property int s5: 20
        readonly property int s6: 24
        readonly property int s8: 32
        readonly property int s10: 40
        readonly property int s12: 48
        readonly property int s16: 64
    }
    readonly property Spacing space: Spacing {}

    // ------------------------------------------------------------------ Rayons
    component Radii: QtObject {
        readonly property int xs: 4
        readonly property int sm: 6
        readonly property int md: 8
        readonly property int lg: 12
        readonly property int xl: 16
        readonly property int full: 999
    }
    readonly property Radii radius: Radii {}

    // ------------------------------------------------------------------ Tailles
    component Sizes: QtObject {
        readonly property int controlSm: 28
        readonly property int control: 32
        readonly property int controlLg: 36
        readonly property int hitMin: 32
        readonly property int iconSm: 16
        readonly property int icon: 20
        readonly property int iconLg: 24
        readonly property real iconStroke: 1.75
        readonly property int toolbarH: 52
        readonly property int statusBarH: 32
        readonly property int titleBarH: 32
        readonly property int cardMinH: 80
        readonly property int thumbW: 96
        readonly property int thumbH: 54
        readonly property int thumbActiveW: 128
        readonly property int thumbActiveH: 72
        readonly property int thumbNarrowW: 56
        readonly property int thumbNarrowH: 32
        readonly property int progressH: 4
        readonly property int progressHActive: 6
        readonly property int badgeH: 20
        readonly property int statusDot: 8
        readonly property int checkBadge: 20
        readonly property int focusRingW: 2
        readonly property int focusRingOffset: 2
        readonly property int border: 1
        readonly property real dropZoneBorder: 1.5
        readonly property int windowMinW: 760
        readonly property int windowMinH: 520
        readonly property int windowDefaultW: 1100
        readonly property int windowDefaultH: 720
        readonly property int contentMaxW: 880
        readonly property int sheetW: 520
        readonly property int sheetTop: 64
        readonly property int toastMaxW: 480
        readonly property int primaryButtonMinW: 148
        readonly property int breakpointNarrow: 900
        readonly property int breakpointShort: 600
        // Bouton de verre autonome (capsule), ENH-004 : 44 px, marge latérale 24 px.
        readonly property int glassButtonH: 44
        readonly property int glassButtonPadX: 24
    }
    readonly property Sizes size: Sizes {}

    // ------------------------------------------------------------------ Élévations
    // Avec Qt >= 6.9 : RectangularShadow { offset.y: e.y; blur: e.blur; color: e.color; radius: <rayon de l'élément> }
    // Sinon : MultiEffect { shadowEnabled: true; shadowVerticalOffset: e.y; blurMax: 64;
    //                       shadowBlur: e.blur / 64; shadowColor: e.color }
    component Shadow: QtObject {
        property real y
        property real blur
        property color color
    }
    component Elevations: QtObject {
        property Shadow e1
        property Shadow e2
        property Shadow e3
    }
    readonly property Elevations elevation: Elevations {
        e1: Shadow { y: 1;  blur: 2;  color: theme.color.shadow1 }
        e2: Shadow { y: 4;  blur: 12; color: theme.color.shadow2 }
        e3: Shadow { y: 16; blur: 40; color: theme.color.shadow3 }
    }

    // ------------------------------------------------------------------ Matières
    component Materials: QtObject {
        readonly property real sheetBlur: 32          // MultiEffect : blurMax 32, blur 1.0
        readonly property real sheetSaturation: 0.2   // MultiEffect.saturation (0 = neutre ; 0.2 ≈ ×1.2)
    }
    readonly property Materials material: Materials {}

    // ------------------------------------------------------------------ Mouvement : durées (ms)
    // dur.<nom>      : fondus d'opacité et de couleur (conservés en mouvement réduit)
    // dur.move<Nom>  : déplacements, échelles, hauteurs (0 ms en mouvement réduit)
    component Durations: QtObject {
        property bool reduce: false
        readonly property int press: 100
        readonly property int fast: 150
        readonly property int base: 200
        readonly property int medium: 250
        readonly property int slow: 300
        readonly property int slower: 350
        readonly property int stagger: 30
        readonly property int themeFade: 300
        readonly property int highlightFlash: 400
        readonly property int tooltipDelay: 600
        readonly property int shimmer: 1400
        readonly property int indeterminate: 1600
        readonly property int toast: 5000
        readonly property int refusedMessage: 6000
        // Exceptions à « les fondus restent » : instantanés en mouvement réduit (ENH-004).
        readonly property int windowAppear: reduce ? 0 : 200
        readonly property int themeSwap: reduce ? 0 : 250   // < 300 ms capture comprise

        readonly property int movePress: reduce ? 0 : press
        readonly property int moveFast: reduce ? 0 : fast
        readonly property int moveBase: reduce ? 0 : base
        readonly property int moveMedium: reduce ? 0 : medium
        readonly property int moveSlow: reduce ? 0 : slow
        readonly property int moveSlower: reduce ? 0 : slower
        readonly property int moveStagger: reduce ? 0 : stagger
        // Boucles (squelette, barre indéterminée) : arrêtées en mouvement réduit.
        readonly property bool loopsEnabled: !reduce
    }
    readonly property Durations dur: Durations { reduce: theme.reduceMotion }

    // ------------------------------------------------------------------ Mouvement : courbes
    // À utiliser avec easing.type: Easing.BezierSpline ; format Qt [x1, y1, x2, y2, 1, 1].
    // Identiques aux cubic-bezier() de tokens.css.
    component Curves: QtObject {
        readonly property list<real> standard:   [0.4, 0.0, 0.2, 1.0, 1.0, 1.0]
        readonly property list<real> decelerate: [0.0, 0.0, 0.2, 1.0, 1.0, 1.0]
        readonly property list<real> accelerate: [0.4, 0.0, 1.0, 1.0, 1.0, 1.0]
        readonly property list<real> emphasized: [0.32, 0.72, 0.0, 1.0, 1.0, 1.0]
        readonly property list<real> spring:     [0.34, 1.30, 0.64, 1.0, 1.0, 1.0]
        readonly property list<real> linear:     [0.0, 0.0, 1.0, 1.0, 1.0, 1.0]
    }
    readonly property Curves ease: Curves {}

    // Ressort physique : uniquement pour le relâchement d'une carte glissée
    // (SpringAnimation { spring: Theme.springDrag.spring; damping: Theme.springDrag.damping; ... }).
    // Valeurs de départ, à affiner sur le prototype ENH-004.
    component SpringParams: QtObject {
        readonly property real spring: 4.0
        readonly property real damping: 0.40
        readonly property real mass: 1.0
        readonly property real epsilon: 0.25
    }
    readonly property SpringParams springDrag: SpringParams {}
}
