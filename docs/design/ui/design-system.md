# Enhance — Design system

Statut : v1.0 (ui-designer), 2026-10-21. Sources : `docs/design/ux/` (principes, écrans, wireframes, parcours dont P-08 `SourceUnreadable` et P-08b), ADR-001 (Qt 6.8+ / Qt Quick, style Basic, Mica via DWM, `MultiEffect`, D3D11) et ADR-008 (i18n).

| Fichier | Rôle |
|---|---|
| `tokens.json` | **Source unique** des valeurs |
| `Theme.qml` + `qmldir` | Singleton QML `Enhance.Design/Theme`, que le developer importe tel quel |
| `tokens.css` | Les mêmes tokens pour les maquettes (recopiés en tête de chaque maquette pour qu'elle reste autonome) |
| `maquettes/index.html` | Catalogue animé : tokens, contrastes calculés en direct, composants et leurs états |
| `maquettes/ecran-principal.html` | Écran principal haute fidélité : 9 états, transitions, feuilles, clavier, glisser-déposer réel |

Règle : on modifie une valeur dans `tokens.json`, puis dans `Theme.qml` et `tokens.css`, **dans le même commit**. Suggestion pour le developer : un petit test unitaire qui lit `tokens.json` et compare quelques valeurs à celles de `Theme` (couleurs, durées, courbes).

---

## 1. Direction visuelle

**Trois mots : calme, précis, lumineux.**

| Référence | Ce qu'on lui emprunte |
|---|---|
| Final Cut Pro | Interface graphite qui s'efface derrière l'image ; une seule couleur pour l'action |
| Compressor | File de travaux dense mais ordonnée ; chiffres honnêtes, alignés, tabulaires |
| Pixelmator Pro | Profondeur et matières (verre, ombres douces) ; micro-animations agréables mais brèves |

- **Ambiance** : graphite neutre en sombre (par défaut), gris perle en clair ; les vignettes vidéo sont les seules taches de couleur.
- **Accent** : indigo « Iris » (`#535BE8` sombre / `#4B53E0` clair), réservé au bouton principal, au focus, à la progression et aux liens. **Un seul élément plein accent à l'écran.**
- **Matières** : Mica pour le fond de la fenêtre, verre flouté pour les feuilles, rien d'autre de translucide.
- **Mouvement** : 150–350 ms, courbes douces ou légèrement élastiques ; trois moments soignés (dépôt en cascade, coche de fin, plus tard la comparaison avant/après), le reste reste sobre.

Pas d'imitation de macOS : barre de titre Windows standard (sombre ou claire via DWM), boutons de fenêtre Windows, raccourcis Ctrl. Aucun élément protégé d'Apple (ni SF Symbols, ni SF Pro, ni captures d'écran).

---

## 2. Couleurs

Noms identiques dans les trois fichiers (`Theme.color.textSecondary` = `--c-text-secondary` = `color.dark.textSecondary`).

### 2.1 Palette

| Rôle | Sombre (défaut) | Clair | Usage |
|---|---|---|---|
| bgWindow | `#1B1B1E` | `#F2F2F5` | Fond de fenêtre opaque (repli du Mica) |
| windowTint | `rgba(27,27,30,.80)` | `rgba(242,242,245,.78)` | Teinte posée sur le Mica |
| surfaceCard | `#242428` | `#FFFFFF` | Carte de fichier |
| surfaceCardHover | `#2A2A2F` | `#F7F7F9` | Carte survolée |
| surfaceCardSelected | `#2C2D44` | `#F2F3FD` | Carte sélectionnée (+ bord accent 1 px) |
| surfaceRaised | `#2C2C31` | `#FFFFFF` | Toast, menu |
| surfaceSheet | `rgba(42,42,47,.88)` | `rgba(252,252,253,.88)` | Feuille (verre) ; repli `#2A2A2F` / `#FCFCFD` |
| surfaceTooltip | `#3A3A40` | `#FFFFFF` | Info-bulle |
| thumbPlaceholder | `#303036` | `#E4E4E9` | Vignette absente |
| textPrimary | `#F4F4F6` | `#1D1D21` | Titres, noms de fichiers, messages d'erreur |
| textSecondary | `#A9A9B2` | `#5E5E66` | Méta, descriptions, barre d'état |
| textTertiary | `#9696A0` | `#6E6E77` | Estimation, légende HEVC/AV1, étape |
| textDisabled | `#5F5F67` | `#A6A6AD` | Désactivé (exempté de contraste, WCAG 1.4.3) |
| textOnAccent | `#FFFFFF` | `#FFFFFF` | Libellé du bouton principal |
| accent / hover / pressed | `#535BE8` / `#5B63F0` / `#4A52D9` | `#4B53E0` / `#434BD6` / `#3A42C4` | Bouton principal |
| accentText | `#9AA0FF` | `#4B53E0` | Liens, boutons discrets, « Rétablir » |
| accentTint | `rgba(91,99,240,.14)` | `rgba(75,83,224,.08)` | Zone de dépôt survolée, surbrillance de doublon |
| progress | `#7178FF` | `#4B53E0` | Remplissage de la barre |
| focusRing | `#9AA0FF` | `#4B53E0` | Anneau de focus |
| success / tint / onSuccess | `#3DD68C` / 14 % / `#1B1B1E` | `#1A7F45` / 10 % / `#FFFFFF` | Coche de fin, pastille |
| warning / tint | `#F5B43C` / 14 % | `#9A5A00` / 10 % | Bandeaux de prérequis |
| danger / tint | `#FF6B6B` / 14 % | `#C9302C` / 8 % | Échec |
| borderHairline / Card / Control / Strong | blanc 8 / 6 / 12 / 24 % | noir 8 / 6 / 12 / 22 % | Séparateurs, contour de carte, contrôles, pointillés de dépôt |
| fillControl / Hover / Pressed | blanc 7 / 10 / 5 % | noir 5 / 8 / 11 % | Bouton secondaire, piste de segment |
| segmentThumb | blanc 16 % | `#FFFFFF` | Pastille du segment choisi |
| scrim | noir 40 % | noir 20 % | Voile sous une feuille |

### 2.2 Contrastes vérifiés (WCAG 2.2)

Calculés sur les valeurs opaques (couleurs translucides composées sur leur fond réel). Le catalogue les **recalcule en direct** (section « Contrastes ») et affiche OK / NON.

| Paire | Sombre | Clair | Seuil |
|---|---|---|---|
| textPrimary / bgWindow | 15,6 | 15,0 | 4,5 |
| textSecondary / bgWindow | 7,4 | 5,8 | 4,5 |
| textSecondary / surfaceCardHover | 6,1 | 6,1 | 4,5 |
| textSecondary / pastille (fillControl sur carte) | 5,4 | 5,7 | 4,5 |
| textTertiary / bgWindow | 5,9 | 4,52 | 4,5 |
| textTertiary / surfaceCard | 5,3 | 5,05 | 4,5 |
| textTertiary / surfaceCardSelected | 4,58 | 4,57 | 4,5 |
| textOnAccent / accent (repos · survol · pressé) | 5,2 · 4,7 · 5,9 | 5,8 · 6,5 · 7,6 | 4,5 |
| accentText / bgWindow | 7,2 | 5,2 | 4,5 |
| danger / pastille « Échec » | 4,52 | 4,7 | 4,5 |
| textSecondary / bandeau (warningTint) | 5,5 | 5,0 | 4,5 |
| focusRing / bgWindow · surfaceCard | 7,2 · 6,5 | 5,2 · 5,8 | 3 |
| progress / piste | 3,5 | 5,2 | 3 |
| barre en pause (textTertiary) / piste | 4,3 | 4,4 | 3 |
| accent (bord du bouton principal) / bgWindow | 3,3 | 5,2 | 3 |
| warning (icône) / bandeau | 7,1 | 4,3 | 3 |
| onSuccess (coche) / success | 9,2 | 5,0 | 3 |

Règles qui découlent de ces calculs :
- **Jamais de textTertiary** sur `fillControl`, sur une matière translucide (Mica, verre) ni sur une vignette. Minimum textSecondary.
- Le choix d'un segment est porté par la pastille **et** par le libellé (semi-gras, couleur primaire vs secondaire) : la pastille seule n'a pas 3:1 sur la piste, l'information n'en dépend donc pas.
- Le sens n'est jamais porté par la couleur seule : coche + « Terminé en… », icône + pastille « Échec » + phrase, point d'état + texte « prête ».

### 2.3 Contraste élevé Windows

Quand `SPI_GETHIGHCONTRAST` est actif (`Theme.highContrast`) : palette remplacée par les couleurs système (`Window`, `WindowText`, `Highlight`, `HighlightText`, `GrayText`, `ButtonFace`, `ButtonText`, `HotLight`), via `SystemPalette` ou `SystemAppearance` (C++) si la palette Qt ne reflète pas fidèlement `GetSysColor` (à vérifier dans ENH-004). Conséquences :
- aucune transparence (`Theme.materialsEnabled = false`), aucune ombre, pas de voile ;
- bordures de 2 px (`Theme.borderWidth`) sur cartes, boutons, barre de progression ;
- états succès / alerte / erreur en `WindowText` : le sens reste porté par l'icône et le texte ;
- segment choisi et bouton principal en `Highlight` / `HighlightText`.

Le catalogue et la maquette ont un bouton « Contraste élevé » qui simule le thème Windows « Aquatique ».

---

## 3. Typographie

**Segoe UI Variable**, police système de Windows 11 : aucun fichier à livrer, aucune licence à redistribuer, rendu ClearType/DirectWrite natif et tailles optiques (Display ≥ 20 px, Text 13–17 px, Small ≤ 12 px). **Inter (OFL)** n'est pas retenue : elle n'apporterait rien ici et ajouterait un fichier à livrer. Code et détails techniques : **Cascadia Mono** (livrée avec Windows 11), repli Consolas.

| Style | Famille | Taille / interligne | Graisse | Usage |
|---|---|---|---|---|
| display | Display | 28 / 34 (−0,2 px) | 600 | (réservé, grands états vides) |
| title1 | Display | 22 / 28 | 600 | Titre de zone de dépôt, titre de feuille |
| title2 | Text | 17 / 22 | 600 | Titre de barre d'outils « À traiter · 3 vidéos » |
| headline | Text | 14 / 20 | 600 | Nom de fichier, %, bouton principal, titre de bandeau |
| body | Text | 14 / 20 | 400 | Texte courant, boutons, message d'erreur |
| callout | Text | 13 / 18 | 400 | Méta, stats, boutons de carte |
| caption | Small | 12 / 16 | 400 | Barre d'état, descriptions de préréglage, info-bulle |
| captionStrong | Small | 12 / 16 | 600 | En-têtes de groupe (« À suivre · 2 »), libellés « Préréglage » |
| badge | Small | 11 / 14 (+0,2 px) | 600 | Pastilles |
| mono | Cascadia Mono | 12 / 18 | 400 | Détail technique copiable |

- **Chiffres tabulaires** (OpenType `tnum`) sur %, images/s, durées, tailles : `font.features: { "tnum": 1 }` en QML, `.num` dans les maquettes.
- Noms de fichiers : une ligne, **ellipse au milieu** en QML (`Text.ElideMiddle`, garde l'extension) ; nom complet en info-bulle et dans le nom accessible. Messages d'erreur : retour à la ligne, jamais d'ellipse.
- Rendu : `Text.NativeRendering` par défaut (`Theme.textRenderType`). Aucun texte n'est mis à l'échelle pendant une animation (la zone de dépôt agrandit l'illustration, pas le titre) : net à 100–200 %.
- Toutes les tailles suivent la taille de texte Windows : prévoir des hauteurs minimales, pas fixes.

---

## 4. Espacements, grille, tailles

Grille de 4 px : `s0_5` 2 · `s1` 4 · `s2` 8 · `s3` 12 · `s4` 16 · `s5` 20 · `s6` 24 · `s8` 32 · `s10` 40 · `s12` 48 · `s16` 64.

| Élément | Valeur |
|---|---|
| Fenêtre | minimum 760 × 520, défaut 1100 × 720 ; seuil « étroit » < 900 px, seuil « bas » < 600 px |
| Colonne de contenu | 880 px max, centrée ; marges latérales 24 px (16 px si étroit) |
| Barre d'outils / barre d'état / barre de titre | 52 / 32 / 32 px |
| Contrôles | 36 (bouton principal, min 148 px de large), 32 (standard, segment), 28 (dans les cartes) ; cible cliquable ≥ 32 px (28 px dans les cartes, > 24 px WCAG 2.5.8) |
| Carte | padding 12, espace entre cartes 8, hauteur min 80 ; vignette 96 × 54 (en cours 128 × 72, étroit 56 × 32) |
| Barre de progression | 4 px (6 px dans la carte en cours) |
| Icônes | 16 px (boutons, cartes), 20 px (barre d'outils, bandeaux), trait 1,75 |
| Séparateurs | 1 pixel **physique** : `Theme.hairline = 1 / devicePixelRatio` |

Alignements : le bord gauche de la vignette, du titre de la barre d'outils, des en-têtes de groupe et des contrôles du panneau tombe sur la même verticale (marge 24 px). Les actions de carte sont ancrées à 8 px du coin haut droit.

---

## 5. Rayons, ombres, élévations

Rayons : `xs` 4 · `sm` 6 (vignette, pastille de segment, info-bulle) · `md` 8 (boutons, piste de segment) · `lg` 12 (carte, bandeau, toast) · `xl` 16 (feuille, zone de dépôt) · `full` (pastilles, barre). Les rayons sont **concentriques** : piste 8 → pastille 6 avec 2 px de marge.

| Niveau | y / flou | Couleur (sombre · clair) | Usage |
|---|---|---|---|
| e0 | — | — | Barres, panneau |
| e1 | 1 / 2 | noir 30 % · 6 % | Carte au repos (+ contour `borderCard`) |
| e2 | 4 / 12 | noir 32 % · 10 % | Carte en cours, toast, info-bulle |
| e3 | 16 / 40 | noir 48 % · 18 % | Feuille, menu, carte soulevée pendant un glisser |

QML : `RectangularShadow` (Qt ≥ 6.9, peu coûteux) avec `offset.y`, `blur`, `color` de `Theme.elevation.eN` ; sinon `MultiEffect` (`shadowBlur = blur / 64`, `blurMax: 64`). Réglage fin à valider sur le prototype ENH-004.

---

## 6. Matières

| Matière | Implémentation | Repli |
|---|---|---|
| **Fenêtre (Mica)** | `DWMSBT_MAINWINDOW` + `windowTint` peint par-dessus dans la fenêtre Qt Quick transparente ; barre de titre via `DWMWA_USE_IMMERSIVE_DARK_MODE` | `bgWindow` opaque (quasi indiscernable) |
| **Feuille (verre)** | `MultiEffect` sur une capture de la zone sous la feuille : flou 32 px, saturation +0,2 ; remplissage `surfaceSheet` 88 % ; voile `scrim` | `surfaceSheetOpaque` |

Tout le reste est opaque (cartes, toast, menu, panneau Préréglage) : lisibilité garantie et coût GPU nul pendant un traitement. Le panneau Préréglage n'est **pas** translucide : la liste s'arrête au-dessus de lui, avec un fondu de 20 px en bas de la liste quand elle déborde.

---

## 7. Iconographie

**Lucide** (https://lucide.dev, dépôt `lucide-icons/lucide`), licence **ISC** (quelques icônes héritées de Feather, MIT). Grille 24, trait 1,75, bouts et angles arrondis : le dessin le plus proche de l'esprit Apple parmi les jeux libres, et assez complet pour la vision long terme. Import en SVG individuels (dans `src/ui/icons/`), affichés via `Image { sourceSize }` / `IconImage` pour un rendu net à toutes les échelles ; couleur pilotée par `Theme`.

**Non téléchargé** : à valider par secdevops. Les maquettes contiennent des tracés provisoires dessinés à la main dans le même style.

| Besoin | Icône Lucide prévue |
|---|---|
| Ajouter, Retirer, Terminé | `plus`, `x`, `check` |
| Démarrer / Lire, Pause, Réessayer / Relancer / Rétablir | `play`, `pause`, `rotate-ccw` |
| Réglages, Dossier, Dépôt | `sliders-horizontal`, `folder`, `import` |
| Alerte, Info, Échec | `triangle-alert`, `info`, `circle-alert` |
| Poignée, Plus d'actions, Déplier | `grip-vertical`, `ellipsis`, `chevron-down` |
| Copier, Monter / Descendre, Raccourcis | `copy`, `arrow-up` / `arrow-down`, `keyboard` |

Icône d'application (barre de titre, barre des tâches) : à dessiner (carré arrondi dégradé indigo + étincelle), hors périmètre de ce lot.

---

## 8. Mouvement

### 8.1 Durées (ms) et courbes

`press` 100 · `fast` 150 · `base` 200 · `medium` 250 · `slow` 300 · `slower` 350 · `stagger` 30 · `themeFade` 300 · `highlightFlash` 400 · `tooltipDelay` 600 · `shimmer` 1400 · `indeterminate` 1600 · `toast` 5000 · `refusedMessage` 6000.

| Courbe | Bézier (x1, y1, x2, y2) | Quand |
|---|---|---|
| standard | 0.4, 0, 0.2, 1 | Changement sur place (couleur, barre, chiffre) |
| decelerate | 0, 0, 0.2, 1 | Entrées : toast, bandeau, survol de dépôt, focus |
| accelerate | 0.4, 0, 1, 1 | Sorties : toast, feuille, carte retirée |
| emphasized | 0.32, 0.72, 0, 1 | Grands mouvements : feuille, déploiement de carte, pastille de segment, panneau |
| spring | 0.34, 1.30, 0.64, 1 | Ressort court (≈ 3 % de dépassement) : arrivée de carte, coche, relâchement, interrupteur |

QML : `easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.ease.<nom>` (format Qt `[x1, y1, x2, y2, 1, 1]`). CSS : `var(--ease-<nom>)`. **Valeurs identiques.**

**Ressorts physiques** (documentation et équivalences) :
| Ressort | Raideur / amortissement / masse | Taux d'amortissement | Équivalent utilisé |
|---|---|---|---|
| gentle | 300 / 30 / 1 | 0,87 | `emphasized`, 300 ms |
| snappy | 400 / 28 / 1 | 0,70 | `spring`, 250 ms |
| relâchement d'un glisser (QML seulement) | `SpringAnimation { spring: 4.0; damping: 0.40; mass: 1.0; epsilon: 0.25 }` | — | — |

Toutes les transitions passent par des courbes de Bézier + durées pour que maquette et app soient identiques ; `SpringAnimation` n'est utilisé que quand la carte relâchée doit rejoindre sa place depuis une position quelconque.

### 8.2 Ce qui s'anime, et comment

| Transition | Durée | Courbe | Propriétés |
|---|---|---|---|
| Survol | fast | standard | fond, couleur, opacité des actions de carte |
| Pression / relâchement | press / base | standard / spring | échelle 0,97 → 1 (icône : 0,94) |
| Anneau de focus | fast | decelerate | opacité, décalage 4 → 2 px |
| Pastille de segment | medium | emphasized | x |
| Interrupteur | base | spring | x du bouton |
| Survol de dépôt (entrée / sortie) | base / fast | decelerate / accelerate | pointillés qui se referment en trait plein, fond accentTint, illustration 1,02 + flèche qui monte |
| Arrivée de carte (cascade) | medium + 30 ms par carte | spring | y +12 → 0, échelle 0,98 → 1, opacité |
| Lecture du fichier | shimmer, en boucle | linear | squelette (seule boucle avec la barre indéterminée) |
| Retrait de carte | base puis medium | accelerate / standard | opacité, échelle 0,98, puis hauteur → 0 |
| Cartes écartées / réordonnées | medium | spring | y (ListView `displaced`, `move`) |
| Déploiement (en cours) | slow | emphasized | hauteur, taille de vignette |
| Fin (coche) | base puis slow | standard puis spring | barre → 100 % verte, repli, pastille 0,6 → 1, tracé de la coche, liseré accent de la vignette |
| Valeur de la barre | base | standard | largeur (mises à jour ≤ 10 Hz lissées) |
| Changement de chiffre / texte | fast | standard | fondu + y 4 px (estimation, synthèse) ; pas pour les images/s rafraîchies chaque seconde |
| Bouton principal « +1 vidéo » | press puis base | decelerate puis spring | échelle 1 → 1,04 → 1 |
| Bandeau | medium | decelerate | hauteur, opacité |
| Toast | medium / base | decelerate / accelerate | y +8 → 0, opacité ; barre de temps 5 s (pause au survol / focus) |
| Feuille (ouverture / fermeture) | slower / medium | emphasized / accelerate | opacité, y −12 → 0, échelle 0,98 → 1 ; voile en medium |
| Panneau Préréglage | slow | emphasized | apparition y +16, hauteur |
| Doublon | highlightFlash | standard | fond accentTint aller-retour |
| Info-bulle | délai 600, fast | standard | opacité |
| Changement de thème | themeFade | standard | fondu d'une capture de la fenêtre (`grabToImage`), pas d'animation couleur par couleur |

Interdits : rien ne clignote au-delà de 3 Hz, aucune boucle décorative, le fond Mica ne bouge jamais, aucune animation pendant le traitement à part la barre et ses chiffres.

### 8.3 Réduction des animations

Si « Effets d'animation » est désactivé dans Windows (`SPI_GETCLIENTAREAANIMATION`) → `Theme.reduceMotion = true` :
- toutes les durées `Theme.dur.move*` (déplacements, échelles, hauteurs) passent à **0 ms** ; les fondus (`Theme.dur.fast`, `base`…) restent ;
- squelette et barre indéterminée deviennent statiques (`Theme.dur.loopsEnabled = false`) ;
- barre de temps du toast masquée (le délai reste).

Dans les maquettes : bouton « Réduire les animations », ou préférence système `prefers-reduced-motion`.

### 8.4 Performance (60 images/s pendant un traitement GPU)

Animer de préférence `opacity`, `scale`, `x`, `y` avec les *Animators* (`OpacityAnimator`, `ScaleAnimator`, `YAnimator`), qui tournent sur le thread de rendu. Hauteurs de carte : `NumberAnimation` sur `implicitHeight`, une carte à la fois. Les effets `MultiEffect` sont limités à la feuille ouverte.

---

## 9. Composants

États communs à tout élément interactif :
| État | Rendu |
|---|---|
| Repos | voir chaque composant |
| Survol | fond `fillControlHover` (ou `accentHover`), 150 ms |
| Pressé | fond `fillControlPressed` (ou `accentPressed`) + échelle 0,97, 100 ms ; relâchement 200 ms spring |
| Focus clavier | anneau 2 px `focusRing`, décalé de 2 px, rayon = rayon de l'élément + 2 ; **seulement au clavier** (`visualFocus` dans Qt Quick Controls) |
| Désactivé | fond `fillControl`, texte `textDisabled`, aucune réaction ; info-bulle d'explication conservée (« Mettez à jour le pilote pour démarrer. ») |

### 9.1 Boutons
| Type | Taille | Repos | Usage |
|---|---|---|---|
| Principal | 36 px, min 148 px, headline | `accent`, texte blanc, ombre e1 + reflet interne 1 px | Démarrer (N) · Pause · Reprendre ; OK / Fermer dans une feuille |
| Secondaire | 32 px, body | `fillControl`, texte primaire | Choisir des fichiers…, Comment mettre à jour |
| Discret | 32 / 28 px | transparent, texte `accentText` (ou secondaire « neutre ») | Rétablir, Modifier, Annuler, Détails, Retirer |
| Petit (carte) | 28 px, callout, icône 16 | secondaire | Pause, Lire, Afficher dans le dossier, Réessayer |
| Icône | 32 (28) px carré | transparent, icône secondaire | Réglages, ⋯, ✕ — toujours avec nom accessible + info-bulle |

Le libellé du bouton principal change en fondu (150 ms) ; sa largeur minimale évite qu'il « saute ». En largeur étroite, les boutons de carte qui ont une icône passent en icône seule (nom accessible et info-bulle conservés).

### 9.2 Contrôle segmenté (Préréglage, Format, Apparence, Dossier de sortie)
Piste `fillControl` r 8, padding 2 ; segments **de largeur égale** ; pastille `segmentThumb` r 6 + ombre e1, glisse en 250 ms emphasized. Libellé choisi : primaire semi-gras (largeur réservée pour éviter tout décalage), autres : secondaire. Survol : libellé primaire. Pressé : opacité 0,7. Désactivé (AV1 indisponible) : libellé `textDisabled`, info-bulle « Cette carte ne sait pas produire ce format. ». Clavier : un seul arrêt de tabulation, flèches pour changer (rôle `RadioButton` / `ButtonGroup`), anneau autour de la piste. Description (caption, secondaire) sous le contrôle ; légende `HEVC` / `AV1` en tertiaire.

### 9.3 Carte de fichier — 7 états
Structure unique : vignette 16:9 à gauche, nom (headline), puis des sections qui s'ouvrent / se ferment en hauteur (300 ms emphasized). Actions d'en-tête (⋯, ✕) visibles au survol, à la sélection ou au focus. Toute la carte « En attente » se glisse (la poignée n'est qu'un indice visuel) ; au clavier : Alt+↑/↓.

| État | Rendu | Actions |
|---|---|---|
| Lecture | vignette et 2 lignes en squelette animé, nom visible | ✕ |
| En attente | méta (callout secondaire) ; pastille « 25 → 60 images/s » ou « 60 images/s conservées » ; « ≈ 14 min » à droite (tertiaire) ; pastille « MKV » + ligne « Enregistré en MKV pour conserver les sous-titres. » ; poignée au survol | ⋯ (placer en tête / en fin, afficher l'original, retirer), ✕ |
| En cours | ombre e2, vignette 128 × 72, barre 6 px, « 38 % » (headline) + « 52 images/s · il reste environ 9 min », « Étape : Agrandissement en 4K » (tertiaire) ; « Estimation en cours… » les 10 premières secondes | Pause, Annuler |
| En cours — 1re utilisation (P-08b) | « Préparation de la fluidité… », sous-texte « Première utilisation seulement. Cela prend 1 à 3 minutes, puis ce sera instantané. » (après 5 min : « Cela prend plus de temps que prévu. Enhance travaille toujours. ») ; **barre indéterminée**, ni %, ni images/s, ni temps restant ; barre d'état « Préparation de la première utilisation… » | Annuler → retour « En attente » avec « Préparation interrompue. Elle reprendra au prochain lancement. » |
| En pause | barre figée grise, vignette en niveaux de gris 60 %, « En pause à 38 % » · « Reprise immédiate » (la mémoire graphique reste réservée pendant la pause, décision PO n° 8) | Reprendre, Annuler |
| Terminé | coche verte qui se dessine dans une pastille au coin de la vignette, liseré accent autour de la vignette, « Terminé en 14 min · 4K 60 i/s · 2,3 Gio », « Votre fichier d'origine n'a pas été modifié. » | Lire, Afficher dans le dossier, ✕ |
| Échec | contour danger 40 %, pastille « ! » au coin de la vignette, pastille « Échec », phrase complète en body primaire (retour à la ligne) | Réessayer, Détails, Retirer ; `SourceUnreadable` : + « Afficher dans l'Explorateur » |
| Annulé | vignette grise ; 5 s : « Traitement annulé. » + **Rétablir** + compte à rebours ; puis « Annulé. Le fichier partiel a été supprimé. » | Rétablir (5 s, Ctrl+Z) puis Relancer, ✕ |

Survol : `surfaceCardHover`. Sélection : `surfaceCardSelected` + contour accent 1 px. Focus : anneau. Doublon déposé : surbrillance 400 ms.
Groupes dans la liste (seulement s'il y en a plusieurs) : « En cours », « À suivre · N », « Terminées · N » (échecs en tête), en captionStrong secondaire.

Texte `SourceUnreadable` (P-08) : « Enhance n'arrive pas à ouvrir ce fichier. Il est peut-être utilisé par une autre application, ou vous n'avez pas le droit de le lire. Les autres vidéos continuent. » ; détail replié : chemin + code Windows.

### 9.4 Barre de progression
Piste `fillControl`, remplissage `progress`, rayon plein. Variantes : en cours (6 px), en pause (`textTertiary`), terminée (`success`), indéterminée (segment de 32 % qui balaie en 1,6 s ; statique en mouvement réduit), squelette.

### 9.5 Bandeau
Haut de la liste, r 12, fond `warningTint` (ou `dangerTint`, `accentTint`) + contour de la même teinte à 30 %, icône 20 px, titre headline + texte callout secondaire, une action secondaire à droite. Persistant, jamais bloquant ; entrée 250 ms. Cas : pilote trop ancien, carte non compatible, aucune carte NVIDIA, composant manquant, espace disque, (plus tard) reprise après plantage.

### 9.6 Feuille
Centrée horizontalement, à 64 px du haut, largeur 520 px (moins 32 px si la fenêtre est étroite), r 16, verre + e3, voile. Titre title1, sections séparées par un trait, **boutons en bas à droite** (principal à droite). Échap / clic sur le voile ferment et rendent le focus à l'élément d'origine ; focus piégé dans la feuille. Feuilles : Réglages (W5 ; réglages appliqués immédiatement, « OK » ferme), Détails de l'erreur (W6, détail technique en Cascadia Mono, copiable), Comment mettre à jour (texte local, aucun lien ouvert automatiquement), Raccourcis, Quitter pendant un traitement.

### 9.7 Barre d'état
32 px, caption secondaire. À gauche : point 8 px (vert « prête », indigo « au travail », ambre « pilote à mettre à jour ») + texte. À droite : « 3 sur 12 terminées · il reste environ 2 h 10 ».

### 9.8 Zone de dépôt (état vide)
Occupe toute la liste, r 16, pointillés 1,5 px `borderStrong` (6 / 6) dessinés en `Shape` / `ShapePath` (`strokeStyle: ShapePath.DashLine`). Illustration vectorielle originale (cadre vidéo, cadre arrière incliné, pastille flèche indigo), titre « Déposez vos vidéos ici » (title1), sous-titre body secondaire, bouton secondaire « Choisir des fichiers… ». Au survol (< 100 ms) : les pointillés se referment en trait plein accent, fond accentTint, cadres qui s'écartent, flèche qui monte, titre remplacé en fondu par « Relâcher pour ajouter N vidéos ». Avec une liste non vide : voile accent sur la liste + pastille centrale « Relâcher pour ajouter N vidéos ».

### 9.9 Toast « Rétablir »
Centré au-dessus du panneau, r 12, `surfaceRaised` + e2, body + bouton discret (« Annuler » / « Rétablir »), barre de temps 2 px. Ne prend pas le focus, annoncé poliment au Narrateur, pause au survol et au focus (10 s pour le Narrateur, cf. principes), équivalent Ctrl+Z. Un seul à la fois.

### 9.10 Panneau Préréglage
Collé en bas, séparé par un trait 1 px, sur le fond de fenêtre (pas de carte). Absent quand la liste est vide (apparition 300 ms). Trois modes :
- **Réglage** : « Préréglage » + segment + description, « Format » + segment + description · légende, à droite « Environ 2 h 14 pour l'ensemble » au-dessus de **Démarrer (N)**.
- **En cours** : une ligne « Équilibré · Compatible — appliqué aux vidéos à venir » + **Modifier** (déplie les segments) ; à droite **Pause / Reprendre**.
- **Résumé** : « 3 vidéos terminées. » (coche) ou « 2 vidéos terminées, 2 en échec. » + bouton secondaire « Tout afficher dans le dossier » / « Voir les erreurs ».
Fenêtre basse (< 600 px) : le mode Réglage se replie aussi en une ligne avec « Modifier ».

### 9.11 Menu, info-bulle, pastilles, interrupteur
Menu : `surfaceRaised` + e3, r 10, entrées 28 px r 6, survol `fillControlHover` (pas d'accent : un seul accent à l'écran), flèches / Échap. Info-bulle : délai 600 ms, `surfaceTooltip`, caption. Pastilles : 20 px, badge, `fillControl` + secondaire (neutre), contour (MKV), `dangerTint` + danger (Échec). Interrupteur (vision long terme, ex. « Couleurs HDR ») : 36 × 20, bouton blanc, piste accent quand actif.

---

## 10. Pour le developer (Qt Quick)

- Module `Enhance.Design` : `Theme.qml` + `qmldir` ; avec `qt_add_qml_module`, marquer le fichier `QT_QML_SINGLETON_TYPE TRUE`. Branchement unique dans `Main.qml` : `Theme.dark`, `Theme.highContrast`, `Theme.reduceMotion`, `Theme.devicePixelRatio`.
- Contrôles : style **Basic** / **Templates**, chaque contrôle restylé uniquement avec `Theme` ; aucune couleur, taille ou durée en dur dans les fichiers QML de l'app.
- Liste : `ListView` avec transitions `add` (cascade : `PauseAnimation { duration: index * Theme.dur.moveStagger }`), `remove`, `displaced`, `move` ; tout en courbes `Theme.ease.*`.
- Positions arrondies au pixel (`Math.round`) pour les éléments statiques ; jamais de demi-pixel sur un texte ou un trait.
- Maquettes = référence de contrôle design : à comparer à 100 %, 150 % et 200 %, en sombre et en clair.

---

## 11. Points ouverts

1. **Ctrl+Entrée** (conflit signalé par l'UX) : proposition — Ctrl+Entrée = toujours l'action principale ; « Afficher dans le dossier » sur une carte terminée = **Maj+Entrée**. À trancher avec ux / qa.
2. **Rétablir après annulation** : la maquette remet le fichier en tête de « À suivre » (il repartira depuis le début, le suivant a déjà démarré). À confirmer avec le gpu-engineer (P-06).
3. **Feuilles** : boutons placés en bas à droite (au lieu du « OK » en haut de W5) pour la cohérence entre toutes les feuilles.
4. Prototype ENH-004 : vérifier Mica + fenêtre Qt Quick transparente, nom de famille « Segoe UI Variable Text » sous Qt, `RectangularShadow` (Qt ≥ 6.9) et palette système en contraste élevé.
