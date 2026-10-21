# Checklist UI v1 — ENH-004 (squelette Qt Quick + Mica + verre)

Auteur : qa-tester, 2026-10-21. Statut : v1, à exécuter sur la RTX 5070, build Release. Références : `docs/qa/protocole.md` §9, `docs/design/ui/design-system.md` §12, `docs/qa/sprint-1-seuils.md`.
Légende : A = automatisable (outil/script), M = manuel par le PO ou le QA, **PO** = seul le PO peut le faire/juger. Résultat : `[ ]` à faire, `[x]` OK, `[!]` KO (bug noté), `n/v` non vérifié. Captures dans `docs/qa/resultats/ENH-004/captures/` (sans nom d'utilisateur ni chemin visibles), nommées `<etat>_<100|150|200>_<sombre|clair>.png`.

Session : date ____ ; pilote ____ ; Windows ____ ; build (hash git) ____ ; Qt ____ ; écran ____.

## 1. Démarrage et intégrité

- [ ] A Démarrage à froid < 2 s (de `Start-Process` à la première image affichée ; 5 essais, médiane).
- [ ] A Aucun `Qt6Network.dll` dans le dossier de l'exe (`Get-ChildItem -Recurse -Filter Qt6Network*`) ni dans les modules chargés (`Get-Process enhance | % Modules`).
- [ ] A Aucun module chargé hors dossier de l'app ou de `System32` (SEC-08/SEC-20).
- [ ] A Aucune connexion réseau pendant 60 s d'utilisation (`Get-NetTCPConnection -OwningProcess`, `netstat -ano`).
- [ ] M Titre de la fenêtre : « Enhance » ; texte entièrement en français, aucune fuite de clé `qsTr`.

## 2. Rendu : échelle, thème, matière (captures de chaque combinaison)

Pour chaque échelle **100 %, 150 %, 200 %** (Paramètres > Affichage ; déconnexion si demandé) × **sombre, clair** (Paramètres > Personnalisation > Couleurs ; test aussi la bascule **à chaud**, app ouverte) :
- [ ] M **PO** Texte net (pas de flou), pas de coupure ni de chevauchement, icônes nettes, coins arrondis et ombres homogènes.
- [ ] M Fond Mica visible (teinte du bureau) ou repli opaque teinté si Mica indisponible ; barre de titre sombre en thème sombre, claire en thème clair.
- [ ] M Redimensionnement de la fenêtre (1100 × 720 → plein écran 4K) sans scintillement ni bande blanche/noire.
- [ ] M Déplacement entre deux écrans de mise à l'échelle différente : rendu et taille corrects.
- [ ] M Contraste : texte principal ≥ 4,5:1, texte secondaire ≥ 4,5:1 **sur verre, fonds extrêmes** (blanc, noir, saturé) ; composants ≥ 3:1 (pipette ou Colour Contrast Analyser).

## 3. Réglages Windows et replis

- [ ] **PO** Effets de transparence **désactivés** (Paramètres > Accessibilité > Effets visuels) : fond **opaque** teinté, verre devenu opaque, aucun artefact ; réactivation à chaud : retour au verre sans redémarrer.
- [ ] **PO** Thème de contraste élevé (ex. Crépuscule) : couleurs système, contours visibles, tout lisible, aucun morphing.
- [ ] **PO** Effets d'animation désactivés : aucune animation de ressort/morphing ; changements d'état instantanés mais corrects.
- [ ] M Mode économie d'énergie / GPU occupé : bascule `RenderPolicy` complet → allégé → opaque et retour en < 1 s, sans saut visible (design-system §12 n° 9).

## 4. Clavier, focus, accessibilité

- [ ] M Tab / Maj+Tab : ordre logique, aucun piège, tous les contrôles atteignables.
- [ ] M Anneau de focus visible sur chaque contrôle, thème sombre et clair, y compris sur verre.
- [ ] M Entrée / Espace activent le contrôle ; Échap ferme/annule ; raccourcis documentés fonctionnent ; Ctrl+Entrée / Maj+Entrée selon décision du point ouvert n° 1 du design-system.
- [ ] **PO** Narrateur (Win+Ctrl+Entrée) : annonce le **nom de la fenêtre** (« Enhance »), puis, pour le contrôle de démonstration, **nom + rôle + état** (ex. « Ajouter des vidéos, bouton ») ; la propriété affichée (`Q_PROPERTY`) est lue ; aucun contrôle sans nom ; ordre de lecture logique.
- [ ] M Inspecteur (Accessibility Insights ou `Inspect.exe`) : `Name`, `ControlType`, `IsKeyboardFocusable` renseignés sur la fenêtre et le contrôle.
- [ ] M Fenêtre déplaçable / minimisable / maximisable / fermable au clavier (Alt+Espace) et à la souris ; boutons système corrects.

## 5. Fluidité : 60 images/s (A)

Outil : `QSG_RENDER_TIMING=1` (journal par image : polish, sync, render, swap) et `QSG_VISUALIZE=overdraw` pour le sur-dessin ; en complément PresentMon (`PresentMon --process_name enhance.exe --output_file ...`) pour la cadence réelle de présentation et `QT_LOGGING_RULES="qt.scenegraph.time.renderloop=true"`. Build de test avec compteur de délai de la boucle d'événements si disponible.
- [ ] A **Repos**, animation ressort (action QML → C++ ) : ≥ 59 i/s en moyenne, 99e centile du temps d'image ≤ 16,7 ms ; au plus 0,1 % d'images > 33 ms.
- [ ] A **Pointeur immobile** : 0 image rendue (reflet suit le pointeur seulement quand il bouge).
- [ ] A **Au repos sans animation** : 0 mise à jour de la capture partagée (`ShaderEffectSource`).
- [ ] A Fil d'interface jamais bloqué > 16 ms pendant l'ouverture/fermeture d'une feuille (si présente) et le redimensionnement.
- [ ] A Temps GPU de l'interface (`QSG_RENDER_TIMING` ou outil GPU : PIX / Nsight Graphics / `nvidia-smi dmon`) : ≤ 1 ms moyen, ≤ 2 ms pire (design-system §12 n° 7) ; capture partagée ≤ 0,2 ms ; verre barre ≤ 0,15 ms ; verre feuille ≤ 0,3 ms ; morphing ≤ 0,2 ms.

## 6. Coût GPU du verre (A)

Charge GPU **simulée** (pas besoin de pipeline) : `spike_bench` ou `enhance-bench` en boucle VSR 720p → 4K q4 (≈ 320 i/s, GPU > 90 %), puis, quand ENH-008/040/047 sont mergées, un vrai traitement 720p → 4K60. Fenêtre au premier plan, 1100 × 720 puis plein écran 4K, 100 % puis 200 %.
- [ ] A **Au repos (aucune charge)** : utilisation GPU de l'app (`nvidia-smi pmon -c 30` ou Gestionnaire des tâches > GPU) ≈ 0 % pointeur immobile ; relever la valeur avec verre complet et avec transparence coupée (écart).
- [ ] A **Pendant la charge** : i/s du traitement, **verre complet** vs **opaque** (transparence Windows coupée ou `RenderPolicy` opaque), 7 répétitions alternées, médiane ; **écart ≤ 2 %** (design-system §12 n° 8). (max − min)/médiane de chaque série ≤ 2 %, sinon « non concluant ».
- [ ] A Pendant la charge, interface à 60 i/s : même seuil que §5, sans saccade visible ; relever l'effet sur le temps d'image de l'UI.
- [ ] A VRAM de l'app (pic − repos) notée avec verre complet et opaque.
- [ ] M **PO** Pendant la charge : animation du verre et du ressort fluide à l'œil (aucune saccade visible).

## 7. États minimaux du squelette (captures)

- [ ] M État initial (fenêtre vide / état vide de la maquette), contrôle de démonstration survolé, pressé, désactivé, focalisé.
- [ ] M Fenêtre inactive (perte de focus) : barre de titre et matière corrects.
- [ ] M Fermeture propre : le processus se termine (< 2 s), aucun processus orphelin.

## 8. Sorties à transmettre

Captures (`captures/`) → ui-designer pour le contrôle design ; mesures (CSV/JSON) dans `docs/qa/resultats/ENH-004/` ; bugs au format du verdict QA (bloquant/mineur).

Critères d'échec bloquants : plantage, texte illisible (< 4,5:1), absence de focus visible, Narrateur muet sur le contrôle, `Qt6Network.dll` présent, connexion réseau, UI < 60 i/s hors dégradation volontaire, écart pipeline verre/opaque > 2 % (mesure concluante), repli transparence coupée non opaque.
