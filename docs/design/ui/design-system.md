# Enhance — Design system

Statut : v1.1 (ui-designer), 2026-10-21 — v1.1 : adoption du verre « liquide » (directive PO : « épouser le Liquid Glass 2.0 »). Sources : `docs/design/ux/` (principes, écrans, wireframes, parcours dont P-08 `SourceUnreadable` et P-08b), ADR-001 (Qt 6.8+ / Qt Quick, style Basic, Mica via DWM, `MultiEffect`, D3D11) et ADR-008 (i18n).

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
| Langage Liquid Glass d'Apple (2025 →) | **Les principes seulement** : contrôles et barres en verre qui flottent au-dessus du contenu, réfraction en « lentille » sur les bords, reflets et liseré lumineux, formes qui se transforment et fusionnent, teinte qui s'adapte au fond, hiérarchie contenu > contrôles |

- **Ambiance** : graphite neutre en sombre (par défaut), gris perle en clair ; les vignettes vidéo sont les seules taches de couleur. Le contenu (cartes, vignettes) est opaque et au centre.
- **Accent** : indigo « Iris » (`#535BE8` sombre / `#4B53E0` clair), réservé au bouton principal, au focus, à la progression et aux liens. **Un seul élément plein accent à l'écran.**
- **Matières** : Mica pour le fond de la fenêtre ; **verre « liquide »** pour tout ce qui flotte au-dessus du contenu (barre Préréglage, groupe d'actions, toast, menu, info-bulle, feuilles). Voir §6.
- **Mouvement** : 150–350 ms, courbes douces ou légèrement élastiques ; le verre se transforme (barre ↔ capsule) au lieu d'apparaître/disparaître ; trois moments soignés (dépôt en cascade, coche de fin, plus tard la comparaison avant/après), le reste reste sobre.

Inspiration, pas copie : barre de titre Windows standard (DWM), boutons de fenêtre Windows, raccourcis Ctrl, icônes Lucide, Segoe UI Variable. Aucun élément protégé d'Apple (ni SF Symbols, ni SF Pro, ni captures, ni le nom « Liquid Glass » dans l'interface ou la doc utilisateur : on parle de « verre »).

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
| glassTint | `#1E1E23` | `#F8F8FA` | Teinte du verre (opacité adaptative, §6.3) |
| glassRim / glassRimLow | blanc 22 % / blanc 6 % | blanc 85 % / noir 8 % | Liseré lumineux (côtés éclairés / côtés à l'ombre) |
| glassSpecular | blanc 55 % | blanc 90 % | Reflet (dans le biseau seulement) |
| glassShadow | noir 36 % | noir 14 % | Ombre portée du verre (0 8 28 + 0 1 3) |

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
| e0 | — | — | Barre d'état, barre d'outils |
| verre | 8 / 28 + 1 / 3 | glassShadow | Toutes les surfaces de verre |
| e1 | 1 / 2 | noir 30 % · 6 % | Carte au repos (+ contour `borderCard`) |
| e2 | 4 / 12 | noir 32 % · 10 % | Carte en cours, toast, info-bulle |
| e3 | 16 / 40 | noir 48 % · 18 % | Feuille, menu, carte soulevée pendant un glisser |

QML : `RectangularShadow` (Qt ≥ 6.9, peu coûteux) avec `offset.y`, `blur`, `color` de `Theme.elevation.eN` ; sinon `MultiEffect` (`shadowBlur = blur / 64`, `blurMax: 64`). Réglage fin à valider sur le prototype ENH-004.

---

## 6. Matières : Mica et verre « liquide »

### 6.1 Principe

Deux couches, jamais mélangées :
- **Contenu** (la liste, les cartes, les vignettes, la zone de dépôt) : opaque, au centre, posé sur le **Mica** de la fenêtre.
- **Contrôles** (ce qui agit sur le contenu) : en **verre**, ils **flottent** au-dessus du contenu, qui défile dessous et se voit au travers.

Le verre d'Enhance n'est pas un simple flou : il (1) **réfracte** le contenu près de ses bords comme une lentille épaisse (le contenu « se courbe » dans un biseau de 14–18 px), (2) le **floute** et le sature légèrement, (3) le **teinte** juste assez pour que le texte reste lisible, la teinte s'adaptant au fond pixel par pixel, (4) porte un **liseré lumineux** (bord clair en haut à gauche et en bas à droite) et un **reflet spéculaire** qui suit le pointeur, (5) **se transforme** : une barre devient capsule, deux formes proches fusionnent comme des gouttes.

### 6.2 Niveaux et usages

| Niveau | Où | Flou | Réfraction / biseau | Reflet | Teinte |
|---|---|---|---|---|---|
| **Mica** | fond de fenêtre (système, `DWMSBT_MAINWINDOW` + `windowTint`) | système | — | — | 80 % / 78 % |
| **Verre normal** | barre Préréglage flottante, groupe d'actions de la barre d'outils, toast, menu, info-bulle (sans reflet) | 24 px, sat. ×1,35 | 12 px / 14 px | 0,35 | adaptative |
| **Verre clair** | au-dessus d'images : pastille « Relâcher pour ajouter… », futur comparateur avant/après | 8 px, ×1,2 | 16 px / 16 px | 0,45 | adaptative × 0,55, voile local sous le libellé |
| **Verre épais** | feuilles (Réglages, Détails de l'erreur, Comment mettre à jour, Raccourcis, Quitter) | 40 px, ×1,2 | 8 px / 18 px | 0,25 | adaptative |

Rayons : barre 22 px (capsule 30 px quand compacte), groupe et pastilles : capsule, toast 18 px, menu 14 px, feuille 20 px.

**Règles**
1. Jamais de verre sur du verre (une feuille cache la barre ; un menu ouvert depuis une feuille est en verre épais opaque à 100 %).
2. Jamais de verre sur le contenu lui-même : les cartes restent opaques (hiérarchie contenu > contrôles).
3. Aucun texte dans le biseau : padding intérieur ≥ largeur du biseau.
4. Sur le verre : textPrimary et textSecondary seulement, jamais textTertiary.
5. Un seul élément plein accent : le bouton principal, posé dans la barre de verre, est un « verre teinté » accent opaque (capsule, reflet interne), pas une teinte translucide.
6. Toujours une ombre `shadowGlass` (0 8 28 + 0 1 3) pour décoller le verre du contenu.
7. Pas de verre tant qu'il n'y a rien dessous à montrer : la barre Préréglage est absente quand la liste est vide (inchangé).

### 6.3 Lisibilité garantie : teinte adaptative

La teinte (`glassTint` : `#1E1E23` sombre, `#F8F8FA` clair) est posée avec une opacité calculée **par pixel** dans le shader, à partir de la luminance relative *Lb* du fond flouté sous ce pixel :

`alpha = mix(A, B, smoothstep(L1, L2, Lb))`
- sombre : A 0,62 · B 0,88 · L1 0,05 · L2 0,50 (fond clair → teinte plus opaque) ;
- clair : A 0,90 · B 0,62 · L1 0,10 · L2 0,60 (fond sombre → teinte plus opaque).

Pire cas calculé sur tous les gris de 0 à 255 et des couleurs saturées (catalogue, section « Matières », recalculé en direct) : textSecondary ≥ 4,6:1 et textPrimary ≥ 9:1 en sombre, textSecondary ≥ 4,8:1 en clair. Le reflet ne touche que le biseau (sans texte). Les maquettes HTML ne savent pas mesurer le fond : elles utilisent une teinte fixe de 80 %.

### 6.4 Réalisation en Qt Quick 6.11 (RHI Direct3D 11)

| Effet | Comment | Coût GPU estimé **[H]** (RTX 5070, fenêtre 2560 × 1440) |
|---|---|---|
| **Capture du contenu sous le verre** | Une seule `ShaderEffectSource` (`sourceItem` = zone de contenu, frère *en dessous* du verre, jamais parent), `textureSize` = ½ résolution, `mipmap: true`, `hideSource: false`. Rafraîchie seulement quand le contenu change (Qt ne re-rend la source que si son sous-arbre est modifié) ; partagée par toutes les surfaces de verre (chacune lit sa zone via un `sourceRect` en uniformes) | 0,1–0,2 ms par mise à jour (passe de rendu du contenu à ½ résolution + génération des mipmaps) ; 0 ms quand rien ne bouge |
| **Flou** | Dans le shader du verre : `textureLod` sur les mipmaps (niveau ≈ log2(flou)) + 5 échantillons décalés → flou lisse sans passe séparée. Alternative si artefacts : un `MultiEffect` (blur) partagé appliqué une fois à la capture | ~0,05 ms pour la barre ; ~0,15 ms pour une feuille |
| **Réfraction (lentille)** | `ShaderEffect` + `glass.frag` compilé en `.qsb` (`qt_add_shaders`, Qt Shader Tools = outil de build, non redistribué). Distance signée au rectangle arrondi (SDF) → normale du biseau → décalage des coordonnées de texture vers l'intérieur selon un profil de lentille (`refraction` px max, `bezel` px de large) ; aberration chromatique désactivée par défaut | inclus dans la même passe (10–12 lectures de texture / pixel sur l'aire du verre uniquement) |
| **Teinte adaptative** | Même shader : luminance du pixel flouté → `alpha` (formule §6.3) → mélange avec `glassTint` | inclus |
| **Liseré lumineux** | Même shader : bande de 1 px physique le long du bord, intensité = f(normale · direction de lumière) | inclus |
| **Reflet spéculaire** | Même shader : lobe analytique (pas de texture) dans le biseau, direction de lumière en uniforme ; elle suit le pointeur (`HoverHandler`, lissage `Theme.dur.fast` / decelerate) et le défilement de la liste ; pression : intensité × 1,6 | inclus ; **aucune image rendue** tant que le pointeur ne bouge pas au-dessus du verre |
| **Fusion / morphing** | Les formes d'un même groupe (ex. barre Préréglage + bouton principal, boutons d'un groupe) passées en tableau d'uniformes (`vec4 rects[4]`, rayons) ; union lisse `smin(k = 16 px)` des SDF : les formes se rapprochent et fusionnent ; leur géométrie est animée par `NumberAnimation` (`Theme.dur.moveSlower`, `Theme.ease.spring`) | inclus ; seulement pendant la transition (350 ms) |
| **Ombre** | `RectangularShadow` (Qt ≥ 6.9) sous chaque surface | négligeable |
| **Pastille de segment « liquide »** | Échelle 1,12 × 0,94 à mi-course pendant le glissement (`SequentialAnimation` de `ScaleAnimator`) | négligeable |

