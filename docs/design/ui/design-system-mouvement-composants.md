# Design system — § 8 à 12 Mouvement, composants, developer, points ouverts

> Suite de [design-system.md](design-system.md) — extrait du fichier d'origine lors du ménage docs du 2026-10-29, texte inchangé.

## 8. Mouvement

### 8.1 Durées (ms) et courbes

`press` 100 · `fast` 150 · `base` 200 · `medium` 250 · `slow` 300 · `slower` 350 · `stagger` 30 · `themeFade` 300 · `highlightFlash` 400 · `tooltipDelay` 600 · `shimmer` 1400 · `indeterminate` 1600 · `toast` 5000 · `refusedMessage` 6000 · `windowAppear` 200 · `themeSwap` 250 (ces deux derniers : 0 en mouvement réduit ; `themeSwap` remplace `themeFade` pour tenir « bascule < 300 ms » capture comprise).

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
| Changement de thème | themeSwap | standard | fondu d'une capture de la fenêtre (`grabToImage`), pas d'animation couleur par couleur ; barre de titre DWM basculée dans la même itération |
| Ouverture de la fenêtre | windowAppear | decelerate | animation d'ouverture du système (DWM) ; la première image visible est déjà complète (fenêtre masquée par `DWMWA_CLOAK` jusqu'à la première image Qt) |

Interdits : rien ne clignote au-delà de 3 Hz, aucune boucle décorative, le fond Mica ne bouge jamais, aucune animation pendant le traitement à part la barre et ses chiffres.

### 8.3 Réduction des animations

Si « Effets d'animation » est désactivé dans Windows (`SPI_GETCLIENTAREAANIMATION`) → `Theme.reduceMotion = true` :
- toutes les durées `Theme.dur.move*` (déplacements, échelles, hauteurs) passent à **0 ms** ; les fondus (`Theme.dur.fast`, `base`…) restent, **sauf** l'ouverture de la fenêtre et le changement de thème (`Theme.dur.windowAppear`, `Theme.dur.themeSwap` : 0 ms) ;
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
| Verre autonome | 44 px (`glassButtonH`), marge 24 px, headline, capsule, min 148 px | verre normal (biseau effectif 12 px), libellé textPrimary ; survol voile `fillControlHover` + reflet qui suit le pointeur ; pression : 1 → 1,03 (press, decelerate) → 1 (medium, spring), reflet × 1,6 ; opaque : `surfaceRaised` + contour `borderControl` + e2 ; contraste élevé : `ButtonFace` + liseré 2 px `ButtonText` | Capsule de démonstration (ENH-004) ; futures actions flottantes isolées |
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

**Bandeau compact** (message système d'une phrase, sans action, quand il n'y a pas encore de liste — ex. SEC-20 « lancé en administrateur ») : capsule de `badgeH + s2` = 28 px, centrée dans la réserve de la barre d'outils (52 px), marge latérale `s4` 16, fond `warningTint`, contour 1 px `warning` à 30 % (2 px `WindowText` en contraste élevé), texte caption `textPrimary` sur une ligne (largeur max = colonne − 32 ; au-delà, retour à la ligne et hauteur `auto`, jamais d'ellipse). Apparition : fondu `fast` / decelerate. Le texte suit la typographie française (espace insécable U+00A0 avant « : »).

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

- Module `Enhance.Design` (dans `src/app/qml/Enhance/Design/`, à côté du module `Enhance.App` de l'ADR-036 ; détail dans `ENH-004-specs.md` §2) : `Theme.qml` + `qmldir` ; avec `qt_add_qml_module`, marquer le fichier `QT_QML_SINGLETON_TYPE TRUE`. Branchement unique dans `Main.qml` : `Theme.dark`, `Theme.highContrast`, `Theme.reduceMotion`, `Theme.devicePixelRatio`, `Theme.transparencyEnabled`, `Theme.glassQualityRequested`.
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
