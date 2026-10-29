# ENH-004 — Squelette Qt Quick et prototype Mica : spécification UI

Statut : v1.0 (ui-designer), 2026-10-21. Pour : developer (implémentation), reviewer, qa-tester, ui-designer (contrôle design).
Sources : `docs/design/ux/ENH-004.md` (périmètre, 9 états, textes, clavier, critères), `design-system.md` v1.2, `Theme.qml` / `tokens.json` / `tokens.css` v1.2, ADR-001, ADR-036.
Maquette de référence : `maquettes/enh-004-squelette.html` (double-clic ; Edge ou Chrome pour la réfraction). Bouton **Cotes** : toutes les dimensions ci-dessous, en pixels logiques.

Légende : **[H]** = hypothèse technique à confirmer pendant le prototype ; toute valeur citée vient de `Theme` (aucune valeur en dur dans le QML de l'app).

---

## 1. Ce qui change dans les tokens (v1.2)

| Token | Valeur | Pourquoi |
|---|---|---|
| `Theme.size.glassButtonH` | 44 | Capsule de verre autonome (nouveau composant, design-system §9.1) |
| `Theme.size.glassButtonPadX` | 24 | Marge latérale du libellé |
| `Theme.dur.windowAppear` | 200 (0 en mouvement réduit) | Ouverture de la fenêtre |
| `Theme.dur.themeSwap` | 250 (0 en mouvement réduit) | Bascule de thème, tient « < 300 ms » capture comprise |
| `Theme.glassBezelFor(level, h, lineHeight)` | min(level.bezel, ⌊(h − lineHeight) / 2⌋) | Aucun texte dans le biseau : 12 px pour la capsule |

`Theme.qml` de `docs/design/ui/` est **copié tel quel** dans `src/app/qml/Enhance/Design/Theme.qml` (même commit que toute évolution de `tokens.json`). Test unitaire conseillé : `tests/unit/qml/tst_theme.qml` compare 10 valeurs de `Theme` à `tokens.json`.

---

## 2. Structure du code

### 2.1 Modules QML (`src/app/qml/`, conforme à l'ADR-036)

L'ADR-036 prévoit un module QML dans `src/app/qml/`. On le découpe en **deux URI** (deux `qt_add_qml_module` dans le même `CMakeLists.txt` ou deux sous-dossiers), pour que le design system ne dépende jamais de l'app :

```
src/app/qml/
  Enhance/Design/             URI Enhance.Design  (design system : aucune référence au C++ de l'app)
    Theme.qml                 singleton (QT_QML_SINGLETON_TYPE TRUE) — copie de docs/design/ui/Theme.qml
    ThemedText.qml            Text + style typographique (font, lineHeight FixedHeight, renderType, couleur)
    GlassSurface.qml          surface de verre (3 qualités) — §5
    GlassButton.qml           bouton de verre autonome (capsule 44) — §6
    FocusRing.qml             anneau de focus clavier — §6.4
    DashedFrame.qml           cadre arrondi en pointillés / trait plein (Shape) — §4.2
    EmptyIllustration.qml     illustration de l'état vide (Shape, décorative) — §4.3
    StatusDot.qml             pastille d'état 8 px + halo — §4.5
    shaders/glass.frag        compilé en .qsb par qt_add_shaders (Qt Shader Tools : outil de build)
  Enhance/App/                URI Enhance.App
    Main.qml                  fenêtre, couches, bascule de thème, raccourcis — §3
    EmptyStateInert.qml       état vide W1 sans action — §4
    SpringDemo.qml            capsule + compteur + mention (provisoire, supprimé par ENH-016) — §6.5
    AppStatusBar.qml          barre d'état — §4.5
  icons/enhance-app.svg       copie de docs/design/ui/icones/enhance-app-provisoire.svg
```

Règle : un fichier de `Enhance/App` n'utilise que `Theme.*` et les composants de `Enhance.Design` ; aucune couleur `#…`, aucune taille, durée ou courbe numérique (le reviewer et le contrôle design cherchent `#[0-9A-Fa-f]{3,8}`, `duration: [0-9]`, `: [0-9]+$` sur les propriétés de géométrie).

### 2.2 C++ de présentation (`src/app/`, cible `enh_app`) et Win32 (`src/platform/win/`, cible `enh_platform`)

| Classe | Cible | Rôle | Exposé au QML |
|---|---|---|---|
| `platform::win::Appearance` (fonctions libres + struct) | `enh_platform` (sans Qt) | Lit `AppsUseLightTheme`, `EnableTransparency` (HKCU\…\Themes\Personalize), `SPI_GETHIGHCONTRAST`, `SPI_GETCLIENTAREAANIMATION`, `GetSysColor` (COLOR_WINDOW, WINDOWTEXT, BTNFACE, BTNTEXT, HIGHLIGHT, HIGHLIGHTTEXT, GRAYTEXT) | non |
| `platform::win::WindowChrome` | `enh_platform` | Attributs DWM sur un `HWND` (§3.2) | non |
| `SystemAppearance` | `enh_app` | QObject : `dark`, `highContrast`, `reduceMotion`, `transparency`, couleurs système ; mis à jour sur `WM_SETTINGCHANGE` / `WM_THEMECHANGED` / `WM_SYSCOLORCHANGE` (`QAbstractNativeEventFilter`) ; `dark` peut venir de `QStyleHints::colorScheme()` si cohérent | singleton QML |
| `GlassQualityGovernor` | `enh_app` (C++20 pur, sans Qt, testé en CI) | Règle de dégradation du verre (§5.4) | non |
| `RenderPolicy` | `enh_app` | QObject : mesure le temps GPU de rendu Qt (requêtes D3D11 *timestamp*), alimente le gouverneur, publie `glassQuality` ; option `--glass-quality=0|1|2` | singleton QML |
| `DemoCounter` | `enh_app` | `Q_PROPERTY(int count READ count NOTIFY countChanged)`, `Q_INVOKABLE void increment()` | singleton QML |
| `AppStatus` | `enh_app` | `Q_PROPERTY(QString gpuStatusText CONSTANT)` = `tr("GeForce RTX 5070 · prête")` (texte fixe, cf. question PO n° 1 de l'UX) ; `Q_INVOKABLE void requestQuit()` (ENH-004 : quitte ; plus tard : feuille si traitement) | singleton QML |

Branchement **unique** dans `Main.qml` (et nulle part ailleurs) :
```qml
Binding { target: Theme; property: "dark";                  value: SystemAppearance.dark }
Binding { target: Theme; property: "highContrast";          value: SystemAppearance.highContrast }
Binding { target: Theme; property: "reduceMotion";          value: SystemAppearance.reduceMotion }
Binding { target: Theme; property: "transparencyEnabled";   value: SystemAppearance.transparency && !RenderPolicy.micaUnavailable }
Binding { target: Theme; property: "glassQualityRequested"; value: RenderPolicy.glassQuality }
Binding { target: Theme; property: "devicePixelRatio";      value: window.screen ? window.screen.devicePixelRatio : 1 }
```
`RenderPolicy.micaUnavailable` = repli du risque A1 (Mica instable avec la fenêtre Qt Quick transparente) : passe toute l'app en mode opaque, décision consignée en ADR.

---

## 3. Fenêtre

### 3.1 Géométrie
| Propriété | Valeur |
|---|---|
| Taille de la zone client par défaut | `Theme.size.windowDefaultW × windowDefaultH` = 1100 × 720, **réduite** à 90 % de `Screen.desktopAvailableWidth/Height` si l'écran est plus petit, centrée sur l'écran du pointeur |
| Taille minimale de la zone client | `minimumWidth: Theme.size.windowMinW` (760), `minimumHeight: Theme.size.windowMinH` (520), en pixels logiques à toutes les échelles |
| Barre de titre | **DWM, standard** (pas de barre dessinée) : texte « Enhance », icône provisoire (`QGuiApplication::setWindowIcon(QIcon(":/icons/enhance-app.svg"))`), boutons Windows natifs, snap layouts |
| Mise à l'échelle | Manifeste PerMonitorV2 ; arrondi Qt `PassThrough` (125 / 175 % exacts) ; `WM_DPICHANGED` géré par Qt |

Les tailles 1100 × 720 et 760 × 520 sont celles de la **zone client** (`Window.width/height`) ; la barre de titre DWM s'y ajoute. La maquette dessine exactement cela.

### 3.2 Mica, barre de titre, ouverture sans flash
Ordre impératif (dans `main.cpp` / `WindowChrome`, avant le premier `show()`) :
1. `QQuickWindow::setDefaultAlphaBuffer(true)` avant la création de la fenêtre (mode Mica uniquement) **[H, risque A1]**.
2. `window.color` = `"transparent"` (Mica) ou `Theme.color.bgWindow` (opaque / contraste élevé). **Jamais** le blanc par défaut.
3. `DWMWA_USE_IMMERSIVE_DARK_MODE` (20) = `Theme.dark`.
4. `DWMWA_SYSTEMBACKDROP_TYPE` (38) = `DWMSBT_MAINWINDOW` (2) si Mica, sinon `DWMSBT_NONE` (1) ; `DwmExtendFrameIntoClientArea` avec marges −1 si Mica.
5. `DWMWA_CAPTION_COLOR` (35) = `Theme.color.bgWindow` (COLORREF) en mode opaque **pour qu'il n'y ait aucune couture entre la barre de titre et le contenu** ; `DWMWA_COLOR_DEFAULT` en Mica et en contraste élevé. **Exception (contrôle design, 2026-10-23)** : si l'utilisateur a activé « Afficher la couleur d'accentuation sur les barres de titre » (`HKCU\Software\Microsoft\Windows\DWM\ColorPrevalence = 1`), Windows teinte la barre de titre avec l'accent : c'est son choix explicite, on le respecte (comportement système, pas une couture) et on laisse `DWMWA_COLOR_DEFAULT` **aussi en mode opaque**, pour que la barre soit la même dans les deux modes (design-system §6.6).
6. `DWMWA_CLOAK` (13) = TRUE, `show()`, puis au **premier `QQuickWindow::frameSwapped`** : `DWMWA_CLOAK` = FALSE. La première image visible est donc complète (fond + titre + état vide) ; l'animation d'ouverture est celle de Windows (≈ `Theme.dur.windowAppear`, désactivée automatiquement si « Effets d'animation » est coupé). **Pas** de fondu d'opacité côté Qt (il montrerait un Mica vide avant le contenu).
7. Contenu de `Main.qml` entièrement synchrone : pas de `Loader { asynchronous: true }`, pas d'image chargée en différé (l'illustration est un `Shape`).

Changement à chaud (thème, transparence, contraste élevé) : `WindowChrome` réapplique 2 à 5 **dans la même itération** que le basculement de `Theme` (§3.4).

### 3.3 Couches de `Main.qml` (de bas en haut)
```
Window (id: window)
└─ Item contentRoot (anchors.fill)
   ├─ Item backdropLayer            ← capturé par le verre (sourceItem)
   │  ├─ Rectangle tint             color: Theme.materialsEnabled ? Theme.color.windowTint : Theme.color.bgWindow
   │  ├─ EmptyStateInert            (§4)
   │  └─ GlassTestbed               motif sous la capsule, visible seulement avec --glass-testbed (§6.6)
   ├─ ShaderEffectSource backdrop   sourceItem: backdropLayer, hideSource: false, mipmap: true, live: true,
   │                                textureSize: Qt.size(width * dpr * Theme.glassGeo.backdropScale, height * dpr * Theme.glassGeo.backdropScale)
   │                                (une seule par fenêtre ; aucun rendu si backdropLayer ne change pas)
   ├─ SpringDemo                    (§6.5) — au-dessus de la capture, jamais dedans
   ├─ AppStatusBar                  (§4.5)
   └─ Image themeSnapshot           bascule de thème (§3.4), visible: false au repos
```
Note Mica : la capture ne voit que ce que Qt dessine (teinte + état vide), **pas** le Mica (composé par DWM derrière la fenêtre). Le shader travaille en alpha prémultiplié : là où la capture est translucide, le verre laisse voir le Mica à travers sa teinte. Sur ce fond calme, la réfraction est invisible : c'est attendu (UX §4) ; le motif de test sert à la prouver.

### 3.4 Bascule de thème (Windows change pendant l'exécution)
```qml
function swapTheme(apply) {                       // apply() = pose les nouvelles entrées de Theme + WindowChrome
    if (Theme.dur.themeSwap === 0) { apply(); return }
    contentRoot.grabToImage(function (r) {
        themeSnapshot.source = r.url; themeSnapshot.opacity = 1; themeSnapshot.visible = true
        apply()                                   // barre de titre DWM + Theme basculent dans la même itération
        snapshotFade.restart()                    // OpacityAnimator 1 → 0, Theme.dur.themeSwap, Theme.ease.standard
    })                                            // à la fin : visible = false, source = ""
}
```
Ni animation couleur par couleur, ni clignotement ; total < 300 ms (250 + 1 à 2 images de capture). Mouvement réduit : instantané.

---

## 4. Zone centrale : état vide inerte, barre d'état

### 4.1 Grille verticale de la zone client (pixels logiques)
| Rangée | Hauteur | Contenu |
|---|---|---|
| 1 | `Theme.size.toolbarH` = **52** | Vide : réserve la place de la future barre d'outils (W1), pour que rien ne bouge quand ENH-016 l'ajoutera |
| 2 | reste (≥ 300 à 760 × 520) | Cadre de l'état vide |
| 3 | `Theme.space.s6` = **24** | Espace |
| 4 | 44 + 8 + 16 = **68** | Démonstration : capsule 44, mention à 8 sous la capsule (caption 16) |
| 5 | `Theme.space.s6` = **24** | Espace |
| 6 | `Theme.size.statusBarH` = **32** | Barre d'état |

À 1100 × 720 : cadre = 720 − 52 − 24 − 68 − 24 − 32 = **520** px de haut. À 760 × 520 : **320** px (contenu du cadre : 256 px, aucun défilement).

### 4.2 Colonne et cadre
| Élément | Mesure |
|---|---|
| Colonne | largeur `min(Theme.size.contentMaxW + 2 × s6, largeur)` = 928 max, centrée ; marges latérales `s6` = 24 (≥ 900 px), `s4` = 16 (< `breakpointNarrow` 900) |
| Cadre (`DashedFrame`) | remplit la rangée 2 dans la colonne ; rayon `Theme.radius.xl` 16 ; trait `Theme.size.dropZoneBorder` 1,5 `Theme.color.borderStrong`, pointillés 6 / 6 → en QML `dashPattern: [4, 4]` (unités = largeur de trait) ; tracé inscrit à 0,75 px du bord (`PathRectangle { x: 0.75; y: 0.75; width: w − 1.5; height: h − 1.5; radius: 16 }`, Qt ≥ 6.7) ; **contraste élevé** : trait plein 2 px `Theme.color.borderStrong` (= WindowText), inscrit à 1 px, rayon 15 |
| Rendu du trait | net à 125–200 % : `preferredRendererType: Shape.CurveRenderer` si les pointillés y sont pris en charge, sinon `GeometryRenderer` + `antialiasing: true` **[H]** |
| Padding intérieur | `s8` 32 en haut / bas, `s6` 24 à gauche / droite |
| Pile centrée | largeur max 440 ; illustration 120 × 96 ; puis 12 + 8 = **20** jusqu'au titre ; titre ; **8** ; sous-titre |
| Titre | `Theme.typo.title1` (22 / 28, 600, Display), `textPrimary`, centré, une ligne |
| Sous-titre | `Theme.typo.body` (14 / 20), `textSecondary`, centré, `wrapMode: Text.WordWrap`, largeur `min(440, colonne − 48)` (2 lignes à toutes les tailles ; jamais d'ellipse) |
| Interaction | **aucune** : pas de `HoverHandler`, pas de `DropArea` (Windows affiche alors le curseur « interdit »), pas de curseur main |

Tous les `Text` : `renderType: Theme.textRenderType`, `lineHeightMode: Text.FixedHeight`, `lineHeight: style.lineHeight` (via `ThemedText`).

### 4.3 Illustration (`EmptyIllustration.qml`, décorative)
Repère 120 × 96, identique à la maquette (copier les tracés) : cadre arrière `rect(24, 14, 74, 50, r 10)` tourné de −8° autour de son centre, remplissage `fillControl`, trait 1,5 `borderStrong` ; cadre avant `rect(16, 26, 84, 56, r 10)`, remplissage `surfaceCard`, trait 1,5 `borderStrong` ; triangle « lecture » `accentText` ; pastille `circle(96, 26, r 14)` `accent` ; flèche (trait 2,25, bouts ronds) `textOnAccent`. `Accessible.ignored: true`. Aucune animation dans ENH-004.

### 4.4 Textes (exacts, `qsTr`, `lang = fr`)
« Déposez vos vidéos ici » · « Enhance les passe en 4K fluide, sur votre ordinateur. Rien n'est envoyé sur Internet. » · « Tester le ressort » · « Essais : %1 » (espace **insécable** U+00A0 avant « : ») · « Démonstration technique » · « GeForce RTX 5070 · prête » (point médian U+00B7). Apostrophe droite `'` comme dans `ENH-004.md` (cohérence avec l'écran principal).

### 4.5 Barre d'état (`AppStatusBar.qml`)
| Élément | Mesure |
|---|---|
| Hauteur | `Theme.size.statusBarH` 32, pleine largeur de la fenêtre |
| Séparateur haut | `Theme.hairline` (1 pixel **physique**), `borderHairline`, y arrondi au pixel physique : `Math.round(y * dpr) / dpr` |
| Fond | aucun (e0, le Mica continue) |
| Contenu | à `s4` 16 du bord gauche, centré verticalement : `StatusDot` puis **8** puis texte `Theme.typo.caption` (12 / 16) `textSecondary` |
| `StatusDot` | disque 8 (`statusDot`) `success`, halo : disque 14 centré `successTint` ; contraste élevé : disque `textPrimary` (WindowText), sans halo ; `Accessible.role: Accessible.Graphic`, `Accessible.name: qsTr("Carte graphique prête")` |

---

## 5. `GlassSurface.qml`

### 5.1 API
```qml
GlassSurface {
    level: Theme.glass.regular            // GlassLevel : blur, saturation, refraction, bezel, specular, tintScale
    radius: height / 2                    // capsule ; le SDF reçoit le rayon réel (pas 999)
    bezel: Theme.glassBezelFor(level, height, Theme.typo.headline.lineHeight)
    backdrop: backdrop                    // la ShaderEffectSource partagée de Main.qml
    lightPos: Qt.point(x, y)              // position du reflet (px locaux)
    pressed: false                        // reflet × Theme.glassGeo.pressSpecularBoost
    readonly property int quality: Theme.glassQuality   // 2 complète, 1 allégée, 0 opaque
}
```

### 5.2 Rendu par qualité
| Qualité | Rendu |
|---|---|
| 2 — complète | `ShaderEffect` (`glass.frag.qsb`) : réfraction en lentille dans le biseau, flou par mipmaps (niveau ≈ log2(blur × backdropScale)), saturation, teinte adaptative (`Theme.glassTintCurve`, `glassTint`), liseré 1 px physique (`glassRim` côté lumière, `glassRimLow` à l'ombre, angle `glassGeo.lightAngleDeg`), reflet `glassSpecular × level.specular` confiné au biseau ; ombre verre : deux `RectangularShadow` (`Theme.elevation.glassFar` y 8 / flou 28 et `Theme.elevation.glassNear` y 1 / flou 3, `glassShadow` ; jetons ajoutés en v1.3) |
| 1 — allégée | même shader, réfraction = 0 et reflet fixe (pas de suivi du pointeur) |
| 0 — opaque | `Rectangle` `surfaceRaised`, `border.width: Theme.borderWidth`, `border.color: borderControl` ; ombre e2 (`RectangularShadow` y 4 / flou 12 `shadow2`) ; **aucun** `ShaderEffect` ni lecture de la capture |
| Contraste élevé | qualité 0 forcée (`materialsEnabled = false`) : fond `fillControl` (= ButtonFace), liseré 2 px `borderControl` (= ButtonText), **ni ombre ni reflet** (les ombres HC sont transparentes : ne pas instancier `RectangularShadow`) |

Passage d'une qualité à l'autre : fondu croisé `Theme.dur.fast` / `Theme.ease.standard` (deux couches, opacités animées), jamais de saut.
`sourceRect` du verre dans la capture : `backdropLayer.mapFromItem(surface, 0, 0, width, height)`, recalculé seulement quand la géométrie change (pendant le ressort, l'échelle s'applique au rendu, pas au `sourceRect`).

### 5.3 Performances (à prouver, design-system §12)
Aucun rendu au repos (pas de `Timer`, pas d'animation en boucle ; le reflet ne bouge que sous le pointeur). Mesures attendues dans ENH-004 : §12 n° 1, 2, 3, 6, 10, 11, 12, 13 au repos et sous **charge GPU synthétique** ; n° 7, 8, 9 « pendant un traitement réel » dès que le pipeline existe (proposition : ENH-010), consigné par la QA.

### 5.4 `RenderPolicy` et `GlassQualityGovernor`
- Mesure : requêtes `D3D11_QUERY_TIMESTAMP_DISJOINT` + 2 × `D3D11_QUERY_TIMESTAMP` posées sur `beforeRenderPassRecording` / `afterRenderPassRecording` du thread de rendu (périphérique via `QSGRendererInterface`), lues 2 à 3 images plus tard (jamais d'attente bloquante) **[H]**.
- Gouverneur (pur C++20, `std::chrono`) : `Quality update(std::chrono::duration<double, std::milli> gpu, bool processing, std::chrono::steady_clock::time_point now)`. Au repos (`processing == false`) : **complète**. Pendant un traitement : moyenne glissante 30 images > 4 ms → allégée ; > 8 ms → opaque ; retour d'un cran après 10 s sous `Theme.glassGeo.budgetGpuMs` (1 ms). Seuils = constantes nommées reprises de `tokens.json` (`glass.budget`).
- `--glass-quality=0|1|2` force la qualité (QA, contrôle design) ; valeur hors bornes ignorée. Dans ENH-004 `processing` vaut toujours `false`.
- Tests CI (sans GPU) : le gouverneur seul (bascule, hystérésis 10 s, retour, valeurs extrêmes).

---

## 6. `GlassButton.qml` et la démonstration

### 6.1 Mesures
| Propriété | Valeur |
|---|---|
| Base | `T.AbstractButton` (QtQuick.Templates) |
| Hauteur | `Theme.size.glassButtonH` **44** |
| Padding horizontal | `Theme.size.glassButtonPadX` **24** de chaque côté ; vertical : libellé centré (12 au-dessus / dessous = biseau effectif) |
| Largeur | `Math.max(Theme.size.primaryButtonMinW, contentItem.implicitWidth + 48)`, arrondie à l'entier |
| Rayon | `height / 2` (capsule) |
| Libellé | `Theme.typo.headline` (14 / 20, 600), `textPrimary`, une ligne, jamais d'ellipse ; **jamais mis à l'échelle** |
| Fond | `GlassSurface` niveau `regular`, biseau 12 |
| Voile d'état (au-dessus du verre, sous le libellé) | `Rectangle` même rayon : repos transparent ; survol `fillControlHover` ; pressé `fillControlPressed` ; `ColorAnimation` `Theme.dur.fast` / `standard` (pressé : `Theme.dur.press`) |

### 6.2 États
| État | Rendu |
|---|---|
| Repos | verre + liseré + ombre verre ; reflet à sa position par défaut (haut gauche, angle −50°) |
| Survol | voile `fillControlHover` ; le reflet suit le pointeur (`HoverHandler`, positions lissées par `Behavior` `Theme.dur.moveFast` / `decelerate`) ; **seulement si `Theme.glassMotion`** |
| Pressé | le verre et le voile (pas le libellé) : échelle 1 → `glassGeo.pressScale` 1,03 en `Theme.dur.movePress` / `decelerate` ; reflet × 1,6 |
| Relâché | 1,03 → 1 en `Theme.dur.moveMedium` / `Theme.ease.spring` (≈ 3 % de dépassement) ; l'action part au relâchement (`clicked`) |
| Focus clavier | `FocusRing` (§6.4) ; aucun anneau au clic souris (`visualFocus`) |
| Opaque (transparence coupée, qualité 0) | `surfaceRaised` + contour 1 px `borderControl` + e2 ; ressort conservé (si animations actives), pas de reflet |
| Contraste élevé | `fillControl` (ButtonFace) + liseré 2 px `borderControl` (ButtonText) ; survol : liseré `accent` (Highlight) ; pressé : fond `accent` (Highlight), libellé `textOnAccent` (HighlightText) ; ni ombre ni reflet |
| Mouvement réduit | **aucune** échelle (ne pas jouer d'animation à 0 ms : ne pas changer `scale` du tout), reflet fixe ; seul le compteur change |
| Désactivé / erreur | sans objet dans ENH-004 (le contrôle n'est jamais désactivé) |

### 6.3 Animations (QML de référence)
```qml
// dans GlassButton.qml — glassLayer = Item qui contient GlassSurface + voile (pas le libellé)
ScaleAnimator { id: pressIn;  target: glassLayer; to: Theme.glassGeo.pressScale
                duration: Theme.dur.movePress;  easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.ease.decelerate }
ScaleAnimator { id: pressOut; target: glassLayer; to: 1
                duration: Theme.dur.moveMedium; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.ease.spring }
SequentialAnimation { id: pulse; ScriptAction { script: pressOut.stop() } ; ScaleAnimator { /* = pressIn */ } ; ScaleAnimator { /* = pressOut */ } }
onPressedChanged: {
    if (Theme.reduceMotion) return
    if (pressed) { pressOut.stop(); pressIn.restart() } else { pressIn.stop(); pressOut.restart() }
}
Keys.onReturnPressed: activateByKey()
Keys.onEnterPressed:  activateByKey()
function activateByKey() { if (!Theme.reduceMotion) pulse.restart(); control.clicked() }
// Espace : géré par AbstractButton (pressed tant que la touche est enfoncée, clicked au relâchement).
```
Les `ScaleAnimator` tournent sur le thread de rendu : le ressort reste à 60 i/s même si le thread principal est occupé.

### 6.4 `FocusRing.qml`
`Rectangle` transparent, `border.width: Theme.size.focusRingW` (2), `border.color: Theme.color.focusRing`, `radius: target.height / 2 + 4` (capsule), ancré sur la cible avec `anchors.margins: −(focusRingOffset + focusRingW)` = −4. Apparition : opacité 0 → 1 en `Theme.dur.fast` / `decelerate` et marge −6 → −4 en `Theme.dur.moveFast` / `decelerate`. Visible uniquement si `target.visualFocus`. **Hors** de `glassLayer` (ne gonfle pas). Contraste : `focusRing` / fond ≥ 7,2:1 (sombre), 5,2:1 (clair) ; HC : Highlight.

### 6.5 `SpringDemo.qml` (provisoire)
| Élément | Position |
|---|---|
| Capsule | centrée horizontalement dans la colonne, en haut de la rangée 4 ; texte `qsTr("Tester le ressort")` ; `onClicked: DemoCounter.increment()` (aucune logique en QML) |
| Compteur | `anchors.left: capsule.right; anchors.leftMargin: Theme.space.s4` (16), centré verticalement sur la capsule ; `Theme.typo.callout` (13 / 18) `textSecondary`, `font.features: { "tnum": 1 }` ; texte `qsTr("Essais : %1").arg(DemoCounter.count)` ; ancré à gauche, il s'allonge vers la droite sans jamais décaler la capsule |
| Changement de valeur | tout le libellé : opacité 0 → 1 en `Theme.dur.fast` / `standard` + y 4 → 0 en `Theme.dur.moveFast` / `standard` (relancé à chaque incrément) ; annonce Narrateur polie une fois : `Accessible.announce(text)` (Qt ≥ 6.8) **[H]** |
| Mention | `anchors.top: capsule.bottom; anchors.topMargin: Theme.space.s2` (8), centrée sur la capsule ; `Theme.typo.caption` (12 / 16) `textSecondary` |
| Ordre d'arbre (= ordre Narrateur) | capsule, compteur, mention |

### 6.6 Banc d'essai du verre (`--glass-testbed`)
Option de ligne de commande, désactivée par défaut, acceptée dans toutes les configurations (purement visuelle) : ajoute dans `backdropLayer`, centré sur la capsule, un rectangle 196 × 76, rayon 16, dégradé horizontal (`#F2B880`, `#D9776A`, `#7C83FF`, `#3DD68C`, `#9AD7F5`) + rayures diagonales blanches 55 % (6 px / 12 px, 115°). Seules couleurs « en dur » tolérées, dans un fichier `GlassTestbed.qml` marqué `// banc d'essai, hors design system`. Masqué en mode opaque et en contraste élevé. Il sert à la QA (§12 n° 3, 10, 12) et au contrôle design ; la démo au PO se fait sans.

---

## 7. Clavier et accessibilité
| Point | Réalisation |
|---|---|
| Focus initial | sur `contentRoot` (`focus: true`, pas `activeFocusOnTab`) : aucun anneau |
| Tab / Maj+Tab | seul arrêt : la capsule (`activeFocusOnTab: true`). Win32 ne met pas la barre de titre dans l'ordre de tabulation : avec un seul arrêt, Tab y reste, sans piège (Alt+Espace, Alt+F4, Ctrl+Q restent actifs) |
| Ctrl+Q | `Shortcut { sequences: ["Ctrl+Q"]; context: Qt.ApplicationShortcut; onActivated: AppStatus.requestQuit() }` (pas `StandardKey.Quit`, vide sous Windows) ; croix et Alt+F4 : fermeture normale |
| Noms | fenêtre « Enhance » ; capsule `Accessible.role: Accessible.Button`, nom = libellé ; illustration ignorée ; pastille « Carte graphique prête » |
| Glisser-déposer | aucune `DropArea` : curseur « interdit », aucun message |

---

## 8. Contrastes (vérifiés, maquette : bandeau « Contrastes » recalculé en direct)
| Paire | Sombre | Clair | Seuil |
|---|---|---|---|
| Libellé capsule (textPrimary) / verre sur fenêtre | ≈ 15,3 | ≈ 15,6 | 4,5 |
| textSecondary (sous-titre, compteur, mention, barre d'état) / fenêtre | 7,4 | 5,8 | 4,5 |
| Titre (textPrimary) / fenêtre | 15,6 | 15,0 | 4,5 |
| Anneau de focus / fenêtre | 7,2 | 5,2 | 3 |
| Pointillés borderStrong / fenêtre | décoratif | décoratif | — |

Le contour de la capsule (liseré de verre ≈ 2:1) n'est pas requis par WCAG 1.4.11 : le bouton est identifié par son libellé (15:1), comme un bouton discret. **Proposition à l'ux-designer et au qa-tester** : lire le critère UX n° 4 « capsule / focus ≥ 3:1 » comme « libellé ≥ 4,5:1 et anneau de focus ≥ 3:1 ». Un contour à 3:1 alourdirait le verre ; en contraste élevé, le liseré de 2 px s'applique.

---

## 9. Contrôle design de fin de story (grille que j'appliquerai)

**Entrées demandées au qa-tester** : captures PNG 1:1 de la fenêtre (zone client + barre de titre) en **sombre et clair**, à **100, 150 et 200 %** (plus 125 et 175 % pour la netteté), en **1100 × 720 et 760 × 520** ; une capture par repli (transparence coupée, contraste élevé « Aquatique », animations coupées) ; une vidéo à 60 i/s (ou trace PresentMon) de : ouverture à froid, 20 activations rapides du ressort (souris et Entrée), bascule de thème Windows, survol de la capsule ; captures avec `--glass-testbed` et `--glass-quality=0|1|2`. Je compare chaque capture à la maquette (bouton **Cotes**) à la même échelle.

| # | Point vérifié | Attendu | Écart **bloquant** si… | Écart **mineur** si… |
|---|---|---|---|---|
| 1 | Ouverture | première image complète, Mica ou fond opaque, aucune image blanche ou vide | flash blanc, fenêtre vide qui se remplit, contenu qui « saute » | animation d'ouverture système un peu différente (DWM) |
| 2 | Barre de titre | DWM standard, thème assorti, aucune couture avec le contenu (y compris opaque : `DWMWA_CAPTION_COLOR`) | barre claire sur fenêtre sombre (ou l'inverse), couture visible | — |
| 3 | Mica | Mica + `windowTint` ; ou repli opaque décidé par ADR | ni Mica ni ADR ; scintillement au redimensionnement | teinte du Mica légèrement différente de la maquette (le fond d'écran diffère) |
| 4 | Grille | 52 / cadre / 24 / 44 + 8 + 16 / 24 / 32 ; marges 24 (16 si < 900) ; colonne ≤ 928 | écart > 1 px logique, alignement différent d'une échelle à l'autre | écart de 1 px logique isolé |
| 5 | Cadre de l'état vide | r 16, 1,5 px, 6 / 6, `borderStrong` ; HC : plein 2 px | pointillés irréguliers, coins non concentriques, trait flou au repos à 125–175 % | rythme des pointillés décalé à un coin |
| 6 | Typographie | Segoe UI Variable (Display / Text / Small), tailles et interlignes du §4, `NativeRendering`, aucun texte mis à l'échelle | texte tronqué, superposé, flou au repos, mauvaise graisse, ellipse | repli sur « Segoe UI » non variable (à corriger, ticket) |
| 7 | Textes | identiques à §4.4 (U+00A0, U+00B7) | texte différent, faute, anglais, élément en plus (bouton, info-bulle, curseur de dépôt) | — |
| 8 | Capsule de verre | 44, padding 24, min 148, capsule, verre `regular`, biseau 12, liseré, ombre verre | style hors design system, texte dans le biseau, absence d'ombre ou de liseré, capsule non centrée | intensité du reflet ou du liseré ±20 %, flou de l'ombre ±25 % |
| 9 | Ressort | 1 → 1,03 (100 ms, decelerate) → 1 (250 ms, spring) ; libellé fixe ; reflet × 1,6 | absent, saccadé (image > 20 ms), > 400 ms au total, libellé qui grossit, échelle en mouvement réduit | durée ±20 ms, dépassement visiblement différent de ≈ 3 % |
| 10 | Reflet | suit le pointeur (150 ms, decelerate) au survol seulement ; aucun rendu pointeur immobile | rendu continu au repos, reflet en mouvement réduit, en opaque ou HC | taille ou forme du lobe différente |
| 11 | Focus clavier | anneau 2 px `focusRing` à 2 px, capsule, entrée 150 ms ; pas au clic | absent, visible au clic souris, < 2 px, < 3:1, rectangulaire, qui gonfle avec le verre | apparition sans le glissement 4 → 2 px |
| 12 | Compteur | callout secondaire, chiffres tabulaires, +16 à droite, fondu + 4 px, annonce une fois | la capsule bouge quand le nombre grandit, pas d'annonce ou annonce double | animation du chiffre absente |
| 13 | Barre d'état | 32, filet 1 px physique net, pastille 8 + halo, caption secondaire à 16 | filet flou ou de 2 px physiques à 125–175 %, pastille seule sans texte | halo légèrement différent |
| 14 | Bascule de thème | fondu d'une capture 250 ms, barre de titre simultanée, < 300 ms | clignotement, couleurs qui changent une à une, décalage barre / contenu, > 300 ms | — |
| 15 | Transparence coupée | fond `bgWindow`, capsule `surfaceRaised` + contour + e2, positions identiques | verre encore translucide, couture, déplacement d'un élément | ombre e2 approximative |
| 16 | Contraste élevé | couleurs système uniquement, cadre plein 2 px, capsule liseré 2 px, ni ombre ni reflet, pastille + texte | couleur de marque visible, ombre, transparence, pointillés | survol HC différent (liseré Highlight attendu) |
| 17 | Animations réduites | aucune échelle, aucun reflet mobile, ouverture et thème instantanés | une seule animation de mouvement restante | fondu du compteur absent |
| 18 | Échelles 100–200 %, changement d'écran | même mise en page, net, redessin < 1 s | flou persistant, troncature, icône floue dans la barre de titre à 150 % | icône provisoire légèrement molle à 125 % |
| 19 | Taille minimale | 760 × 520 sans défilement ni troncature, sous-titre sur 2 lignes, y compris à 200 % | défilement, élément masqué, cadre < 300 px | — |
| 20 | Fluidité / repos | 60 i/s (aucune image > 20 ms) à 100 et 200 % ; 0 image rendue au repos | saccade, rendu continu au repos | — |
| 21 | Code QML | aucune couleur, taille, durée, courbe en dur hors `Theme` (sauf `GlassTestbed.qml`) ; positions statiques arrondies au pixel | toute valeur en dur visible à l'écran | valeur en dur sans effet visible (à corriger, ticket) |

Verdict : **CONFORME** si aucun bloquant ; **ECARTS** sinon (retour au developer, compte comme un cycle). Les mineurs sont listés et acceptés ou non par l'orchestrateur.

---

## 10. Ressources tierces
Aucune : Segoe UI Variable et Cascadia Mono sont des polices système de Windows 11 (non livrées) ; illustration, icône provisoire et motif de test sont des créations de l'équipe (`docs/design/ui/icones/enhance-app-provisoire.svg`). Rien à transmettre à secdevops pour ENH-004. Lucide reste à valider avant la première icône (ENH-016).

---

## 11. Implementation (developer) : lancer l'application

- Build : `scripts\build.ps1 -Preset dev` puis `scripts\test.ps1 -Preset dev` (tests `unit` et `ui`).
- Executable : `build\dev\src\enhance.exe` (runtime Qt deploye a cote : DLL, `plugins\`, `qml\`).
- Options : `--glass-testbed` (motif sous la capsule), `--glass-quality=0|1|2` (force la qualite du verre), `--no-mica` (repli opaque force), `--check-modules` (SEC-08 : liste les modules charges hors des dossiers de confiance puis quitte ; code 0 / 3).
- Mesure du temps GPU de l'interface (QA) : `QT_LOGGING_RULES="enh.render.info=true"` et `QT_FORCE_STDERR_LOGGING=1` (une ligne par seconde de rendu).
- Decision Mica / repli : `docs/architecture/ADR-053.md`.

## 12. À retenir en C++

- **RAII sur les ressources Win32** : `WindowCloak` masque la fenetre dans son constructeur et la devoile au plus tard dans son destructeur ; `std::unique_ptr<HANDLE, HandleCloser>` ferme un jeton, `unique_ptr<wchar_t, decltype(&CoTaskMemFree)>` libere une chaine du shell. En C, il faudrait un `goto cleanup` a chaque sortie de fonction ; ici la liberation est garantie, meme en cas de retour anticipe.
- **`Microsoft::WRL::ComPtr`** (requetes D3D11 de `GpuFrameTimer`) : un pointeur intelligent qui appelle `AddRef`/`Release` tout seul, la ou le C compte les references a la main.
- **`enum class`, `std::optional`, `std::span`, `std::string_view`** (`GlassQualityGovernor`, `parseAppOptions`) : une qualite de verre ne se confond pas avec un `int`, « pas de valeur » s'exprime sans valeur sentinelle (-1), et les arguments se lisent sans copie ni `strcmp`.
- **`std::chrono` et les tests deterministes** : le gouverneur recoit l'heure en parametre (`time_point`), le test fait avancer une horloge factice de 10 s en une ligne, sans `sleep` ; en C on passerait des `uint64_t` en millisecondes sans que le compilateur verifie les unites.