Composant à créer par le developer : `GlassSurface.qml` (propriétés : `level` → `Theme.glass.regular|clear|thick`, `radius`, `shapes` pour la fusion, `backdrop` = la `ShaderEffectSource` partagée) + `glass.frag`. Si `Theme.glassQuality` vaut 0, `GlassSurface` se rend comme un simple `Rectangle` opaque (`surfaceRaised`, contour `borderControl`) : aucun shader, aucune capture.

### 6.5 Garde-fous de performance (ne jamais gêner le pipeline)

- **Budget GPU de l'interface** : ≤ 1 ms par image en moyenne, ≤ 2 ms au pire (`Theme.glassGeo.budgetGpuMs*`), mesuré par horodatage GPU du rendu Qt (`QQuickWindow::beforeRendering/afterRendering`, requêtes D3D11 timestamp) et par QA pendant un traitement CUDA/NVENC.
- **Pas de rendu continu au repos** : aucune animation en boucle hors barre de progression et préparation (indéterminée) ; reflet seulement quand le pointeur bouge ; le Mica est système (DWM), il ne coûte rien à Qt.
- **Capture mise en cache** : la `ShaderEffectSource` ne se met à jour que si le contenu change ; pendant un traitement, mises à jour **plafonnées à 10 Hz** (`live: false` + `scheduleUpdate()` sur minuterie quand le contenu est sale), capture à ½ résolution.
- **Dégradation automatique** (`RenderPolicy` en C++ → `Theme.glassQualityRequested`) : complète au repos ; pendant un traitement, si le rendu de l'interface dépasse 4 ms GPU sur 30 images → **allégée** (flou + teinte + liseré fixe ; pas de réfraction ni de reflet mobile) ; au-delà de 8 ms → **opaque**. Retour à la qualité supérieure après 10 s sous le budget. Aucun changement brusque : fondu de 150 ms.
- La liste garde un espace en bas égal à la hauteur de la barre + 32 px : la carte en cours (qui bouge à 10 Hz) n'est en général pas sous le verre.

### 6.6 Accessibilité et replis

| Réglage Windows | Effet sur le verre |
|---|---|
| « Effets de transparence » désactivé (`EnableTransparency = 0`) → `Theme.transparencyEnabled = false` | verre **opaque** (`surfaceRaised`), pas de capture, ombre e2 ; Mica remplacé par `bgWindow` |
| Contraste élevé | verre opaque aux couleurs système, liseré `WindowText` 2 px, ni reflet ni ombre |
| « Effets d'animation » désactivé | pas de morphing (fondu enchaîné 150 ms), pas de reflet qui suit le pointeur, pas d'étirement de la pastille |

Les maquettes simulent les trois réglages (boutons « Transparence », « Contraste élevé », « Réduire les animations ») ; « Réfraction » bascule l'approximation SVG.

### 6.7 Approximation dans les maquettes HTML

`backdrop-filter: url(#lg-refract) blur() saturate()` : filtre SVG `feDisplacementMap` alimenté par une carte de déplacement en dégradés (dessinée en ligne, aucune ressource externe) ; liseré = bord en dégradé masqué (`::before`) ; reflet = dégradé radial positionné sous le pointeur, confiné au biseau (`::after`) ; morphing barre ↔ capsule = transition de `width` vers `fit-content` (`interpolate-size`). Rendu fidèle dans Edge / Chrome ; ailleurs, le verre reste flouté sans réfraction.

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
| Barre Préréglage (apparition) | slow | emphasized | y +24 → 0, échelle 0,98 → 1, opacité |
| **Verre : barre ↔ capsule, fusion** | slower | spring | largeur, hauteur, rayon, facteur de fusion (formes du groupe) ; mouvement réduit : fondu enchaîné fast |
| Verre : pression | press puis medium | decelerate puis spring | échelle 1 → 1,03 → 1, reflet × 1,6 (le verre « gonfle » au lieu de s'enfoncer) |
| Verre : reflet qui suit le pointeur | fast | decelerate | direction de la lumière (uniquement au survol) |
| Pastille de segment « liquide » | medium | emphasized | étirement 1,12 × 0,94 à mi-course |
| Doublon | highlightFlash | standard | fond accentTint aller-retour |
| Info-bulle | délai 600, fast | standard | opacité |
| Changement de thème | themeFade | standard | fondu d'une capture de la fenêtre (`grabToImage`), pas d'animation couleur par couleur |

Interdits : rien ne clignote au-delà de 3 Hz, aucune boucle décorative, le fond Mica ne bouge jamais, aucune animation pendant le traitement à part la barre et ses chiffres.

### 8.3 Réduction des animations

Si « Effets d'animation » est désactivé dans Windows (`SPI_GETCLIENTAREAANIMATION`) → `Theme.reduceMotion = true` :
- toutes les durées `Theme.dur.move*` (déplacements, échelles, hauteurs) passent à **0 ms** ; les fondus (`Theme.dur.fast`, `base`…) restent ;
- squelette et barre indéterminée deviennent statiques (`Theme.dur.loopsEnabled = false`) ;
- barre de temps du toast masquée (le délai reste) ;
- verre : ni morphing, ni fusion animée, ni reflet qui suit le pointeur, ni étirement de pastille (`Theme.glassMotion = false`).

Dans les maquettes : bouton « Réduire les animations », ou préférence système `prefers-reduced-motion`.

### 8.4 Performance (60 images/s pendant un traitement GPU)

Animer de préférence `opacity`, `scale`, `x`, `y` avec les *Animators* (`OpacityAnimator`, `ScaleAnimator`, `YAnimator`), qui tournent sur le thread de rendu. Hauteurs de carte : `NumberAnimation` sur `implicitHeight`, une carte à la fois. Le verre suit le budget et la dégradation automatique du §6.5.

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
| Principal | 36 px, min 148 px, headline ; capsule dans le verre | `accent` opaque, texte blanc, ombre e1 + reflet interne haut 1 px / ombre interne bas | Démarrer (N) · Pause · Reprendre ; OK / Fermer dans une feuille |
| Dans un groupe de verre | 28 px, capsule | transparent ; survol `fillControlHover` ; pression : échelle 1,03 (gonfle) | Ajouter, Réglages (barre d'outils) |
| Secondaire | 32 px, body | `fillControl`, texte primaire | Choisir des fichiers…, Comment mettre à jour |
| Discret | 32 / 28 px | transparent, texte `accentText` (ou secondaire « neutre ») | Rétablir, Modifier, Annuler, Détails, Retirer |
| Petit (carte) | 28 px, callout, icône 16 | secondaire | Pause, Lire, Afficher dans le dossier, Réessayer |
| Icône | 32 (28) px carré | transparent, icône secondaire | Réglages, ⋯, ✕ — toujours avec nom accessible + info-bulle |

Le libellé du bouton principal change en fondu (150 ms) ; sa largeur minimale évite qu'il « saute ». En largeur étroite, les boutons de carte qui ont une icône passent en icône seule (nom accessible et info-bulle conservés).

### 9.2 Contrôle segmenté (Préréglage, Format, Apparence, Dossier de sortie)
Piste `fillControl` r 8, padding 2 ; segments **de largeur égale** ; pastille `segmentThumb` r 6 + ombre e1, glisse en 250 ms emphasized en s'étirant comme une goutte (1,12 × 0,94 à mi-course). Libellé choisi : primaire semi-gras (largeur réservée pour éviter tout décalage), autres : secondaire. Survol : libellé primaire. Pressé : opacité 0,7. Désactivé (AV1 indisponible) : libellé `textDisabled`, info-bulle « Cette carte ne sait pas produire ce format. ». Clavier : un seul arrêt de tabulation, flèches pour changer (rôle `RadioButton` / `ButtonGroup`), anneau autour de la piste. Description (caption, secondaire) sous le contrôle ; légende `HEVC` / `AV1` en tertiaire.

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
Centrée horizontalement, à 64 px du haut, largeur 520 px (moins 32 px si la fenêtre est étroite), r 20, **verre épais** + ombre verre, voile. Titre title1, sections séparées par un trait, **boutons en bas à droite** (principal à droite). Échap / clic sur le voile ferment et rendent le focus à l'élément d'origine ; focus piégé dans la feuille. Feuilles : Réglages (W5 ; réglages appliqués immédiatement, « OK » ferme), Détails de l'erreur (W6, détail technique en Cascadia Mono, copiable), Comment mettre à jour (texte local, aucun lien ouvert automatiquement), Raccourcis, Quitter pendant un traitement.

### 9.7 Barre d'état
32 px, caption secondaire. À gauche : point 8 px (vert « prête », indigo « au travail », ambre « pilote à mettre à jour ») + texte. À droite : « 3 sur 12 terminées · il reste environ 2 h 10 ».

### 9.8 Zone de dépôt (état vide)
Occupe toute la liste, r 16, pointillés 1,5 px `borderStrong` (6 / 6) dessinés en `Shape` / `ShapePath` (`strokeStyle: ShapePath.DashLine`). Illustration vectorielle originale (cadre vidéo, cadre arrière incliné, pastille flèche indigo), titre « Déposez vos vidéos ici » (title1), sous-titre body secondaire, bouton secondaire « Choisir des fichiers… ». Au survol (< 100 ms) : les pointillés se referment en trait plein accent, fond accentTint, cadres qui s'écartent, flèche qui monte, titre remplacé en fondu par « Relâcher pour ajouter N vidéos ». Avec une liste non vide : voile accent sur la liste + pastille centrale « Relâcher pour ajouter N vidéos » en **verre clair**.

### 9.9 Toast « Rétablir »
Centré au-dessus de la barre Préréglage flottante (il la suit quand elle change de taille), r 18, **verre normal**, body + bouton discret (« Annuler » / « Rétablir »), barre de temps 2 px. Ne prend pas le focus, annoncé poliment au Narrateur, pause au survol et au focus (10 s pour le Narrateur, cf. principes), équivalent Ctrl+Z. Un seul à la fois.

### 9.10 Barre Préréglage (verre flottant)
**Barre de verre normal qui flotte** à 16 px du bas de la zone de contenu, largeur de la colonne (880 + 24 px), r 22 ; la liste défile dessous (marge basse de la liste = hauteur de la barre + 32 px). Absente quand la liste est vide (apparition 300 ms). La barre **se transforme** d'un mode à l'autre (350 ms, spring) : pleine largeur en mode Réglage, **capsule** ajustée à son contenu (r 30) en modes En cours et Résumé. Trois modes :
- **Réglage** : « Préréglage » + segment + description, « Format » + segment + description · légende, à droite « Environ 2 h 14 pour l'ensemble » au-dessus de **Démarrer (N)**.
- **En cours** : une ligne « Équilibré · Compatible — appliqué aux vidéos à venir » + **Modifier** (déplie les segments) ; à droite **Pause / Reprendre**.
- **Résumé** : « 3 vidéos terminées. » (coche) ou « 2 vidéos terminées, 2 en échec. » + bouton secondaire « Tout afficher dans le dossier » / « Voir les erreurs ».
Fenêtre basse (< 600 px) : le mode Réglage se replie aussi en une ligne avec « Modifier ».

### 9.11 Menu, info-bulle, pastilles, interrupteur
Barre d'outils : titre sur le Mica à gauche ; à droite, **groupe de verre** en capsule (Ajouter, Réglages). Menu : **verre normal**, r 14, entrées 28 px r 6, survol `fillControlHover` (pas d'accent : un seul accent à l'écran), flèches / Échap. Info-bulle : délai 600 ms, verre normal sans reflet, caption. Pastilles : 20 px, badge, `fillControl` + secondaire (neutre), contour (MKV), `dangerTint` + danger (Échec). Interrupteur (vision long terme, ex. « Couleurs HDR ») : 36 × 20, bouton blanc, piste accent quand actif.

---

## 10. Pour le developer (Qt Quick)

- Module `Enhance.Design` : `Theme.qml` + `qmldir` ; avec `qt_add_qml_module`, marquer le fichier `QT_QML_SINGLETON_TYPE TRUE`. Branchement unique dans `Main.qml` : `Theme.dark`, `Theme.highContrast`, `Theme.reduceMotion`, `Theme.devicePixelRatio`, `Theme.transparencyEnabled`, `Theme.glassQualityRequested`.
- Verre : un composant `GlassSurface.qml` + un shader `glass.frag` (§6.4), une seule `ShaderEffectSource` partagée par fenêtre. `RenderPolicy` (C++, couche présentation) mesure le temps GPU de rendu et fixe la qualité ; ce n'est pas de la logique métier.
- Contrôles : style **Basic** / **Templates**, chaque contrôle restylé uniquement avec `Theme` ; aucune couleur, taille ou durée en dur dans les fichiers QML de l'app.
- Liste : `ListView` avec transitions `add` (cascade : `PauseAnimation { duration: index * Theme.dur.moveStagger }`), `remove`, `displaced`, `move` ; tout en courbes `Theme.ease.*`.
- Positions arrondies au pixel (`Math.round`) pour les éléments statiques ; jamais de demi-pixel sur un texte ou un trait.
- Maquettes = référence de contrôle design : à comparer à 100 %, 150 % et 200 %, en sombre et en clair.

---

## 11. Points ouverts

1. **Ctrl+Entrée** (conflit signalé par l'UX) : proposition — Ctrl+Entrée = toujours l'action principale ; « Afficher dans le dossier » sur une carte terminée = **Maj+Entrée**. À trancher avec ux / qa.
2. **Rétablir après annulation** : la maquette remet le fichier en tête de « À suivre » (il repartira depuis le début, le suivant a déjà démarré). À confirmer avec le gpu-engineer (P-06).
3. **Feuilles** : boutons placés en bas à droite (au lieu du « OK » en haut de W5) pour la cohérence entre toutes les feuilles.
4. **« Liquid Glass 2.0 »** : je connais le langage Liquid Glass présenté par Apple en juin 2025 (iOS / macOS 26) et ses premiers ajustements (réglage d'opacité « transparent / teinté », lisibilité renforcée). Je n'ai pas d'information fiable sur ce que le PO appelle « 2.0 » (une évolution plus récente) : la direction ci-dessus s'appuie sur les principes connus. Si le PO a des captures ou une liste de différences, je les intègre.

## 12. À prouver dans le prototype ENH-004

Chaque mesure est faite à 100 % et 200 %, fenêtre 1100 × 720 puis plein écran 4K, **au repos et pendant un traitement CUDA/NVENC réel** (720p → 4K60), sur la RTX 5070. Critère commun : 60 images/s sans saccade dans l'interface et aucune baisse > 2 % des images/s du pipeline.

| # | À prouver | Mesure | Seuil |
|---|---|---|---|
| 1 | Fenêtre Qt Quick transparente + Mica (DWM) + barre de titre sombre | visuel + temps de rendu | rendu correct, pas de scintillement au redimensionnement |
| 2 | Capture partagée (`ShaderEffectSource`, ½ résolution, mipmaps) | temps GPU par mise à jour ; nombre de mises à jour/s au repos | ≤ 0,2 ms ; **0 mise à jour au repos** |
| 3 | Shader du verre complet (réfraction + flou mip + teinte + liseré + reflet) sur la barre | temps GPU de la passe | ≤ 0,15 ms |
| 4 | Même shader sur une feuille ouverte | temps GPU | ≤ 0,3 ms |
| 5 | Fusion de 2 à 4 formes (smin) pendant le morphing barre ↔ capsule | temps GPU + fluidité | ≤ 0,2 ms, 60 i/s |
| 6 | Reflet qui suit le pointeur | nombre d'images rendues pointeur immobile | 0 |
| 7 | Total interface pendant un traitement | temps GPU par image (moyenne / pire) | ≤ 1 ms / ≤ 2 ms |
| 8 | Impact sur le pipeline | images/s du traitement avec l'app au premier plan, verre complet vs opaque | écart ≤ 2 % |
| 9 | Dégradation automatique (`RenderPolicy`) | simuler une charge GPU : bascule complète → allégée → opaque et retour | bascules sans saut visible, < 1 s |
| 10 | Contraste réel sur verre | captures sur fonds extrêmes (vignette blanche, noire, saturée) | textSecondary ≥ 4,5:1 |
| 11 | Replis Windows : transparence désactivée, contraste élevé, animations désactivées | visuel | verre opaque, couleurs système, aucun morphing |
| 12 | Netteté du texte sur le verre à 125 / 150 / 175 % | visuel | aucun flou de texte (le texte n'est jamais dans la texture capturée) |
| 13 | `Segoe UI Variable Text` sous Qt, `RectangularShadow` (Qt ≥ 6.9), palette système en contraste élevé | visuel | conforme aux maquettes |
