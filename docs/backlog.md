# Enhance — Backlog produit

Version : v0.3 du 2026-10-21 (scrum-master, étape 7 du kickoff). Statut : v0.2 validée par le PO le 2026-10-21 ; **v0.3 à relire par le PO** (nouvelles stories, seuils QA, statuts du kickoff).
Les chiffres des critères `[GPU]` sont désormais **ancrés sur les mesures du spike** (`docs/gpu/spike.md`, phases 2 et 3) ; ceux qui restent des hypothèses sont marqués **[H]**.
Clips de référence : `BBB-FAST`, `BBB-PAN`, `TOS-FACE`, `TOS-DARK`, `SYN-TEXT` (`docs/qa/clips.md`) ; « REF-A » = `BBB-FAST` en jeu A tant que le PO n'a pas désigné un clip unique. « Sur la 5070 » = RTX 5070 12 Gio du PO, pilote 617.42.

## Plan de release

| Indicateur | Valeur |
|---|---|
| Total points MVP | **174** (126 en v0.2 ; +44 pts de stories nouvelles issues du spike, du threat model et de la revue des licences ; +1 net de ré-estimations, voir « Mouvements v0.3 » ; **+3 pts : ENH-008b**, redécoupage d'ENH-008 au planning du Sprint 1, voir « Mouvements v0.4 ») |
| Points livrés (kickoff = « sprint 0 », non compté dans la vitesse) | **33** : kickoff 23 (ENH-001 (5), ENH-002 (5), ENH-003 (4 sur 5), ENH-005 (3), ENH-006 (3), ENH-007 (3)) + **ENH-008 (5, DONE le 2026-10-23, merge `0c2f748`)** + **ENH-004 (5, DONE, merge `79a3e93`)** ; Sprint 1 : 10 / 15 |
| Points restants | **141** (dont 5 pts encore engagés au Sprint 1, **démarré le 2026-10-21** : ENH-040 en revue (cycle 1), ENH-047 TODO) |
| Vitesse supposée | **15 pts / sprint** (aucun sprint terminé : pas d'historique ; le kickoff n'est pas un sprint) |
| Sprints restants estimés avant le MVP | **10** (141 / 15 = 9,4 ; hors Sprint 1 en cours) ; fourchette **10 à 12** si deux stories `[UI]` passent à 3 cycles de revue (le risque Mica est levé : ENH-004 livrée) |
| Dernière mise à jour | 2026-10-23 (ENH-004 et ENH-008 DONE ; Sprint 1 en cours, aucun sprint terminé) |

Répartition des 141 points restants : Fondations, sécurité et CI 8 (ENH-049, 050), Pipeline GPU 68 (dont ENH-008b), File / préréglages / UI / transverse 65.
Total « Plus tard » : 52 pts + 8 pts (ENH-051) ; « Avant publication » : 3 pts (hors MVP).

### Jalons (indicatifs, à recaler à chaque fin de sprint)

| Jalon | Contenu | Sprint visé |
|---|---|---|
| J0 | Kickoff : spikes VSR / interpolation, protocole QA, threat model, licences, design system, CI locale (**fait**) | 0 |
| J1 | **Fenêtre Enhance** qui s'ouvre (Qt Quick, sombre, Mica), **décodage NVDEC** mesuré, **pont RTX Video** en production, `enhance-bench` v1 (agrandir une image en 4K) | 1 |
| J2 | **Premier fichier 4K de bout en bout** : décodage -> super-résolution -> NVENC HEVC 10 bits, i/s et VRAM mesurés par la QA | 2 |
| J3 | **Vrai 4K60** : RIFE (export maison, moteur TensorRT, coupures de plan) intégré, AV1, budget VRAM, audio / sous-titres | 4-5 |
| J4 | **L'application complète côté utilisateur** : file, glisser-déposer, préréglages, progression, pause / annulation, thèmes | 7-8 |
| J5 | **MVP** : robustesse (fuzzing), zéro réseau prouvé, FFmpeg minimal sans réseau, paquet portable, À propos / CLUF | 10 |

Ordre de bataille : voir `docs/sprints/sprint-1.md` (Sprint 1 = 15 pts : ENH-004, ENH-008, ENH-040, ENH-047). Sprint 2 pressenti (15 pts) : ENH-042, ENH-009, ENH-010. ENH-008b (3 pts) : avant ENH-048, au plus tard Sprint 3. Sprints 3-4 : RIFE (ENH-043, ENH-044, ENH-012, ENH-046), AV1, file (ENH-016), journal (ENH-025), NGX hors ligne (ENH-041).

### Mouvements v0.4 (planning du Sprint 1, 2026-10-21)

- **ENH-008 reste à 5 pts (Sprint 1)** mais son périmètre de matrice est réduit à la matrice « noyau » (8 combinaisons + ProRes en repli logiciel + clip QA `BBB-FAST`) : estimation du gpu-engineer = 8 pts pour le périmètre complet (`docs/gpu/ENH-008.md` § 8).
- **ENH-008b créée (3 pts, Must)** : reste de la matrice valide, 8 / 10 bits, VFR, rotation, dimensions impaires. À livrer **avant ENH-048** (qui rejoue la matrice) ; Sprint 2 reste à 15 pts, ENH-008b est placée au plus tard au Sprint 3. **Total MVP +3 pts.**
- Critères UX et seuils QA du Sprint 1 intégrés à ENH-004, 008, 008b, 040, 047 (`docs/design/ux/ENH-004.md`, `docs/qa/sprint-1-seuils.md`, `docs/qa/checklist-ui.md`).

### Mouvements v0.3 (traçabilité)

- ENH-009 8 -> 5 et ENH-012 8 -> 5 : le pont, les conversions couleur, le moteur TensorRT et la détection de coupure sont sortis en stories dédiées.
- ENH-015 3 -> 5 (limites, fuzzing, SEC-09, SEC-13), ENH-022 3 -> 5 (persistance durcie SEC-10/11/18), ENH-024 2 -> 3 (SEC-11), ENH-028 2 -> 3 (test dynamique, SEC-22), ENH-030 3 -> 5 (SEC-02/04/16, `package.ps1`).
- ENH-003 5 -> 4 pts : le reliquat (CI GitHub non exécutée, durcissements devops) est porté par ENH-049.
- ENH-001, ENH-002, ENH-005, ENH-006, ENH-007 : DONE ; leurs critères non remplis sont **transférés** à une story nommée (aucun critère perdu).

## Décisions du PO

Décisions n° 1 à 15 : **2026-10-21** (validation de la vision `vision.md`, de ce backlog v0.2, de la stack ADR-001 Qt 6 / Qt Quick et de la direction visuelle). Décisions n° 16 à 19 : **2026-10-23**. Numérotation continue, sans doublon ; les renvois « décision n° N » des autres documents restent valables.

1. **Cadence** : on n'interpole jamais au-delà de 60 i/s ; une source >= 50 i/s n'est pas interpolée ; 24 i/s -> vrai 60 i/s interpolé (ENH-012). **Précision** : toute source strictement sous 50 i/s (y compris 31-49 i/s) est interpolée vers 60 i/s.
2. **Conteneur MVP** : MP4 ; bascule automatique en MKV, avec message, si une piste de sous-titres est incompatible MP4 (ENH-013, ENH-024).
3. **Clips de référence** : clips libres (Blender, Xiph) + 1 ou 2 extraits des archives du PO, jamais commités (ENH-005, ENH-029).
4. **Publication** : gratuite, code privé pour l'instant ; FFmpeg LGPL en liaison dynamique (ENH-006, ENH-030).
5. **Si la 5070 ne tient pas 60 i/s** : qualité d'abord, vitesse mesurée et affichée ; interpolation dégradée seulement dans « Rapide » (ENH-002, ENH-012, ENH-018, ENH-020). *Le spike montre que la 5070 tient largement 60 i/s en Équilibré (103-112 i/s) ; la clause reste valable pour « Qualité max » (36,5 i/s en HEVC).*
6. **Quitter pendant un traitement** : seule exception à « pas de confirmation ». La croix de la fenêtre réduit l'app dans la barre des tâches sans arrêter le traitement ; Ctrl+Q ouvre une petite feuille « Quitter et abandonner le traitement en cours ? » (ENH-021).
7. **Toolchain** : Visual Studio 2026 (MSVC v145) remplace VS 2022 ; CUDA Toolkit 13.4 accepte ce compilateur hôte **sans** `-allow-unsupported-compiler` (confirmé par le spike).
8. **Pause** : la VRAM reste réservée pendant une pause pour une reprise instantanée (MVP) ; libérer la carte pendant une pause viendra avec la reprise après plantage (ENH-021, ENH-038).
9. **Sous-titres** : SRT/WebVTT convertis en `mov_text` pour rester en MP4 ; bascule MKV seulement pour ASS/PGS/VobSub, pièces jointes ou audio non MP4 (ENH-013).
10. **Préparation des modèles d'interpolation** (construction du moteur TensorRT) : au premier traitement, avec un message affiché à l'utilisateur, pas à l'installation ; mis en cache ensuite (ENH-044). *Mesuré : ~85 s à vide, jusqu'à 2,5 min si le PC est chargé.*
11. **Direction visuelle validée** : maquettes `docs/design/ui/maquettes/`, accent **Iris**, fond **Mica + verre**, police **Segoe UI Variable** (système, non livrée), icônes **Lucide (ISC)** (`design-system.md`). La ligne « Jeu d'icônes » du registre des licences est à passer de « À CONFIRMER » à « OK » par secdevops (tâche du Sprint 1).
12. **Outil local Python** : **Python 3.12 + PyTorch (roues CPU suffisantes)** approuvé **comme outil local** pour exporter RIFE en ONNX depuis les poids **MIT** de Practical-RIFE. **Export maison obligatoire** ; l'ONNX de vs-mlrt (dépôt GPL-3.0) est **écarté de la livraison** (reste local, comparaison). Le PO a relu la licence MIT des poids et **accepte le risque résiduel Vimeo-90K** (SEC-03, `licences.md` §7.2). Rien de Python n'est livré (ENH-043).
13. **Comparaison des modèles d'interpolation** : RIFE 4.25 comparé à FILM, IFRNet et EMA-VFI (qualité, vitesse 720p TensorRT, licence des poids) pour départager « Qualité max » et disposer d'un repli licence (ENH-045).
14. **Seuils de qualité en part de la marge restante** (remplace « +5 VMAF ») : écart VMAF vs Lanczos >= **50 %** de (100 - VMAF Lanczos) en **Qualité max**, >= **35 %** en **Équilibré**, >= **20 %** en **Rapide** (voir « Seuils QA de référence » ci-dessous). Raison : les sources libres sont déjà douces, Lanczos y atteint 92-96 de VMAF (`docs/qa/resultats/2026-10-21_reference-lanczos.md`), +5 y est arithmétiquement impossible.
15. **Git** : aucune mention de Claude dans les commits ni les PR ; aucun push sans l'accord du PO.
16. **Test UNC d'ENH-008 reporté à ENH-015** (décision du 2026-10-23) : `Win32File.PartageUnc` (`ENH_TEST_UNC_DIR`), non exécuté faute de partage accessible, est reporté à ENH-015 ; le point UNC du critère AVIO d'ENH-008 est « non vérifié — reporté à ENH-015 ».
17. **Barre de titre et accentuation Windows** (2026-10-23) : la barre de titre suit la couleur d'accentuation Windows choisie par l'utilisateur (`ColorPrevalence`).
18. **Opacité du verre sur fond vif** (2026-10-23) : réglée à ENH-016 sur du vrai contenu ; piste : teinte allégée dans le biseau.
19. **Aucune vraie frappe clavier / clic sur le poste du PO** (2026-10-23) : aucun agent n'envoie de vraies frappes clavier ni de clics sur le poste du PO (incident QA d'ENH-004) ; les tests clavier sont automatisés ou faits par le PO.

## Faits techniques établis par le kickoff (reflétés dans les stories)

| Sujet | Fait | Où dans le backlog |
|---|---|---|
| Chaîne mesurée (RIFE mixte FP16/FP32 720p -> VSR -> NVENC, 4K60) | Équilibré **103-112 i/s** (HEVC 108 / AV1 111-112), Qualité max **36,5 i/s (HEVC) / 51,5 (AV1)**, Rapide **219 i/s** ; VRAM pic **1,5-2,1 Gio** ; **NVENC est le goulot** (un seul moteur) ; 0 Mio de dérive sur 10 jobs | ENH-009, 010, 011, 012, 014 |
| Interpolation | **RIFE 4.25 > FRUC** (+2 à +15 VMAF ; FRUC abandonne dès ±2 % de grain et fait pire que la duplication sur le texte) ; **FRUC = repli local non livrable** (SEC-04) ; RIFE 4K après SR écarté (32 i/s, 3,9 Gio) | ENH-012, 043, 044, 045 |
| RTX Video SDK 1.1.0 | **Entrée RGB seulement** (conversions YUV <-> RGB à notre charge), 10 bits OK, 5 niveaux de qualité (q4 Ultra : 322 i/s, 188 Mio) ; **DLL pont obligatoire** (B4 : lib NGX compilée CUDA 11.8, plante avec le runtime 13) ; **NGX Update Module** à neutraliser et à tester (SEC-01) | ENH-040, 041, 042 |
| TensorRT 11.4 | Plus de `--fp16` : conversion ONNX **mixte** (convolutions FP16, grille et `GridSample` FP32 ; FP16 complet = -4 à -8 VMAF) ; moteur ~85 s au premier traitement (8,5 s avec cache de timing), chargement 0,3-0,6 s | ENH-043, 044 |
| Coupures de plan | Les images générées à cheval sur une coupure sont ratées (VMAF min ~0-5) | ENH-046 |
| Dépendances | Qt 6.11.2 sans Network (overlay ports vcpkg) ; FFmpeg BtbN LGPL = **développement seulement** ; **SEC-06 : FFmpeg vcpkg minimal sans réseau obligatoire avant ENH-030** (exceptions dev E3/E3c/E3d/E3e échues le 2027-01-21) | ENH-048, 028, 030 |
| QA | `enhance-bench.exe` demandé (protocole §4.3) | ENH-047 |
| Machine | ~2 à 3,8 Gio de VRAM déjà occupés au repos par le bureau (Docker, iCUE, navigateur) : mesures en différence ; fermer les applications graphiques avant une mesure officielle | ENH-014, QA |

## Seuils QA de référence (protocole `docs/qa/protocole.md` §7, à jour de la décision n° 14)

**Qualité** (jeu A, moyenne sur les clips à vérité terrain **et** minimum par clip), écart à Lanczos mesuré sur les mêmes images. Notation : `marge = 100 - VMAF Lanczos`.

| Préréglage | Delta VMAF 4K minimal | SSIM / PSNR | Garde-fou VMAF-NEG | Minimum par clip |
|---|---|---|---|---|
| **Qualité max** | >= **50 %** de la marge | jamais sous Lanczos sur aucun clip ; seuils chiffrés en part de marge **à fixer par le qa-tester** (protocole §7.1) | delta NEG >= 50 % de (100 - NEG Lanczos) **[H]** | aucun clip sous Lanczos + 25 % de la marge **[H]** |
| **Équilibré** | >= **35 %** | idem | >= 35 % **[H]** | aucun clip sous Lanczos + 15 % **[H]** |
| **Rapide** | >= **20 %** | idem | >= 20 % **[H]** | aucun clip sous Lanczos |

Exemples chiffrés (VMAF Lanczos du 2026-10-21, jeu A) : **BBB-PAN Kept 82,57** -> Qualité max >= 91,3 ; Équilibré >= 88,7 ; Rapide >= 86,1. **SYN-TEXT 63,74** -> >= 81,9 ; 76,4 ; 71,0. **TOS-FACE 92,81** -> >= 96,4 ; 95,3 ; 94,3. **BBB-FAST Kept 92,26** -> >= 96,1 ; 95,0 ; 93,8. **TOS-DARK 95,90** -> >= 97,9 ; 97,3 ; 96,7. Ces valeurs sont des **cibles non encore mesurées** : la première mesure d'Enhance (ENH-009) peut les contredire ; en cas d'écart, la QA remonte le chiffre au PO (jamais de relance jusqu'à PASS).

**Interpolation** (images `Generated`) : >= valeur du spike - 1 -> **BBB-FAST >= 90,5**, **BBB-PAN >= 89,9**, **SYN-TEXT >= 68,7** (RIFE mixte, 720p, interpolation seule, modèle VMAF par défaut : 91,5 / 90,9 / 69,7) ; 1er centile >= celui de la duplication ; Rapide : >= duplication + 1.

**Contre NVIDIA Broadcast 2.3** (jeu B, >= 2 clips) : VMAF >= Broadcast - 0,5 ; SSIM et PSNR sans tolérance. Rapport Broadcast **en cours de production par le qa-tester** (`docs/qa/resultats/2026-10-21_reference-broadcast.md`, à venir) : tant qu'il n'existe pas, ce seuil est « non vérifié », jamais PASS par défaut.

**Vitesse** (images de sortie / s, médiane de 5 passes, valeur soutenue) : budgets **Qualité max >= 30**, **Équilibré >= 60**, **Rapide >= 100**. Mesuré au spike (marge) : 36,5 HEVC / 51,5 AV1 ; 103-112 ; 219 (**la marge de Qualité max est faible : 1,2x en HEVC**). PASS / avertissement / FAIL : voir protocole §7.2.

**Ressources** : VRAM pic (pic - repos, in-process) **avertissement > 3 Gio** (mesuré 1,5-2,1 Gio), **FAIL > 9 216 Mio** (budget d'architecture) ; dérive VRAM sur 10 fichiers <= 50 Mio ; dérive mémoire système <= 100 Mio ; aucun plantage sur les cas limites (protocole §8). **Régression** : FAIL si > 5 % de perte (VMAF, SSIM, PSNR, i/s) ou > 5 % de VRAM en plus par rapport à la dernière mesure de `main`.

## Critères UX transverses (toute story `[UI]`, d'après `principes.md`)

Applicables en plus des critères propres à chaque story ; vérifiés par le ui-designer (contrôle design) et le qa-tester (`docs/qa/checklist-ui.md`).
- [ ] Parcours réalisable **au clavier seul** (ordre de tabulation = ordre visuel, focus visible >= 2 px et >= 3:1, pas de piège clavier, Échap restitue le focus).
- [ ] Aucun texte tronqué à **200 %** et à **760 px** de large ; rendu net à 100, 125, 150, 200 %.
- [ ] Contraste >= 4,5:1 (texte) et >= 3:1 (composants, focus), thèmes **sombre et clair** ; mode **contraste élevé Windows** : couleurs système, bordures visibles.
- [ ] **Narrateur** : nom, rôle, valeur de chaque contrôle ; lecture complète d'une carte dans chacun de ses 7 états ; progression annoncée <= 1 fois / 5 s ; erreurs en assertif.
- [ ] **« Réduire les animations »** de Windows : aucune transition de mouvement spatial ; aucune animation clignotante > 3 Hz.
- [ ] Textes en français (vouvoiement discret / formes impersonnelles, `qsTr`), nombres à la française ; un seul bouton plein accent à l'écran.
- [ ] Animations 150-350 ms issues de `Theme` (aucune durée, couleur ou taille en dur dans le QML) ; **fil d'interface : 99e centile des images <= 16,7 ms** pendant un traitement GPU (protocole §9).
- [ ] Captures de chaque état, thème et échelle fournies au ui-designer (sans chemin ni nom d'utilisateur visible) ; écart aux maquettes `docs/design/ui/maquettes/` classé CONFORME / ECARTS.

## Questions pour le PO et points ouverts

**Pour le PO**
0. **Squelette ENH-004 (UX) — en attente PO** : (a) la barre d'état affiche-t-elle « GeForce RTX 5070 · prête » en dur (y compris sur une autre machine, le temps du développement) ou le texte neutre « Carte graphique · vérification à venir » ? (b) la capsule « Tester le ressort » reste-t-elle visible pendant la démo (avec « Démonstration technique ») ou n'est-elle activée que par Ctrl+Maj+D ? Valeurs par défaut en attendant : texte en dur, capsule visible (à confirmer avant le développement d'ENH-004).
1. **Composition du Sprint 1** : on privilégie une démo « fenêtre + décodage + super-résolution d'une image » (ENH-004, ENH-008, ENH-040, ENH-047) et on place l'export RIFE (ENH-043) en sprint 3. Si vous préférez sécuriser la licence RIFE tout de suite, ENH-043 (5 pts) remplace ENH-008 (le premier fichier 4K de bout en bout glisse alors d'un sprint). Confirmer.
2. **Dépôt GitHub** : sans remote (`rolkapone/P153` non créé), la CI GitHub (`ci.yml`, `security.yml`) n'a jamais tourné ; ENH-003 reste à 4/5 et ENH-049 ne peut pas le clore. Créer le dépôt (privé) au sprint 2 ? Aucun push sans votre accord.
3. **SEC-05 (HIGH, processus)** : la config git **globale** de la machine contient l'adresse professionnelle ; à corriger par vous (noreply, ou `includeIf`), et activer sur GitHub « Keep my email addresses private » + « Block command line pushes that expose my email ». Bloque la fin d'ENH-049.
4. **Poids RIFE** : télécharger vous-même `flownet.pkl` (RIFE 4.25, lien « Trained Model » du README de Practical-RIFE), noter son SHA-256 et archiver README + `LICENSE` du commit `bbfd2ea9` dans `third_party/models/rife/_licences/` (ENH-043, sprint 3).
5. **WER (SEC-14)** : exclure Enhance des rapports d'erreurs Windows (`WerAddExcludedApplication`) avec vidage local facultatif ? Recommandation secdevops : oui. À trancher avant ENH-025.
6. **Seuils SSIM / PSNR** : la décision n° 14 ne vise que le VMAF ; le qa-tester proposera des équivalents en part de marge (protocole §7.1). Valider à ce moment-là.
7. **Notification de fin de file** (ENH-019) : toast Windows (exige un identifiant d'application, absent d'une version portable) ou repli discret dans l'app + clignotement de la barre des tâches ? À décider au sprint de ENH-019.

**Entre agents (consignés, pas pour le PO)**
- **Pilote minimal affiché** : `parcours.md` P-01 dit « version 580 », le guide du RTX Video SDK dit r550.58 (VSR) / r570 (TrueHDR), `scripts/gpu-requirements.json` exige 617.42 (pilote de la machine). Le gpu-engineer tranche la valeur, l'ux-designer aligne le texte (ENH-031).
- **`parcours.md` P-06 est périmé** : il écrit « la VRAM est relâchée (< 2 s) » à la pause, contraire à la décision n° 8 (VRAM conservée). L'ux-designer corrige avant ENH-021.
- **Ctrl+Entrée** et **Rétablir = relancer depuis le début** (`design-system.md` §11) : trancher ux / ui / gpu-engineer avant ENH-017 / ENH-021.
- **Textes P-08 `SourceUnreadable` et P-08b (préparation des modèles)** : **faits** par l'ux-designer (`parcours.md`), plus à transmettre.
- **Comparaison Broadcast** : renvoi `docs/qa/resultats/2026-10-21_reference-broadcast.md` (qa-tester, en cours) ; à intégrer au plan dès qu'il existe (seuil « vs Broadcast » de ENH-009, ENH-029).

## État des stories du kickoff

| Story | Statut | Reste à faire / transféré à |
|---|---|---|
| ENH-001 Spike VSR | **DONE** (5/5) | Critères transférés : sortie 4K lisible et VMAF/SSIM/PSNR de la VSR vs Lanczos -> **ENH-009** ; test réseau de NGX -> **ENH-041**. Livrable : `docs/gpu/spike.md` (phases 1-2) |
| ENH-002 Spike interpolation | **DONE** (5/5) | Brique choisie : RIFE 4.25 (ADR-043/044). Livrable : `docs/gpu/spike.md` (phase 3). Licences consignées. Suites : ENH-043, 044, 045, 046 |
| ENH-003 Repo, build, CI, tests GPU | **PARTIEL** (4/5) | Fait : `build.ps1` MSVC v145 + vcpkg, `test-gpu.ps1` (rapport 2026-10-21_1832 VERT), nvcc 13.4 sans `-allow-unsupported-compiler`, identité noreply locale + hook `commit-msg`, `ci.yml` et `security.yml` écrits. **Non vérifié** : CI verte sur GitHub (pas de remote). Reliquat -> **ENH-049** |
| ENH-005 Protocole QA et clips | **DONE** (3/3) | Critère « extraits des archives du PO » -> **ENH-029** ; comparaison Broadcast en cours (renvoi ci-dessus) |
| ENH-006 Threat model et licences | **DONE** (3/3) | Findings SEC-01 à SEC-22 et registre livrés ; les actions sont reprises dans les stories ci-dessous |
| ENH-007 Design system | **DONE** (3/3) | Tokens (`tokens.json`, `tokens.css`, `Theme.qml`, `qmldir`), maquettes validées par le PO, contrastes vérifiés. Reste : ligne « icônes Lucide ISC » du registre (tâche secdevops, Sprint 1) |

## Traçabilité sécurité : findings -> stories

| Finding | Sév. | Traité par |
|---|---|---|
| SEC-01 NGX Update Module | HIGH | ENH-041 |
| SEC-02 TensorRT (obligations de livraison) | LOW (levé) | ENH-030 |
| SEC-03 poids RIFE / export maison | MEDIUM | ENH-043 (+ ENH-030) |
| SEC-04 FRUC non redistribuable | MEDIUM | ENH-030 (refus par `package.ps1`) ; FRUC reste repli local |
| SEC-05 config git globale | HIGH (processus) | Action PO + ENH-049 |
| SEC-06 FFmpeg minimal | HIGH (porte de release) | ENH-048 (avant ENH-030) |
| SEC-07 CRL / OCSP de NGX | MEDIUM | ENH-041 |
| SEC-08 durcissement du chargement de DLL | MEDIUM | ENH-004 (+ ENH-040 pour les SDK) |
| SEC-09 sondage dans le processus principal | MEDIUM | ENH-015 (limites, fuzzing) ; isolement en processus = ENH-051 (Plus tard) |
| SEC-10 chemins UNC au redémarrage | MEDIUM | ENH-022 |
| SEC-11 suppression de temporaires | MEDIUM | ENH-022, ENH-024 |
| SEC-12 cache de moteurs TensorRT | MEDIUM | ENH-044 |
| SEC-13 dépassements d'entiers | MEDIUM | ENH-008 (décodeur, **fait**), ENH-015 (fin) |
| SEC-14 journaux tiers / WER | MEDIUM | ENH-025 |
| SEC-15 glisser-déposer, `ShellExecute` | LOW | ENH-017, ENH-023 |
| SEC-16 chemins de build dans les binaires | LOW | ENH-030 |
| SEC-17 `ffmpeg.lock.json` versionné | LOW | **fait** (`deps/ffmpeg.lock.json`) |
| SEC-18 bornes JSON | LOW | ENH-018, ENH-022 |
| SEC-19 binaires non signés | LOW | Avant publication (ENH-052) |
| SEC-20 exécution en administrateur | LOW | ENH-004 |
| SEC-21 CLUF d'Enhance | à confirmer | ENH-050 (+ ENH-052) |
| SEC-22 imports `ws2_32` de Qt6Core | LOW | ENH-028 |
| SEC-23 liste blanche de décodeurs, `format_whitelist`, `max_pixels` | MEDIUM | ENH-015 (+ liste de ENH-048) |
| SEC-27 cache de pipeline Qt (doc utilisateur, désinstalleur) | LOW | ENH-025, ENH-030 |
| SEC-28 `SetSearchPathMode` + test | LOW | Avant ENH-016 |

---

# MVP

## Epic A — Fondations techniques

### ENH-001 — Spike RTX Video SDK : super-résolution 720p -> 4K  [GPU] [Must | 5 pts]  — **DONE (kickoff)**
En tant qu'équipe, nous voulons mesurer la super-résolution du RTX Video SDK sur la RTX 5070 afin de confirmer la faisabilité et fixer les budgets.
Critères d'acceptation :
- [x] Étant donné le SDK posé par le PO dans `third_party/`, quand on lance le banc du spike, alors la super-résolution s'exécute sur la 5070 (interface CUDA, après contournement de B4). *« Vidéo 4K lisible » sur `REF-A` : transféré à ENH-009 / ENH-010.*
- [x] (GPU) débit en images/s et VRAM pic consignés dans `docs/gpu/spike.md` : q4 Ultra **322 i/s, 188 Mio** ; q3 High 421 i/s. *VMAF/SSIM/PSNR vs Lanczos : transféré à ENH-009 (la qualité n'a pas été mesurée au spike).*
- [x] Les 5 niveaux de qualité du SDK sont listés avec leur coût (i/s, VRAM).
- [x] Prérequis documentés : pilote >= r550.58 (VSR), CUDA >= 12.8 (13.4 retenu), MSVC v145 accepté par `nvcc`, `sm_120`.
- [x] Verdict go consigné (`spike.md` P2.4) ; ADR-040 à ADR-051 (`docs/gpu/architecture-pipeline.md`) ; budgets reportés dans `docs/architecture.md` par ENH-014.
Notes : B4 (lib NGX compilée CUDA 11.8) levé par une DLL pont de contournement -> productisée par ENH-040.

### ENH-002 — Spike interpolation : RIFE vs NVIDIA Optical Flow FRUC  [GPU] [Must | 5 pts]  — **DONE (kickoff)**
En tant qu'équipe, nous voulons comparer RIFE (TensorRT/CUDA) et le FRUC de NVIDIA Optical Flow afin de choisir la brique de génération d'images.
Critères d'acceptation :
- [x] i/s, VRAM pic et VMAF des images générées pour chaque candidat (`spike.md` phase 3) : **RIFE mixte VMAF 91,5 / 90,9 / 69,7** (BBB-FAST, BBB-PAN, SYN-TEXT) contre FRUC 89,2 / 87,8 / 55,0 ; RIFE mixte 181 i/s, 354 Mio (720p).
- [x] Licences de chaque candidat inscrites dans `licences.md` (SEC-03 poids RIFE, SEC-04 FRUC).
- [x] Impact combiné estimé et **mesuré** : chaîne complète 103-112 i/s (Équilibré), VRAM 1,5-2,1 Gio.
- [x] ADR-043 / ADR-044 : **RIFE 4.25 + TensorRT natif, 720p, avant la super-résolution** ; FRUC = repli local non livrable.
Notes : TensorRT 11 ne propose plus `--fp16` : précision mixte maison (voir ENH-043).

### ENH-003 — Setup repo, build CMake/MSVC, CI et tests GPU locaux  [Must | 4 pts]  — **PARTIEL (4/5 livrés)**
En tant que développeur, je veux un build reproductible et une CI afin de livrer chaque story proprement.
Critères d'acceptation :
- [x] Étant donné un clone propre, `scripts/build.ps1` compile avec MSVC v145 (VS 2026) et vcpkg en mode manifeste.
- [ ] Le workflow GitHub Actions exécute build, `clang-format`, `clang-tidy` et tests unitaires sans GPU et passe au vert. *Écrit, jamais exécuté (pas de remote) : reliquat dans ENH-049.*
- [x] `scripts/test-gpu.ps1` lance `tests/gpu/` en local et retourne un code non nul en cas d'échec (rapport VERT du 2026-10-21).
- [x] Identité git noreply en config locale, hook `commit-msg`, liste d'exclusion respectée. *SEC-05 (config globale) = action PO, suivie dans ENH-049.*
- [x] `nvcc` 13.4 accepte MSVC v145 sans `-allow-unsupported-compiler`.

### ENH-004 — Squelette de l'application Qt Quick + prototype Mica  [UI] [Must | 5 pts]  — **DONE (2026-10-23)**
Livré : revue APPROVE (cycle 2/3), contrôle design CONFORME, QA PASS (vérifications à l'écran faites par le PO), secdevops PASS, merge `79a3e93` (--no-ff). Les cases ci-dessous restent à titre de référence des critères vérifiés. Suites ajoutées : voir « Suites d'ENH-004 » après cette story.
En tant que PO, je veux une fenêtre Enhance qui s'ouvre, branchée sur le C++ afin de valider la stack et la finition de base.
Critères d'acceptation :
- [x] ADR front : Qt Quick comparé à WinUI 3, WebView2 et Dear ImGui (ADR-001), **validé par le PO**.
- [ ] Étant donné l'exécutable, quand on le lance, alors une fenêtre « Enhance » s'affiche en **moins de 2 s** (mesure à froid), thème sombre, texte en français, sans assistant ni écran d'accueil (P-01).
- [ ] La fenêtre reprend la coque de `maquettes/ecran-principal.html` : barre de titre sombre, **fond Mica** (`DWMWA_SYSTEMBACKDROP_TYPE`) à 100 % et 200 % ; **repli** : fond opaque teinté par les tokens si le Mica avec fenêtre Qt Quick transparente est instable (risque A1, décision consignée en ADR).
- [ ] Le singleton `Theme` (`docs/design/ui/Theme.qml`, module `Enhance.Design`) est intégré ; **aucune couleur, taille ou durée en dur** dans le QML ; une animation ressort de démonstration est mesurée à 60 i/s.
- [ ] Un `Q_PROPERTY` C++ est affiché dans le QML ; une action QML appelle le C++ ; aucune logique métier dans le QML (revue).
- [ ] Tests unitaires du modèle C++ (`enh_core`, GoogleTest) et un test Qt Quick Test s'exécutent sans GPU en CI ; **jamais** de `Qt6Network.dll` dans le dossier de sortie (contrôle C2).
- [ ] **Durcissement SEC-08** : `SetDefaultDllDirectories` + `SetDllDirectoryW(L"")` en première instruction, `/DEPENDENTLOADFLAG:0xA00`, `qt.conf` + `setLibraryPaths` avant `QGuiApplication`, variables `QT_PLUGIN_PATH` / `QML_IMPORT_PATH` ignorées en release ; test des chemins réels des modules chargés. **SEC-20** : manifeste `asInvoker` + `longPathAware` + PerMonitorV2, bandeau si le jeton est élevé.
- [ ] Critères UX transverses (section dédiée), en particulier rendu net à 100 / 125 / 150 / 200 % et contrôle design CONFORME.
- [ ] **Critères UX propres au squelette** (`docs/design/ux/ENH-004.md` §7 ; 9 états en §2) :
  - [ ] UX-1 premier affichage : la première image visible contient déjà fond, titre « Enhance » et « Déposez vos vidéos ici » (pas d'écran d'accueil, pas de flash blanc ; capture image par image).
  - [ ] UX-2 contenu strict : barre de titre, état vide, capsule « Tester le ressort » + « Essais : 0 » + « Démonstration technique », barre d'état ; **aucun autre** bouton, menu ou texte ; textes identiques à §5 de la note UX.
  - [ ] UX-3 dépôt inerte : un fichier glissé n'ajoute rien, aucun message, aucun plantage, curseur « interdit ».
  - [ ] UX-4 thèmes : sombre / clair suivent Windows, bascule en direct en **< 300 ms** sans clignotement ; contrastes texte >= 4,5:1, capsule et focus >= 3:1.
  - [ ] UX-5 replis : transparence désactivée = fond et capsule opaques sans déplacement ; contraste élevé = couleurs système + liseré 2 px ; animations réduites = aucune transition de mouvement.
  - [ ] UX-6 échelle 100 / 125 / 150 / 175 / 200 % : rien de tronqué, texte net ; fenêtre minimale **760 x 520** utilisable ; changement de moniteur net en < 1 s.
  - [ ] UX-7 clavier seul : Tab atteint la capsule (anneau >= 2 px, >= 3:1), Espace / Entrée l'activent, pas de piège, Ctrl+Q et la croix ferment sans boîte de dialogue.
  - [ ] UX-8 Narrateur : ordre de lecture et noms de §5, capsule annoncée comme bouton, compteur annoncé une fois par incrément, illustration non lue (**vérification par le PO**, voir seuils QA §6).
  - [ ] UX-9 fluidité : ressort et fondu à 60 i/s à 100 % et 200 % (aucune image > 20 ms sur 20 activations rapides) ; **0 image rendue** au repos.
  - [ ] UX-10 démarrage à froid **< 2 s**, sans appel réseau.
- [ ] **Seuils QA** (`docs/qa/checklist-ui.md`, `docs/qa/sprint-1-seuils.md` §3) : 99e centile du temps d'image <= 16,7 ms (33 ms toléré sur < 0,1 % des images) au repos et sous charge GPU ; temps GPU de l'interface <= 1 ms moyen / <= 2 ms pire ; écart d'i/s du pipeline verre complet vs opaque <= 2 % (7 passes alternées ; séries instables = « non concluant », jamais PASS) ; aucun `Qt6Network.dll`.
Notes : **décisions PO du 2026-10-21** : (1) barre d'état « GeForce RTX 5070 · prête » en dur (détection réelle : ENH-031) ; (2) capsule « Tester le ressort » visible à la démo ; (3) démo : fenêtre calme d'abord, `--glass-testbed` sur demande ; (4) icône provisoire `docs/design/ui/icones/enhance-app-provisoire.svg` acceptée, définitive plus tard. **Arbitrages** : UX-4 lu « libellé >= 4,5:1 et anneau de focus >= 3:1 » (WCAG 1.4.11) ; mesures du coût du verre pendant un vrai traitement GPU (design-system §12 n° 7 à 9) reportées à ENH-010 ; compléter `maquettes/index.html` (bouton de verre autonome). Branche `feature/ENH-004-squelette-qt`. Références design : `docs/design/ui/maquettes/enh-004-squelette.html`, `docs/design/ui/ENH-004-specs.md` (ui-designer, en cours d'écriture). Dépend du build Qt déjà produit (overlay ports vcpkg, Qt 6.11.2 sans Network). Prototype borné dans le temps (voir risques du Sprint 1). Licences Qt à jour dans `licences.md` (secdevops).

#### Suites d'ENH-004 (ajoutées le 2026-10-23, hors points)
- **SEC-27** (cache de pipeline Qt) : documenter l'emplacement du cache dans la doc utilisateur et le faire supprimer par le désinstalleur -> à traiter dans **ENH-025** (journalisation / données locales) et **ENH-030** (version portable / désinstallation).
- **SEC-28** (`SetSearchPathMode` + test) : à faire **avant ENH-016**.
- **B2** (pics ponctuels de rendu observés) : à rejouer sur machine calme ; si confirmés, ouvrir une story de correction.
- **Dette design** : (a) pointillés du cadre de dépôt générés depuis un jeton du design system ; (b) illustration de l'état vide en `.svg`.
- **Recoupement PresentMon** du coût GPU de l'interface : déjà noté dans les critères d'**ENH-010** (« Avant de démarrer ») ; rien à ajouter, à ne pas oublier.
- Décisions du PO n° 17 à 19 (voir « Décisions du PO »), dont le réglage de l'opacité du verre à **ENH-016**.

### ENH-005 — Protocole qualité et clips de référence  [Must | 3 pts]  — **DONE (kickoff)**
En tant que qa-tester, je veux un protocole de mesure afin que chaque seuil soit vérifiable.
Critères d'acceptation :
- [x] `docs/qa/protocole.md` : méthode (4K -> 720p -> traitement -> comparaison), modèles VMAF (`vmaf_4k_v0.6.1` + NEG), PSNR/SSIM, mesure d'i/s et de VRAM in-process.
- [x] `docs/qa/clips.md` : 4 clips libres (BBB-FAST, BBB-PAN, TOS-FACE, TOS-DARK) + 1 synthétique (SYN-TEXT), source, licence, SHA-256 ; récupérés par `scripts/fetch-clips.ps1`, aucun commité.
- [x] `tests/quality/` produit le tableau VMAF/SSIM/PSNR de Lanczos sans Enhance (`2026-10-21_reference-lanczos.md`, 18 mesures).
- [x] Méthode de comparaison avec Broadcast 2.3 décrite (`docs/qa/broadcast.md`) ; **mesure en cours** (renvoi ci-dessus).
- [ ] 1 ou 2 extraits des archives du PO (décision n° 3) -> **transféré à ENH-029**.
Notes : le protocole §7.1 est à mettre à jour par le qa-tester (seuils en part de marge, décision n° 14).

### ENH-006 — Threat model et registre des licences  [Must | 3 pts]  — **DONE (kickoff)**
Critères d'acceptation : [x] `threat-model.md` (STRIDE, SEC-01 à SEC-22) ; [x] `licences.md` (SDK NVIDIA, FFmpeg, Qt, polices, modèles, outils) ; [x] méthode « zéro réseau » (`zero-reseau.md`) ; [x] hypothèse de publication consignée (décision n° 4).
Notes : les findings sont transformés en stories (section « Traçabilité sécurité »).

### ENH-007 — Design system et tokens  [UI] [Must | 3 pts]  — **DONE (kickoff)**
Critères d'acceptation : [x] tokens sombre / clair, typographie, espacements, rayons, ombres, durées 150-350 ms en QML importable (`Theme.qml`) ; [x] police Segoe UI Variable (système, non livrée) validée ; [x] maquettes HTML animées validées par le PO ; [x] contrastes >= 4,5:1 vérifiés dans les deux thèmes (`design-system.md` §2.2).
Notes : icônes Lucide (ISC) à inscrire dans le registre (secdevops).

### ENH-040 — DLL pont RTX Video et chargement différé des DLL NVIDIA  [GPU] [Must | 3 pts]
En tant qu'équipe, nous voulons une DLL pont qui isole la bibliothèque NGX (compilée avec CUDA 11.8) du runtime CUDA 13 afin que la super-résolution fonctionne sans plantage (bloquant B4).
Critères d'acceptation :
- [ ] Étant donné `rtxvideo_bridge.dll` (contient `nvsdk_ngx_s.lib`, **ne lie pas `cudart`**, fournit `cudaGetDevice`, `cudaGetDeviceProperties`, `cudaGetErrorString` via l'API driver), quand l'application initialise NGX, alors **aucun** `0xC0000409` : le test `tests/gpu` exécute une super-résolution 1280x720 -> 3840x2160 (q4) d'une image synthétique et vérifie que la sortie diffère de la bicubique (q0).
- [ ] Un test vérifie par analyse des imports (`dumpbin`) que le pont n'importe **aucun** `cudart*.dll`.
- [ ] (GPU) super-résolution seule, 720p -> 4K q4, 8 bits : **>= 300 images/s**, VRAM de l'effet **<= 300 Mio** (mesuré 322 i/s, 188 Mio + 48 Mio de tampons).
- [ ] SEC-08 : `nvngx_vsr.dll` du dossier `rel` uniquement (jamais `dev`), chargée **par chemin absolu** (`LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | SYSTEM32`), signature Authenticode NVIDIA vérifiée en test ; DLL pilote (`nvcuda`, `nvcuvid`, `nvEncodeAPI64`, `nvofapi64`, `nvml`) préchargées depuis System32 ; chargement différé (ADR-048).
- [ ] Interface `IUpscaler` (faux upscaler pour la CI) ; ressources NGX en classes RAII ; 10 créations / destructions de l'effet : 0 Mio de dérive.
- [ ] Note `docs/gpu/ENH-040.md` : cause, contournement, **limites** (n'écrit que le préfixe 11.x de `cudaDeviceProp`), à revoir à chaque nouvelle version du SDK ; texte à transmettre à NVIDIA (signalement) prêt pour le PO.
- [ ] **Seuils QA** (`docs/qa/sprint-1-seuils.md` §3, §4.5-4.8) : VSR 720p -> 4K q4, **10 bits**, médiane de 5 : **>= 300 i/s** (spike 335 ; avertissement 240-300, FAIL < 240) ; VRAM de l'effet (pic - avant création) **<= 300 Mio** ; sortie q4 != q0 sur image réelle : **>= 30 %** d'octets différents (spike 59 %) ; `dumpbin /dependents` sans `cudart*.dll` ; 10 cycles création / destruction : dérive **<= 50 Mio**, 0 handle ; qualité d'image VSR (3 images par clip x BBB-FAST, BBB-PAN, SYN-TEXT) : part Équilibré **>= 35 %** de la marge VMAF vs Lanczos sur la moyenne, aucune métrique (VMAF, SSIM, PSNR) sous Lanczos, part VMAF-NEG >= 25 %.
Notes : pas d'écran : aucun critère UX. Dépend de ENH-001 (fait). Relecture attentive par le reviewer (liaison atypique). Ne contient aucun appel à `NVSDK_NGX_UpdateFeature` (voir ENH-041).

### ENH-041 — NGX hors ligne : neutraliser la mise à jour et le prouver  [GPU] [Must | 3 pts]
En tant que PO, je veux la preuve que la super-résolution n'émet rien sur le réseau afin de tenir la promesse « 100 % local » (SEC-01 HIGH, SEC-07 MEDIUM).
Critères d'acceptation :
- [ ] Étant donné l'application qui initialise NGX et traite une image, quand on exécute les tests dynamiques **M1 (pare-feu bloquant), M2 (audit WFP), M3 (capture)** de `docs/security/zero-reseau.md`, alors **0 tentative de connexion** d'Enhance ni de `nvngx_vsr.dll` est journalisée.
- [ ] `NVSDK_NGX_UpdateFeature` n'est jamais appelée (contrôle statique en CI) ; identifiant d'application `0`, pas de `ProjectIdDescription` ; `InApplicationDataPath = %LOCALAPPDATA%\Enhance\ngx` ; journalisation NGX désactivée en release.
- [ ] Au chargement, le chemin **réel** du module `nvngx_vsr.dll` (`GetModuleFileNameW`) est celui du dossier de l'application ; sinon erreur `ComponentMissing`.
- [ ] Vérification de signature : si un trafic CRL/OCSP est constaté (SEC-07), il est documenté comme exception système (sans donnée utilisateur) ou remonté au PO ; **si NGX télécharge une DLL : escalade immédiate au PO** et question à NVIDIA.
- [ ] Script `scripts/test-no-network-dynamic.ps1` (pare-feu + audit) rejouable en local ; rapport dans `docs/security/`.
Notes : secdevops + gpu-engineer ; dépend d'ENH-040 ; prépare ENH-028.

### ENH-042 — Conversions couleur CUDA (NV12 / P010 <-> RGB)  [GPU] [Must | 5 pts]
En tant qu'équipe, nous voulons des conversions couleur exactes sur le GPU afin d'alimenter le RTX Video SDK (entrée **RGB** seulement) sans perte visible ni décalage de teinte.
Critères d'acceptation :
- [ ] Noyaux CUDA NV12 -> ARGB8, P010 -> ABGR10, ARGB8 -> NV12, ABGR10 -> P010, **BT.709, plage limitée** (SDR), avec prise en charge de la plage pleine et de BT.601 en entrée (étiquetage lu au sondage, sans correction silencieuse).
- [ ] Étant donné NV12 -> RGB -> NV12 (aller-retour), alors l'écart maximal est **<= 1 niveau sur 255** (8 bits) et **<= 2 sur 1023** (10 bits) sur une mire de test, et l'écart des moyennes Y/U/V <= 1 niveau sur 1023 (contrôle `signalstats` du protocole §2.3).
- [ ] Largeurs / hauteurs impaires, 1x1 et 8192x4320 sans débordement (SEC-13 : `checked_mul`, tailles en `std::size_t`).
- [ ] (GPU) conversion 4K : **<= 0,3 ms par image** ; VRAM <= 100 Mio (budget ADR-042 : ~0,2 ms par image de sortie).
- [ ] Les conversions sont derrière une interface testable ; un test CPU de référence (sans GPU) valide les coefficients en CI.
Notes : dépend d'ENH-008 (surfaces NV12 / P010 décodées). Précède ENH-009 et ENH-010 (10 bits).

### ENH-043 — Export ONNX maison de RIFE depuis les poids MIT  [GPU] [Must | 5 pts]
En tant que PO, je veux un modèle d'interpolation dont la licence est propre afin de pouvoir livrer RIFE (SEC-03, décision n° 12).
Critères d'acceptation :
- [ ] Python 3.12 officiel + PyTorch (**roues CPU**) + `onnx` dans un **venv hors dépôt**, versions épinglées, `pip install --require-hashes` (fichier de dépendances avec empreintes, versionné) ; installation validée par le PO ; **jamais lancé en CI**, rien de Python n'est livré (contrôle de `package.ps1`).
- [ ] Un script d'export **écrit par nous** (à partir du code MIT de Practical-RIFE, **sans reprendre** le script ni l'enveloppe « v2 » de vs-mlrt) produit l'ONNX avec `torch.load(..., weights_only=True)` ; SHA-256 du `flownet.pkl`, de l'ONNX et du script consignées dans `deps/models.lock.json`.
- [ ] La **conversion mixte** (59 convolutions en FP16 entourées de `Cast` ; grille d'échantillonnage, `GridSample`, `Resize` en FP32) est faite à l'export (outil de build), pas dans l'application.
- [ ] Qualité : PSNR entre la sortie de l'ONNX exporté et celle de PyTorch CPU (FP32) **>= 60 dB** sur 20 images ; **VMAF des images générées >= 90,5 (BBB-FAST), >= 89,9 (BBB-PAN), >= 68,7 (SYN-TEXT)** (= RIFE mixte du spike - 1), mesuré par le qa-tester.
- [ ] Licences : README et `LICENSE` de Practical-RIFE (commit `bbfd2ea9`) archivés hors dépôt ; mention MIT + copyright dans `THIRD-PARTY-LICENSES.txt` ; ligne du registre `licences.md` passée à « OK » par secdevops.
- [ ] `package.ps1` refuse tout ONNX dont le SHA-256 n'est pas celui du verrou (et tout ONNX vs-mlrt).
Notes : action PO préalable (téléchargement du `flownet.pkl`, voir « Questions pour le PO » n° 4) ; l'ONNX vs-mlrt reste local, comme référence de comparaison.

### ENH-044 — Moteur TensorRT : construction au premier traitement, cache, exécution  [GPU] [Must | 5 pts]
En tant qu'utilisateur, je veux que la préparation de la fluidité se fasse une seule fois, avec un message clair, afin de ne plus attendre ensuite (décision n° 10).
Critères d'acceptation :
- [ ] Étant donné un ONNX mixte (ENH-043), quand un job demande l'interpolation pour la première fois, alors le moteur 720p est construit par l'API C++ (`IBuilder` + `nvonnxparser`), l'étape `PreparingModels` est émise, puis le traitement continue ; le moteur et le **cache de timing** sont écrits dans `%LOCALAPPDATA%\Enhance\cache\trt\`.
- [ ] (GPU) construction **<= 150 s à vide** (mesuré 84,5 s ; 2,5 min si le PC est chargé), reconstruction avec cache de timing **<= 15 s** (mesuré 8,5 s), chargement du moteur sérialisé **<= 1 s** (mesuré 0,3-0,6 s).
- [ ] (GPU) RIFE mixte 720p : **>= 150 images générées/s** (mesuré 181), VRAM **<= 500 Mio** (mesuré 354) ; 10 jobs à la suite : 0 Mio de dérive.
- [ ] **SEC-12** : `setEngineHostCodeAllowed(false)`, aucun moteur version-compatible ni « lean », aucun `loadLibrary` de plugin ; index du cache avec SHA-256 du moteur et clé (version TensorRT, `sm_120`, pilote, SHA-256 de l'ONNX) ; toute incohérence -> **reconstruction** (test unitaire de l'index sans GPU).
- [ ] La préparation est **annulable** (arrêt < 3 s, rien de partiel dans le cache) ; échec (disque plein, droits) -> erreur P-08 standard, traitement sans génération d'images toujours possible.
- [ ] Un moteur par résolution **ou** un profil dynamique : mesure comparée et ADR (profil dynamique = une seule construction pour plusieurs résolutions).
- [ ] Seules les DLL `nvinfer_11`, `nvinfer_builder_resource_sm120_11`, `nvonnxparser_11` (+ `nvinfer_plugin_11` si utilisé) sont chargées, par chemin absolu ; jamais `trtexec`.
Notes : dépend d'ENH-043 ; implémente l'étape décrite par `parcours.md` P-08b (côté UI : ENH-020). Interface `IInterpolator` définie dans ENH-012.

### ENH-045 — Comparaison des modèles d'interpolation : RIFE 4.25, FILM, IFRNet, EMA-VFI  [GPU] [Should | 5 pts]
En tant que PO, je veux comparer RIFE 4.25 à FILM, IFRNet et EMA-VFI afin de choisir le meilleur modèle pour « Qualité max » et de disposer d'un repli si la licence de RIFE posait problème (décision n° 13).
Critères d'acceptation :
- [ ] **Avant tout téléchargement**, secdevops consigne la licence du code **et des poids** de chaque candidat dans `licences.md` (un candidat à licence non commerciale ou non redistribuable est comparé en local mais marqué « BLOQUÉ pour la livraison ») ; poids téléchargés par le PO si un compte est requis.
- [ ] Chaque candidat est exporté en ONNX (outil local Python, ENH-043), converti en précision adaptée (la conversion mixte de RIFE ne se transpose pas à l'identique : à justifier modèle par modèle) et construit en moteur TensorRT 720p sur la 5070.
- [ ] Rapport `docs/gpu/comparaison-interpolation.md` : pour chaque modèle, **temps par image générée en 720p** (ms, médiane), VRAM, temps de construction, **VMAF / SSIM / PSNR des images `Generated`** sur BBB-FAST, BBB-PAN, SYN-TEXT (même méthode que `spike.md` P3.5, RIFE 4.25 mixte = 91,5 / 90,9 / 69,7), 1er centile, licence des poids.
- [ ] **Critère d'éligibilité à « Qualité max »** : **<= 45 ms par image générée** en 720p (calcul : la chaîne doit tenir >= 30 i/s de sortie en 24 -> 60 i/s, soit 33 ms par image de sortie dont 3/5 générées, après VSR et conversions) **[H]** ; un modèle plus lent est documenté mais écarté.
- [ ] Décision (ADR) : modèle retenu par préréglage, modèle de repli licence ; si un candidat bat RIFE de **>= 1,5 VMAF** en moyenne avec une licence livrable et une vitesse éligible, proposition de bascule soumise au PO.
Notes : planifiée après ENH-043 / ENH-044 ; abandonnable sans bloquer le MVP (RIFE 4.25 reste la brique par défaut). Piste complémentaire : RIFE 4.26 et variante « lite » (spike P3.8).

### ENH-046 — Détection de coupure de plan  [GPU] [Must | 3 pts]
En tant qu'utilisateur, je veux qu'aucune image ne soit « inventée » à cheval sur deux plans afin d'éviter les images fantômes.
Critères d'acceptation :
- [ ] Étant donné une paire d'images consécutives, quand une coupure est détectée, alors l'image intermédiaire est **la répétition de l'image précédente** (pas d'interpolation) ; la logique de décision est une **fonction pure** testée sans GPU (seuils en données).
- [ ] Sur `BBB-FAST` (3 coupures connues) : **3 / 3 détectées** ; sur `BBB-PAN` (sans coupure) et `SYN-TEXT` : **0 fausse détection**.
- [ ] Qualité : 1er centile et minimum du VMAF `Generated` de `BBB-FAST` **>= ceux de la duplication** (la duplication donne min 5,5 et 1er centile 13,0).
- [ ] (GPU) coût de la détection **<= 0,3 ms par image** à 720p ; aucune synchronisation CPU ajoutée dans la boucle.
- [ ] Les coupures détectées sont comptées dans le rapport (`enhance-bench`) et journalisées (nombre seulement).
Notes : le critère « changements de plan » d'ENH-012 est transféré ici ; dépend d'ENH-044.

### ENH-047 — `enhance-bench` : banc en ligne de commande pour la QA  [GPU] [Must | 2 pts]
En tant que qa-tester, je veux une CLI de banc afin de mesurer images/s et VRAM sans interface (protocole §4.3).
Critères d'acceptation :
- [ ] Version 1 (Sprint 1) : `enhance-bench --in <fichier> --stage decode --report <json>` décode un fichier vidéo sur NVDEC ; `--stage vsr-image --in <image> --out <image>` agrandit une image 720p -> 4K par la super-résolution (q0 à q4) pour un contrôle visuel. Le format d'image d'entrée / sortie (BMP, visible dans Windows, ou équivalent sans dépendance) est choisi par le gpu-engineer.
- [ ] Le JSON écrit : `frames_in`, `frames_out`, `wall_s`, `fps_out`, `stages[{name, ms_total, ms_per_frame}]`, `vram_peak_mib`, `vram_rest_mib` (in-process : `cuMemGetInfo` + `QueryVideoMemoryInfo`), `process_private_mib` début / fin, `exit_code`.
- [ ] Cible CMake `enhance-bench` (avec `ENH_WITH_GPU=ON` seulement), hors livrable ; `Measure-Speed.ps1` la détecte et s'en sert.
- [ ] Le gpu-engineer et le qa-tester vérifient que les chiffres coïncident avec `nvidia-smi` à < 10 % près.
- [ ] **Seuils QA** (`docs/qa/sprint-1-seuils.md` §3, §4.7) : tous les champs du JSON présents et cohérents (`fps_out` = `frames_out / wall_s` à 1 %) ; répétabilité de `fps_out` : (max - min) / médiane **<= 3 %** (sinon « instable ») ; écart banc / `nvidia-smi` **< 10 %** ; `Measure-Speed.ps1` s'exécute de bout en bout en modes Speed et Leak ; code de sortie 0 en succès, != 0 avec message clair sur fichier inexistant, 0 octet ou corrompu (aucun plantage).
Notes : pas d'écran : aucun critère UX. Étendu ensuite par les stories de chaîne : `--codec`, `--preset`, `--bits` dans ENH-010 ; `--interp` dans ENH-012 (pas de story séparée).

### ENH-048 — FFmpeg minimal sans réseau (port vcpkg)  [GPU] [Must | 5 pts]
En tant que secdevops, je veux un FFmpeg de livraison réduit au strict nécessaire afin de réduire la surface d'attaque et de tenir « zéro réseau » (SEC-06, porte de release **avant ENH-030**).
Critères d'acceptation :
- [ ] Port vcpkg (ou overlay) FFmpeg configuré avec `--disable-network --disable-protocols --disable-avdevice --disable-devices --disable-autodetect --disable-encoders --enable-encoder=mov_text`, **liste blanche** de démuxeurs, décodeurs, analyseurs et muxeurs (à établir avec le gpu-engineer à partir de la matrice d'ENH-008) ; **aucun décodeur audio** ; ni `gpl` ni `nonfree` ; la feature `nvcodec` n'active ni `cuda-nvcc` ni `libnpp`.
- [ ] `scripts/check-ffmpeg-license.ps1` vert ; **`scripts/check-no-network.ps1 -Package` vert sans les exceptions E3, E3c, E3d, E3e** de `scripts/network-allowlist.json` (échéance du 2027-01-21 respectée).
- [ ] La matrice de formats d'ENH-008 **et d'ENH-008b** (MP4, MKV, AVI, MOV, WebM, MPEG-TS x H.264, HEVC, VP9, AV1, MPEG-2 ; combinaisons valides) **passe encore** avec ce build.
- [ ] Sources exactes + configuration (port, overlay, triplet) archivées pour l'offre LGPL ; `NOTICE-SOURCES.txt` prêt (voir ENH-030).
Notes : devops + gpu-engineer ; le fuzzing d'ENH-015 s'exécute sur **ce** build.

### ENH-049 — Garde-fous CI et chaîne de livraison (reliquat devops du kickoff)  [Must | 5 pts]
En tant que devops, je veux terminer les garde-fous de la CI afin qu'aucune régression de sécurité ou de licence n'entre dans `main`.
Critères d'acceptation (chacun testé par un cas fautif qui doit **échouer**) :
- [ ] Image de CI **VS 2026** confirmée (ou repli décidé par ADR) ; reliquat d'ENH-003 : le workflow `ci.yml` **passe au vert sur GitHub** (dépend de la création du dépôt par le PO, voir « Questions pour le PO » n° 2).
- [ ] `scripts/licences.ps1 -Check` : le contenu livré et les dépendances du manifeste vcpkg correspondent au registre `licences.md` ; une dépendance non inscrite échoue.
- [ ] Job `cve-watch` (surveillance FFmpeg, Qt, bibliothèques vcpkg) et dossier `.semgrep/` (règles **locales**, `--metrics=off`).
- [ ] **ASan** sur les tests du cœur (job dédié).
- [ ] Hook de pré-commit **aligné** sur `security.yml` (mêmes contrôles) ; `pip install --require-hashes` pour les outils Python de la CI.
- [ ] `check-no-network.ps1` : une **date d'expiration illisible ou absente = violation** ; contrôle **C1** appliqué aux overlay ports ; `fetch-clips.ps1` **exige** le SHA-256 de chaque téléchargement.
- [ ] SEC-05 clos (action PO faite et vérifiée par le contrôle des auteurs de commits).
Notes : `package.ps1` -> ENH-030 ; `test-no-network-dynamic.ps1` -> ENH-041 / ENH-028.

### ENH-050 — À propos, mentions légales et CLUF  [UI] [Must | 3 pts]
En tant qu'utilisateur, je veux voir d'où viennent les composants et que rien n'est envoyé afin de faire confiance à l'application ; en tant que PO, je veux respecter les licences (SEC-21).
Critères d'acceptation :
- [ ] Section « À propos d'Enhance » de la feuille Réglages (P-09, Ctrl+,) : version, « Aucune donnée n'est envoyée hors de cet ordinateur », licences tierces (affichage de `THIRD-PARTY-LICENSES.txt`).
- [ ] Mentions : **Qt** (LGPLv3, où trouver les sources), **FFmpeg** (LGPL), « **Utilise le NVIDIA RTX Video SDK. NVIDIA et RTX sont des marques de NVIDIA Corporation.** » (**sans logo**), RIFE / Practical-RIFE (MIT + copyright), icônes Lucide (ISC), FreeType et IJG selon `licences.md` §3.
- [ ] Texte du **CLUF d'Enhance** rédigé par secdevops (gratuiciel propriétaire ou licence permissive, **jamais GPL** ; clauses NVIDIA : pas d'ingénierie inverse, pas de redistribution séparée ; clause LGPL d'ingénierie inverse pour le débogage), **validé par le PO** ; fichier `LICENSE` / `LICENSES\` livrés.
- [ ] Critères UX transverses ; textes relus par l'ux-designer.
Notes : ux-designer (écran existant dans `ecrans.md`, à compléter) -> ui-designer -> developer ; secdevops fournit les textes. Je ne suis pas juriste : relecture juridique conseillée avant publication (ENH-052).

## Epic B — Pipeline vidéo GPU

### ENH-008 — Démuxage FFmpeg et décodage NVDEC  [GPU] [Must | 5 pts]  — **DONE (2026-10-23)**
Livré : revue APPROVE (cycle 2/3), QA PASS (3 398 i/s sur BBB-FAST, 2 541 i/s sur BBB-PAN, 58 Mio de VRAM, dérive 0), secdevops PASS, merge `0c2f748` (--no-ff). Reports : test UNC -> ENH-015 ; suites SEC-23 et SEC-13 (fin) -> ENH-015 ; tâche `nvcuvid.dll` -> ENH-008b (voir ci-dessous). Les cases ci-dessous restent à titre de référence des critères vérifiés.

En tant qu'utilisateur, je veux ouvrir n'importe quel format vidéo afin de ne jamais convertir au préalable.
Critères d'acceptation :
- [ ] Étant donné la **matrice « noyau »** (`docs/gpu/ENH-008.md` §4) : MP4 x H.264 8 bits, MP4 x HEVC Main10, MKV x AV1 10 bits, WebM x VP9, MPEG-TS x MPEG-2, AVI x H.264, MOV x H.264, **MOV x ProRes 422 HQ (repli logiciel)**, plus `in720/BBB-FAST.mp4`, quand on décode, alors toutes les images arrivent en mémoire GPU (NVDEC : surfaces NV12 / P010 ; AV1 par le décodeur natif, `path() == Nvdec`) avec un nombre d'images **égal** à celui de `ffprobe -count_frames` (même build FFmpeg). Le reste de la matrice est dans **ENH-008b**.
- [ ] (GPU) décodage 720p **>= 1 000 images/s** (mesuré ~3 100 i/s avec le FFmpeg de mesure ; le plancher de 300 i/s de v0.2 était très pessimiste) ; VRAM **<= 600 Mio**.
- [ ] Si NVDEC ne supporte pas le codec, repli sur décodage logiciel FFmpeg avec message au journal (mesuré ~1 300 i/s en 720p).
- [ ] Ouverture par un `AVIOContext` maison sur `CreateFileW` (préfixe `\\?\`, ADR-049) ; **aucun chemin texte donné à FFmpeg** ; ouvertures secondaires refusées (`io_open` restrictif, `protocol_whitelist=file`) ; tests sur chemin > 260 caractères, accents, UNC. **UNC : non vérifié — reporté à ENH-015 (décision PO 2026-10-23)** (test `Win32File.PartageUnc`, variable `ENH_TEST_UNC_DIR`, non exécuté faute de partage accessible) ; chemin > 260 et accents restent exigés.
- [ ] SEC-13 : toutes les tailles en `std::size_t`, multiplications vérifiées (`checked_mul`), dimensions bornées avant allocation.
- [ ] Les ressources C (FFmpeg, CUDA) sont encapsulées en classes RAII ; aucune fuite sur 10 fichiers ; interface `IDecoder` abstraite (faux décodeur pour la CI).
- [ ] **Seuils QA** (`docs/qa/sprint-1-seuils.md` §3, méthode §4, médiane de 5 passes après chauffe, instable si (max - min) / médiane > 3 %) : décodage `BBB-FAST.mp4` **>= 1 000 i/s** (avertissement 800-1 000) ; VRAM (pic - repos) **<= 600 Mio** ; écart `enhance-bench` / `nvidia-smi` **< 10 %** ; boucle de 10 fichiers distincts : dérive VRAM **<= 50 Mio**, mémoire privée <= 100 Mio, 0 handle / thread en fuite ; repli logiciel (FFV1, ProRes) : décode, message de journal présent, 0 plantage ; cas limites (corrompu, tronqué, 0 octet, nom Unicode, chemin > 260) : aucun plantage, erreur claire, VRAM libérée (+/- 50 Mio) ; clips `in720/TOS-*.mkv` (1280x570) exclus de « 720p -> 4K ».
Notes : FFmpeg BtbN LGPL de développement (livraison : ENH-048). **Redécoupée au planning du Sprint 1** : estimation gpu-engineer 8 pts pour la matrice complète -> matrice étendue sortie en ENH-008b (3 pts). Corpus généré localement (`tests/quality/New-DecodeCorpus.ps1`, rien de commité).

### ENH-008b — Matrice de formats étendue  [GPU] [Must | 3 pts]
En tant qu'utilisateur, je veux que toutes les variantes courantes de fichiers vidéo s'ouvrent afin de ne jamais convertir au préalable.
Critères d'acceptation :
- [ ] Étant donné le **reste de la matrice valide** (MP4 x VP9 / AV1 / MPEG-2, MKV x H.264 / HEVC / VP9 / MPEG-2, MOV x HEVC, MPEG-TS x H.264 / HEVC / AV1 [à vérifier], WebM x AV1, AVI x MPEG-2), quand on décode, alors nombre d'images = `ffprobe -count_frames` et chemin attendu (NVDEC ou repli journalisé).
- [ ] Variantes 8 / 10 bits (dont H.264 10 bits : repli attendu, à vérifier sur Blackwell), **dimensions impaires**, **fréquence variable (VFR)**, **rotation** (métadonnée), changement de résolution en cours de flux (bascule logicielle au vol journalisée), entrelacé : aucun plantage, comportement documenté.
- [ ] Aucune régression sur ENH-008 : seuils inchangés (>= 1 000 i/s sur `BBB-FAST.mp4`, VRAM <= 600 Mio, dérive <= 50 Mio sur 10 fichiers) ; **pas de baisse > 5 %** par rapport à la mesure de `main`.
- [ ] SEC-13 : dimensions impaires, 1x1 et 8192x4320 sans débordement.
- [ ] **Seuils QA** (`docs/qa/sprint-1-seuils.md` §3-§5) : mêmes règles de mesure que ENH-008 ; une combinaison dont l'encodeur de génération est absent du FFmpeg de mesure est notée « non générée » et le critère « non vérifié » (jamais PASS par défaut).
- [ ] **Petite tâche (suite d'ENH-008)** : l'échec du préchargement de `nvcuvid.dll` est **journalisé** (niveau avertissement, code d'erreur Windows, sans chemin personnel) ; test unitaire avec un chargeur factice (sans GPU).
Notes : dépend d'ENH-008 ; **doit passer avant ENH-048** (qui rejoue la matrice) ; au plus tard Sprint 3. Corpus généré par `tests/gpu/corpus/make-corpus.ps1`.

### ENH-009 — Super-résolution RTX Video SDK dans le pipeline  [GPU] [Must | 5 pts]
En tant qu'utilisateur, je veux agrandir en 4K avec la meilleure qualité afin d'obtenir une image nette.
Critères d'acceptation :
- [ ] Étant donné `REF-A`, quand on agrandit en 3840x2160, alors la sortie fait exactement 3840x2160 ; **un fichier 4K lisible** est produit (reliquat d'ENH-001).
- [ ] (GPU) **Seuils Q** (section « Seuils QA de référence ») : Qualité max : delta VMAF >= 50 % de (100 - VMAF Lanczos) ; Équilibré >= 35 % ; Rapide >= 20 % ; SSIM et PSNR jamais sous Lanczos (reliquat d'ENH-001 : première mesure de la qualité de la VSR). Chaque clip mesuré, minimum par clip respecté.
- [ ] (GPU) étape seule : q4 Ultra **>= 300 i/s** (mesuré 322), q2 Medium >= 750 i/s (mesuré 822) ; VRAM **<= 400 Mio** (mesuré 188 + 48) ; pas de régression > 5 %.
- [ ] Les niveaux de qualité du SDK sont associés aux préréglages par **données** (q4 Qualité max et Équilibré, q2 Rapide : à confirmer par la mesure de qualité).
- [ ] Une source déjà en 4K ou plus est copiée sans agrandissement (message `NoUpscaleAlreadyAtTarget`).
- [ ] Utilise `IUpscaler` (ENH-040) et les conversions d'ENH-042 ; faux upscaler pour la CI.
- [ ] Mesure vs Broadcast 2.3 dès que le rapport `2026-10-21_reference-broadcast.md` existe (VMAF >= Broadcast - 0,5, SSIM / PSNR sans tolérance ; sinon « non vérifié »).
Notes : dépend d'ENH-008, 040, 042. **Risque** : les sources libres étant déjà douces, les cibles de VMAF (ex. TOS-FACE >= 96,4 en Qualité max) peuvent être inatteignables ; la QA remonte l'écart au PO au lieu de relancer.

### ENH-010 — Encodage NVENC HEVC 8 et 10 bits  [GPU] [Must | 5 pts]
En tant qu'utilisateur, je veux sortir en HEVC jusqu'à 10 bits afin de ménager les dégradés et la taille.
Critères d'acceptation :
- [ ] Étant donné une suite d'images 4K, quand on encode en HEVC Main10, alors `ffprobe` indique `hevc`, `yuv420p10le`, 3840x2160, BT.709 limité (conformité protocole §6, étiquette MP4 `hvc1`).
- [ ] (GPU) 4K 10 bits, sortie 60 i/s : **Rapide (P3) >= 200 i/s** (mesuré 265), **Équilibré (P5 + multipasse quart + anticipation 16) >= 90 i/s** (mesuré 103), **Qualité max (P7 HQ, multipasse pleine, B-frames 3) >= 30 i/s** (mesuré 37) ; VRAM encodeur **<= 1,5 Gio** (mesuré 0,36 / 0,87 / 1,27 Gio).
- [ ] Débit cible par préréglage en données (HEVC : 25 000 / 40 000 / 60 000 kbit/s [H]) ; débit réel à +/- 15 % de la cible (mesuré +/- 12 %).
- [ ] Le fichier est lisible dans VLC et Films et TV de Windows ; zéro erreur de décodage `ffmpeg -v error`.
- [ ] Exposé dans `enhance-bench` (`--codec`, `--preset`, `--bits`).
- [ ] **Avant de démarrer** : recouper le coût GPU de l'interface (verre, animations) avec **PresentMon** pendant un encodage ; le coût mesuré par `enhance-bench` / `nvidia-smi` et celui de PresentMon doivent concorder à 10 % près, sinon l'écart est consigné dans `docs/gpu/` (reporté d'ENH-004 / design-system §12 n° 7 à 9).
Notes : un seul moteur NVENC (c'est le goulot de la chaîne) ; **UHQ HEVC (29 i/s) est sous le budget de Qualité max** : ne pas le retenir sans décision du PO. Dépend d'ENH-042.

### ENH-011 — Encodage NVENC AV1  [GPU] [Must | 3 pts]
En tant qu'utilisateur, je veux choisir AV1 afin d'avoir des fichiers plus petits à qualité égale.
Critères d'acceptation :
- [ ] Étant donné les mêmes images que ENH-010, quand on choisit AV1 10 bits, alors `ffprobe` indique `av1`, 10 bits, 3840x2160 (étiquette `av01`).
- [ ] (GPU) 4K 10 bits : **Rapide >= 150 i/s** (mesuré 178), **Équilibré >= 90 i/s** (mesuré 105), **Qualité max >= 40 i/s** (mesuré 52) ; VRAM encodeur **<= 1,6 Gio** (mesuré 0,63 / 1,27 / 1,46 Gio).
- [ ] À débit égal, VMAF AV1 >= VMAF HEVC - 0,5 sur `REF-A`.
- [ ] Si le codec n'est pas disponible (`GpuCapabilities`), l'option est grisée avec l'info-bulle « Cette carte ne sait pas produire ce format. » (P-03).

### ENH-012 — Interpolation d'images vers 60 images/s  [GPU] [Must | 5 pts]
En tant qu'utilisateur, je veux des vidéos fluides à 60 i/s afin de profiter d'un écran 4K récent.
Critères d'acceptation :
- [ ] Source 24, 25, 30 i/s : sortie à **60 i/s constants**, durée inchangée (+/- 1 image).
- [ ] Source strictement < 50 i/s, y compris 31 à 49 i/s (ex. 40 i/s) : interpolée vers 60 i/s constants (décision n° 1).
- [ ] Source >= 50 i/s : aucune interpolation, cadence conservée, jamais au-delà de 60 i/s (message au journal).
- [ ] Source 24 i/s : vrai 60 i/s interpolé (pas de pulldown 3:2) ; 3 images générées sur 5.
- [ ] Interface `IInterpolator` + faux interpolateur pour la CI ; brique : RIFE 4.25 mixte, 720p, **avant** la super-résolution (ADR-043 / 044) ; détection de coupure : ENH-046 ; moteur : ENH-044.
- [ ] (GPU) chaîne 720p -> 4K60 complète : **Équilibré >= 60 i/s** (mesuré 103-112), **Qualité max >= 30 i/s** (mesuré 36,5 HEVC / 51,5 AV1), **Rapide >= 100 i/s** (mesuré 219) ; VRAM pic **avertissement > 3 Gio, FAIL > 9 Gio** (mesuré 1,5-2,1 Gio) ; 10 jobs : dérive <= 50 Mio.
- [ ] (GPU) VMAF des images `Generated` **>= 90,5 / 89,9 / 68,7** (BBB-FAST / BBB-PAN / SYN-TEXT), 1er centile >= celui de la duplication.
- [ ] Seul « Rapide » peut dégrader l'interpolation (décision n° 5) : variante dégradée mesurée et documentée (la marge mesurée de Rapide étant large, la dégradation n'est a priori pas nécessaire ; elle reste autorisée).
- [ ] Exposé dans `enhance-bench` (`--interp`).
Notes : dépend d'ENH-044 ; **l'interpolation après la super-résolution est écartée** (32 i/s, 3,9 Gio).

### ENH-013 — Conservation de l'audio, des sous-titres et des métadonnées  [GPU] [Must | 3 pts]
En tant qu'utilisateur, je veux garder mes pistes audio et sous-titres afin de ne rien perdre.
Critères d'acceptation :
- [ ] Étant donné un MKV avec 2 pistes audio et 2 sous-titres texte, quand on traite, alors la sortie contient les mêmes pistes (audio copié sans réencodage, durées identiques à +/- 40 ms).
- [ ] Conteneur MP4 par défaut ; sous-titres SRT/WebVTT -> `mov_text`, texte identique (décision n° 9) ; sous-titres ASS / PGS / VobSub, pièces jointes ou audio non MP4 -> bascule automatique en **MKV** avec message avant le traitement listant les pistes (décisions n° 2 et 9).
- [ ] Test : MKV avec PGS (ou ASS / VobSub) -> `.mkv` + message ; MKV avec SRT/WebVTT -> `.mp4`, texte identique.
- [ ] Sous-titres ASS / PGS / VobSub **copiés sans décodage** ; polices en pièces jointes copiées, **jamais chargées** ; paquet de sous-titre <= 64 Kio.
- [ ] Métadonnées de titre et de langue conservées ; **aucune chaîne personnelle** (chemin, e-mail) dans les tags (protocole §6).
- [ ] Synchro audio / vidéo vérifiée sur un clip de 10 min : décalage < 40 ms.

### ENH-014 — Budget VRAM et absence de fuite  [GPU] [Must | 3 pts]
En tant que PO, je veux que l'app tienne dans 12 Go afin de continuer à utiliser mon PC.
Critères d'acceptation :
- [ ] `docs/architecture.md` fixe le budget par étape, **somme <= 9 Gio** (limite FAIL), avec la **valeur mesurée** à côté (chaîne 1,5-2,1 Gio ; décodage ~0,6 Gio ; pire encodeur UHQ AV1 2,3 Gio ; reste < 4,5 Gio avec tout) et le seuil d'avertissement à 3 Gio.
- [ ] (GPU) 10 fichiers à la suite (Équilibré puis Qualité max) : **dérive VRAM <= 50 Mio** entre le 1er et le 10e, pente non régulièrement positive ; dérive mémoire système <= 100 Mio ; handles et threads : dérive nulle.
- [ ] La VRAM libre est lue au démarrage d'un job (le bureau en occupe 2 à 3,8 Gio au repos) ; si elle est insuffisante : erreur `OutOfVram` lisible (P-08), pas de plantage.
- [ ] Annulation puis nouveau traitement : VRAM revenue au repos de la boucle (+/- 50 Mio).

### ENH-015 — Robustesse face aux fichiers malformés  [GPU] [Must | 5 pts]
En tant qu'utilisateur, je veux qu'un fichier corrompu n'arrête pas la file afin de laisser tourner pendant la nuit.
Critères d'acceptation :
- [ ] Étant donné 10 fichiers tronqués ou corrompus (corpus généré par script), alors aucun plantage : chaque job finit en « Échec » avec un message P-08 en français ; un fichier corrompu au milieu d'une file ne bloque pas les suivants.
- [ ] Aucune exception ne traverse l'API (`enh::Expected` / codes d'erreur).
- [ ] **Limites** vérifiées au sondage (avant toute allocation) : largeur et hauteur 16 à 8192, pixels <= 8192x4320, <= 64 pistes, durée <= 24 h, cadence 1-240 i/s, `probesize` <= 50 Mio, `analyzeduration` <= 30 s, `av_max_alloc` <= 1 Gio, échéance dure de sondage de **2 s** ; refus -> `UnsupportedFormat`.
- [ ] **Fuzzing d'intégration** (`tests/fuzz/probe_fuzz.cpp`, SEC-09 / SEC-13) : 2 min en CI sur le build sans GPU, campagne d'1 h en local avant release ; critères : aucun plantage, aucune fuite (ASan), toute erreur = `PipelineError` attendu, <= 2 s par fichier ; tout plantage reproduit devient un test de non-régression.
- [ ] Cas forgés : MOV avec `dref` externe, liste HLS renommée `.mp4`, script `concat`, fichier de 0 octet, 1000 pistes, dimensions 65535x65535 : **aucune ouverture hors du fichier testé** (journal de l'`io_open` maison) ni connexion.
- [ ] `av_dump_format` interdit, aucune métadonnée du fichier dans le journal (SEC-14).
- [ ] Cas limites du protocole §8 (1 à 10) exécutés par la QA.
- [ ] **Test UNC exécuté sur un vrai partage** (reporté d'ENH-008, décision PO 2026-10-23) : `Win32File.PartageUnc` avec `ENH_TEST_UNC_DIR` pointant sur un partage réel, ouverture par chemin UNC réussie (aucun « non exécuté »).
- [ ] **SEC-23 (suite d'ENH-008)** : liste blanche de décodeurs côté application (seuls les codecs de la matrice sont ouverts ; tout autre -> `UnsupportedFormat`), `format_whitelist` des démuxeurs positionnée à l'ouverture, `max_pixels` borné sur le contexte FFmpeg ; tests unitaires avec fichiers forgés (codec hors liste, démuxeur hors liste, image > 8192x4320).
- [ ] **Fin de SEC-13** : tous les autres calculs de tailles du sondage et du pipeline (hors décodeur, déjà couvert par ENH-008) utilisent `checked_mul` / `std::size_t` ; revue ciblée par le reviewer.
- [ ] **Fichier tronqué décodé en partie** : si le décodage s'arrête avant la fin annoncée (images décodées < images attendues), le job se termine avec un **avertissement visible** à l'utilisateur (« le fichier semble tronqué : N images sur M traitées », texte P-08), jamais en succès silencieux ; test avec un clip tronqué de la matrice.
Notes : dépend d'ENH-008 ; fuzz du build d'ENH-048 ; isolement du sondage dans un processus restreint : ENH-051 (Plus tard).

## Epic C — File d'attente et préréglages

### ENH-016 — Modèle de la file d'attente  [Must | 5 pts]
En tant que développeur, je veux un modèle de file côté C++ afin que l'UI ne contienne aucune logique.
Critères d'acceptation :
- [ ] États d'un job : En attente, En cours, En pause, Terminé, Échec, Annulé ; transitions testées en unitaire (**>= 90 % des chemins**), `transition(état, événement)` pure.
- [ ] Ajout, suppression, déplacement (interdit pour le job en cours), doublons sur chemin normalisé insensible à la casse : testés sans GPU (`FakePipeline`, `ManualExecutor`).
- [ ] Exposé en `QAbstractListModel` ; notifications fines (`dataChanged` limité aux rôles modifiés) ; `QAbstractItemModelTester` en mode fatal.
- [ ] Probe du fichier via `IMediaProbe` en **moins de 1 s** par fichier (échéance dure 2 s) ; vignettes par un fournisseur asynchrone hors thread UI.
- [ ] SEC-15 : seuls les chemins locaux sont acceptés par le cœur (URL, `.lnk`, `.url`, `.scf`, `.library-ms`, dossiers : refus avec motif).

### ENH-017 — Glisser-déposer et ajout à la file  [UI] [Must | 5 pts]
En tant qu'utilisateur, je veux glisser des vidéos dans la fenêtre afin de les ajouter à la file.
Critères d'acceptation :
- [ ] Étant donné la fenêtre vide, quand on survole avec des fichiers, alors la zone de dépôt réagit en **moins de 100 ms** (bordure accentuée, agrandissement 1,02, animation 150-250 ms) et affiche « Relâcher pour ajouter N vidéos » (P-02).
- [ ] Le dépôt de 20 fichiers les ajoute en **moins de 2 s** : cartes en cascade (30 ms d'écart, ressort 250 ms) avec vignette, nom, durée, résolution, cadence ; état « Lecture… » (squelette animé) pendant le sondage ; bouton principal « Démarrer (N) ».
- [ ] Messages non bloquants (6 s, puis lien « N fichiers n'ont pas été ajoutés ») : fichier non vidéo, illisible, **doublon** (carte existante surlignée 400 ms), dossier (« Les dossiers ne sont pas encore pris en charge… »), fichier verrouillé, textes exacts de `parcours.md` P-02.
- [ ] **SEC-15** : `QUrl::isLocalFile()` obligatoire (URL glissée depuis un navigateur refusée avec message), `.lnk` / `.url` refusés.
- [ ] État vide illustré (« Déposez vos vidéos ici », « Rien n'est envoyé sur Internet »), bouton « Choisir des fichiers… » (**Ctrl+O**) ; Ctrl+V pour coller des fichiers copiés **[H, faisabilité à confirmer]**.
- [ ] Source déjà en 4K : pastille « Déjà en 4K » ; interpolation : « 30 -> 60 images/s » ou « 60 images/s conservées » sur la carte (P-03).
- [ ] Critères UX transverses.

### ENH-018 — Préréglages et choix du codec  [UI] [Must | 5 pts]
En tant qu'utilisateur, je veux choisir « Qualité max », « Équilibré » ou « Rapide » afin de ne pas toucher aux réglages techniques.
Critères d'acceptation :
- [ ] Panneau Préréglage visible dès qu'il y a un fichier : contrôle segmenté **Qualité max · Équilibré · Rapide**, « Équilibré » par défaut ; contrôle « Format » **Compatible (HEVC)** / **Compact (AV1)**, sigles en légende secondaire, phrase d'explication (P-03).
- [ ] Description d'une ligne par préréglage (« Rapide » : « Plus rapide, finesse un peu moindre » ; seul « Rapide » peut dégrader l'interpolation, décision n° 5) ; **estimation de durée** affichée pour la liste et par fichier (« ≈ 6 min »), mise à jour avec animation de nombre (150 ms) ; l'estimation s'appuie sur les i/s mesurés (Équilibré ~105, Qualité max ~37 / 52, Rapide ~219).
- [ ] Le choix s'applique à toute la liste, uniquement aux jobs **non commencés** (les cartes lancées portent leur préréglage en légende) ; mémorisé entre deux lancements.
- [ ] AV1 grisé avec info-bulle si la carte ne le supporte pas (ENH-011).
- [ ] Définition des préréglages (niveau de VSR, interpolation, 10 bits, débit, préréglage NVENC) dans un **JSON versionné** embarqué (`schemaVersion`, validation, migrations) ; valeurs initiales : `docs/gpu/architecture-pipeline.md` §4 corrigées par les mesures ; **SEC-18** : taille <= 1 Mio, profondeur <= 32, bornes validées, valeurs inconnues ignorées, fichier invalide -> défauts + `.bak`.
- [ ] Emplacement « Avancé » réservé (aucun bouton grisé « Bientôt »).
- [ ] Critères UX transverses ; navigation entièrement au clavier.

### ENH-019 — Exécution séquentielle de la file  [GPU] [Must | 5 pts]
En tant qu'utilisateur, je veux que les fichiers se traitent l'un après l'autre afin de lancer et partir.
Critères d'acceptation :
- [ ] Étant donné 3 fichiers, « Démarrer » les traite dans l'ordre, un seul à la fois ; un échec ou une annulation n'empêche pas le suivant.
- [ ] (GPU) file de 10 fichiers de 30 s (Équilibré) : tous terminés, dérive VRAM <= 50 Mio.
- [ ] À la fin, notification discrète (non bloquante) avec récapitulatif « N vidéos terminées » (+ échecs, « Voir les erreurs ») ; **choix toast / repli dans l'app** décidé ici (ADR-004, risque A5).
- [ ] Prévérifications (< 1 s) : espace disque estimé, VRAM libre, prérequis ; erreurs P-08.
Notes : dépend d'ENH-009 / 010 / 012 / 016.

### ENH-020 — Progression : images/s et temps restant  [UI] [Must | 5 pts]
En tant qu'utilisateur, je veux voir où en est le traitement afin d'estimer l'attente.
Critères d'acceptation :
- [ ] Barre de progression par fichier et globale ; images/s et temps restant mis à jour **au plus toutes les 500 ms** ; pied de fenêtre « 3 sur 12 terminées · il reste environ 2 h 10 » (P-05).
- [ ] « Estimation en cours… » pendant les 10 premières secondes ; ensuite temps restant lissé, **erreur <= 15 %** ; étapes lisibles « Lecture · Agrandissement · Fluidité · Enregistrement ».
- [ ] La vitesse réelle est toujours affichée, y compris sous le temps réel (« 0,6x la vitesse de lecture » dans le détail, non alarmant) (décision n° 5).
- [ ] **Première utilisation** (P-08b) : carte « Préparation de la fluidité… », « Première utilisation seulement. Cela prend 1 à 3 minutes, puis ce sera instantané. », barre **indéterminée** (ni pourcentage, ni i/s, ni temps restant) ; > 5 min : « Cela prend plus de temps que prévu. Enhance travaille toujours. » ; annulable (« Préparation interrompue. Elle reprendra au prochain lancement. ») ; bascule en douceur vers la progression normale.
- [ ] (GPU) animation de l'interface **>= 60 i/s** pendant un traitement 4K Qualité max (99e centile <= 16,7 ms) ; progression du pipeline à 4 Hz regroupée (ADR-002).
- [ ] Narrateur : <= 1 annonce / 5 s et à chaque changement de fichier ; lecture d'une carte « en cours, 38 pour cent, il reste environ 9 minutes ».
- [ ] Critères UX transverses.

### ENH-021 — Pause et annulation  [UI] [GPU] [Must | 5 pts]
En tant qu'utilisateur, je veux mettre en pause ou annuler afin de reprendre la main sur mon PC.
Critères d'acceptation :
- [ ] Pause (décision n° 8) : la **VRAM reste réservée** (valeur mesurée du job, 1,5-2,1 Gio) ; calcul GPU arrêté en **< 2 s** ; reprise **< 1 s** ; nombre d'images final identique à un traitement sans pause ; carte « En pause à 38 % » (barre grisée).
- [ ] Annulation : arrêt en **< 3 s**, VRAM libérée, fichier partiel supprimé **à l'expiration** du délai ; bandeau « Traitement annulé. [Reprendre depuis le début] » pendant 5 s (10 s si le focus est sur le bandeau pour le Narrateur) ; **Ctrl+Z** équivalent ; aucune boîte de confirmation.
- [ ] Raccourcis : Espace (pause), Échap (annuler le job en cours).
- [ ] (GPU) 20 cycles pause / reprise : aucune fuite VRAM.
- [ ] Quitter (décision n° 6) : la croix / Alt+F4 réduit l'app, le traitement continue (i/s inchangées) ; Ctrl+Q ouvre la feuille « Quitter et abandonner le traitement en cours ? » (Annuler / Quitter) ; confirmer = annulation propre (VRAM libérée, partiel supprimé, file sauvegardée) ; sans traitement, Ctrl+Q quitte directement. **Seule confirmation de l'app.**
- [ ] Fermeture de session Windows : file sauvegardée puis annulation propre.
- [ ] Critères UX transverses.
Notes : `parcours.md` P-06 à corriger par l'ux-designer (VRAM conservée).

### ENH-022 — Réordonnancement et persistance de la file  [UI] [Must | 5 pts]
En tant qu'utilisateur, je veux déplacer des fichiers dans la file et la retrouver au prochain lancement afin de choisir ce qui passe en premier.
Critères d'acceptation :
- [ ] Glisser une carte « En attente » par sa poignée la déplace avec animation (ressort 250 ms) ; alternative clavier **Alt+↑ / Alt+↓** ; le job en cours n'a pas de poignée (info-bulle « Le fichier en cours ne peut pas être déplacé »).
- [ ] L'ordre est conservé après redémarrage : `queue.json` écrit de façon **atomique** (fichier temporaire + `MoveFileExW`), anti-rebond 300 ms, jamais à chaque échantillon de progression ; fichier corrompu -> défauts + `.bak`.
- [ ] Au redémarrage, un job « En cours » ou « En pause » repasse « En attente » et son temporaire **référencé** est supprimé.
- [ ] **SEC-11** : avant toute suppression, le nom finit par `.enhance-tmp.<mp4|mkv>`, c'est un fichier régulier (pas un point d'analyse) et le chemin égale celui recalculé par `OutputPathPlanner` ; test d'un `queue.json` forgé.
- [ ] **SEC-10** : aucun sondage ni vignette automatique des chemins **UNC** au redémarrage (carte avec le nom seul, sondage au démarrage effectif du job).
- [ ] **SEC-18** : `queue.json` / `settings.json` <= 1 Mio, profondeur <= 32, schéma validé, exceptions `nlohmann::json` converties en `Expected`.
- [ ] Retrait d'un fichier : « Retiré de la liste. [Annuler] » 5 s, le fichier d'origine n'est jamais supprimé.
- [ ] Critères UX transverses.

### ENH-023 — Ouvrir le fichier produit  [UI] [Must | 2 pts]
En tant qu'utilisateur, je veux ouvrir le résultat afin de le vérifier tout de suite.
Critères d'acceptation :
- [ ] Carte Terminée : coche animée (300 ms, ressort), « Terminé en 18 min · 4K 60 i/s · 2,3 Gio », « Votre fichier d'origine n'a pas été modifié » ; boutons « Lire » et « Afficher dans le dossier » (Explorateur, fichier sélectionné) ; « Vider les terminées » annulable 5 s (P-07).
- [ ] Fichier déplacé ou supprimé : boutons grisés, « Le fichier n'est plus à cet emplacement. »
- [ ] **SEC-15** : `ShellExecuteExW` uniquement sur le fichier **produit** (extension `.mp4` / `.mkv`, chemin local existant, verbe `open`), jamais sur un chemin venu du QML.
- [ ] Critères UX transverses (raccourci « Afficher dans le dossier » : voir point ouvert Ctrl+Entrée).

### ENH-024 — Dossier de sortie, nommage et réglages  [UI] [Must | 3 pts]
En tant qu'utilisateur, je veux savoir où sont mes fichiers afin de les retrouver.
Critères d'acceptation :
- [ ] Par défaut, sortie à côté de la source avec le suffixe `_Enhance_4K60` ; **jamais d'écrasement** (`_2`, `_3`) ; création du fichier en `CREATE_NEW`, renommage final sans remplacement ; extension `.mp4` ou `.mkv` imposée par le plan du pipeline.
- [ ] Feuille « Réglages » (Ctrl+,) : dossier de sortie (« À côté de la vidéo d'origine » / « Choisir un dossier… »), apparence, « Afficher les journaux », « À propos » (ENH-050) ; aucun réglage de codec.
- [ ] Cas de test unitaires : accents, espaces, > 260 caractères, UNC, noms réservés (`CON`, `NUL`), `:` hors lettre de lecteur / flux de données alternés, point ou espace final, deux jobs de même nom vers le même dossier.
- [ ] Espace disque insuffisant : avertissement avant le démarrage (estimation de taille) avec le message P-04 ; **SEC-11** : nettoyage limité aux temporaires référencés.
- [ ] Critères UX transverses.

### ENH-025 — Journalisation locale  [Must | 2 pts]
En tant que développeur, je veux des journaux locaux afin de diagnostiquer sans réseau.
Critères d'acceptation :
- [ ] Journal fichier tournant (5 x 2 Mio) dans `%LOCALAPPDATA%\Enhance\logs`, niveaux Info / Avertissement / Erreur (spdlog asynchrone, façade `enh::log`).
- [ ] Aucun chemin complet, aucune donnée personnelle par défaut (`fileNameOnly`) ; `--verbose` documenté comme « contient des chemins ».
- [ ] **SEC-14** : `av_log` filtré (<= `AV_LOG_WARNING`, pas d'URL ni de chemin), journal TensorRT à `kWARNING`, journal NGX désactivé, messages Qt redirigés ; **décision WER** (« Questions pour le PO » n° 5) appliquée (`WerAddExcludedApplication`).
- [ ] Bouton « Afficher les journaux » dans les réglages (ouvre le dossier dans l'Explorateur).

### ENH-026 — Thèmes sombre et clair, accessibilité  [UI] [Must | 5 pts]
En tant qu'utilisateur, je veux un thème sombre soigné et un thème clair complet afin de travailler confortablement.
Critères d'acceptation :
- [ ] Sombre par défaut ; « Suivre le système » ; changement animé en **moins de 300 ms** sans scintillement.
- [ ] Contraste >= 4,5:1 (texte) et 3:1 (composants) dans les deux thèmes ; **contraste élevé Windows** (couleurs système).
- [ ] Mise à l'échelle 100, 125, 150, 200 % (et texte Windows jusqu'à 225 %) : rien de tronqué, rendu net.
- [ ] Clavier complet, focus visible, noms accessibles pour Narrateur (7 états de la carte).
- [ ] « Réduire les animations » respecté (`SPI_GETCLIENTAREAANIMATION`) : durées à 0.
- [ ] Parcours P-01 à P-08 réalisables **sans souris** ; checklist `docs/qa/checklist-ui.md` à jour.

### ENH-027 — Interface en français  [UI] [Must | 2 pts]
En tant qu'utilisateur francophone, je veux une interface en français afin de tout comprendre.
Critères d'acceptation :
- [ ] Toutes les chaînes via `qsTr` / `tr` et `enhance_fr.ts` ; **aucun texte en dur** en C++ ou QML ; le cœur ne produit aucun texte destiné à l'utilisateur.
- [ ] Formats français : « 1,2 Gio », « 3 min 05 s » ; version longue pour le Narrateur (« 3 minutes 5 secondes »).
- [ ] Revue de relecture des textes par l'ux-designer (vouvoiement discret, micro-textes humains, jamais de jargon de codec en façade).

### ENH-028 — Garde-fou « zéro réseau »  [Must | 3 pts]
En tant que PO, je veux la preuve que l'app n'émet rien afin de garantir la confidentialité.
Critères d'acceptation :
- [ ] Un test automatisé échoue si le binaire ou une DLL livrée importe des API réseau (`ws2_32`, `winhttp`, `wininet`, `QtNetwork`) hors liste blanche : **exception E2** limitée aux 3 imports de `Qt6Core.dll` (`WSAAsyncSelect`, `WSAStartup`, `WSACleanup`) (SEC-22), revalidation à chaque version de Qt et **au plus tard le 2027-04-21** ; `mpr` / `netapi32` en avertissements acceptés.
- [ ] Test **dynamique** (ENH-041) : application lancée sous un pare-feu bloquant tout, fonctionnement identique, 0 tentative de connexion journalisée (M1-M3 de `zero-reseau.md`) ; exception SMB documentée (partages choisis par l'utilisateur, jamais de reconnexion automatique).
- [ ] Qt déployé sans module Network ; aucune exception « dev-only » d'FFmpeg (E3 / E3c / E3d / E3e) au moment de `-Package` (ENH-048).
- [ ] Règle de code : ne jamais énumérer de partages réseau de façon proactive.

### ENH-029 — Banc d'essai qualité automatisé vs Lanczos et Broadcast  [Must | 5 pts]
En tant que qa-tester, je veux un rapport automatique afin de suivre les seuils à chaque story.
Critères d'acceptation :
- [ ] Une commande produit le tableau (VMAF, VMAF-NEG, SSIM, PSNR, i/s, VRAM pic) par clip et préréglage, avec les **seuils en part de la marge** (décision n° 14) et le verdict par ligne.
- [ ] Comparaison avec Lanczos systématique et avec **Broadcast 2.3 sur >= 2 clips** (jeu B ; processus manuel documenté ; rapport de référence `2026-10-21_reference-broadcast.md` intégré).
- [ ] **1 ou 2 extraits des archives du PO** (décision n° 3, reliquat d'ENH-005) : décrits sans chemin personnel, jamais commités ; comparaison visuelle à l'aveugle avec Broadcast / ancien pipeline.
- [ ] Alerte si régression > 5 % par rapport à la dernière mesure de `main` (baseline `docs/qa/resultats/baseline-main.json`, produite à la première story `[GPU]` mergée) ; mesure instable (> 3 %) signalée, jamais masquée.
- [ ] S'appuie sur `enhance-bench` (ENH-047) ; résultats dans `docs/qa/` ; clips non commités.

### ENH-030 — Version portable pour la démo  [Must | 5 pts]
En tant que PO, je veux un dossier à lancer afin de tester sur ma machine sans environnement de développement.
Critères d'acceptation :
- [ ] `scripts/package.ps1` produit un dossier / zip avec **dossier racine** `Enhance-x.y.z\`, l'exécutable et ses DLL, démarrant sur une machine sans Visual Studio ; CRT VC++ v145 **app-local** ; `qt.conf` livré ; LISEZ-MOI (« extraire dans un dossier dédié ») + SHA-256.
- [ ] **Préalable : ENH-048** (FFmpeg minimal sans réseau) ; FFmpeg en DLL (LGPL, liaison dynamique), `NOTICE-SOURCES.txt` indique comment obtenir les sources et la configuration (décision n° 4).
- [ ] DLL NVIDIA livrées : `rel/nvngx_vsr.dll` ; TensorRT **uniquement** `nvinfer_11`, `nvinfer_builder_resource_sm120_11`, `nvonnxparser_11` (+ `nvinfer_plugin_11` si utilisé) dans `nvidia\tensorrt\`, **non modifiées**, avec `Acknowledgements.txt` (SEC-02) ; **jamais** `NvOFFRUC.dll`, `cudart64_110.dll`, `trtexec`, `.lib`, moteur pré-construit, Python, ONNX hors verrou (SEC-04, ENH-043) -> `package.ps1` **échoue** si l'un est présent.
- [ ] Toutes les DLL redistribuées figurent dans `licences.md` avec « redistribuable : oui » ; `THIRD-PARTY-LICENSES.txt` généré ; `LICENSES\` (LGPL-2.1, LGPL-3.0 + GPL-3.0, MIT / BSD, licence d'Enhance).
- [ ] **SEC-16** : `/PDBALTPATH:%_PDB%`, `source_location` tronqué au dossier `src/`, contrôle de chaînes `C:\Users\` dans le livrable ; `check-no-network.ps1 -Package` vert sur le dossier livré.
- [ ] CLUF (ENH-050) et attribution NVIDIA présents.

### ENH-031 — Détection des prérequis (carte, pilote)  [UI] [Must | 3 pts]
En tant qu'utilisateur, je veux un message clair si ma machine n'est pas compatible afin de comprendre pourquoi.
Critères d'acceptation :
- [ ] Au lancement (< 1 s, hors thread UI), détection de la carte (RTX 50 : capacité de calcul >= 12.0) et du pilote ; si tout va bien : ligne discrète « GeForce RTX 5070 · prête » (P-01).
- [ ] Pilote trop ancien : bandeau « Le pilote graphique est trop ancien (version N). Enhance a besoin de la version M ou plus récente. » + feuille d'aide locale (aucun lien réseau automatique) ; carte non compatible / absente : **lecture seule** (ajout de fichiers possible, **Démarrer désactivé**), sans plantage ; composant manquant (DLL, modèle) : « Un composant d'Enhance est manquant ou endommagé. » + « Copier le détail ».
- [ ] La version minimale affichée est celle **tranchée par le gpu-engineer** (point ouvert : 550 / 570 / 580 / 617.42).
- [ ] Tests unitaires de `PrereqEvaluator` avec `FakeGpuInfoSource` (CI sans GPU) ; DLL du pilote chargées depuis System32 seulement.
- [ ] Critères UX transverses ; jamais de blocage plein écran.

---

# Plus tard (vision long terme, visible pour l'architecture)

Contraintes d'architecture à tenir dès le MVP : étapes du pipeline en modules chaînés (ajout d'une étape HDR sans réécriture), description d'un job sérialisable (reprise, dossiers), préréglages en données, conteneur de sortie paramétrable, décodeur capable de fournir des images 10 bits, RIFE capable de travailler en RGB flottant (entrée P010 sans perte).

### ENH-032 — SDR -> HDR10 (RTX Video SDK)  [GPU] [UI] [Should | 8 pts]
Sortie HEVC/AV1 10 bits HDR10 avec métadonnées (`MASTERING_DISPLAY_INFO`, `CONTENT_LIGHT_LEVEL` disponibles dans NVENC 13.1, HEVC et AV1) ; case à cocher dans le panneau Préréglage (« Couleurs HDR ») ; TrueHDR mesuré au spike : 4K -> 4K **737 i/s, 164 Mio** ; validation visuelle sur un écran HDR.

### ENH-033 — Comparaison avant / après côte à côte  [UI] [GPU] [Should | 8 pts]
Curseur de comparaison sur une image ou quelques secondes ; 60 i/s d'animation ; lecture synchronisée ; bouton « Comparer » sur la carte Terminée.

### ENH-034 — Préréglages personnalisés  [UI] [Could | 5 pts]
Créer, dupliquer, renommer, supprimer ; stockage local versionné ; **les fichiers de préréglages importés sont une entrée non fiable** (SEC-18).

### ENH-035 — Options avancées : débit, résolution, cadence cibles, MP4/MKV  [UI] [GPU] [Could | 8 pts]
Panneau masqué par défaut, valeurs validées côté C++.

### ENH-036 — Aperçu rapide sur quelques secondes  [UI] [GPU] [Should | 5 pts]
Traitement de 5 s choisies en moins de 15 s **[H]** avant de lancer le fichier complet (la construction du moteur n'est payée qu'une fois).

### ENH-037 — Traitement de dossiers entiers  [UI] [Could | 5 pts]
Dépôt d'un dossier, parcours récursif, filtre des vidéos, structure conservée.

### ENH-038 — Reprise après plantage  [GPU] [Should | 8 pts]
Points de reprise tous les N segments ; reprise propre au redémarrage ; test par arrêt brutal.
- [ ] Libération de la VRAM pendant une pause (décision n° 8) : VRAM libérée pendant la pause, reprise sans perte d'images.

### ENH-039 — Installeur Windows propre  [Should | 5 pts]
Installeur Inno Setup par utilisateur (`PrivilegesRequired=lowest`), désinstallation propre (ne touche jamais aux vidéos ; `%LOCALAPPDATA%\Enhance` seulement sur demande), aucun accès réseau ; signé ou non (décision PO) ; devops + secdevops ; permet le toast Windows (identifiant d'application).

### ENH-051 — Sondage des fichiers dans un processus restreint  [GPU] [Should | 8 pts]
Le sondage et les vignettes (entrées non fiables) s'exécutent dans un processus enfant à jeton restreint (AppContainer ou intégrité basse), redémarré à chaque plantage (SEC-09, risque R-1) ; test : un fichier piégé qui fait planter FFmpeg n'arrête pas l'application.

Total « Plus tard » : 60 pts (ENH-032 à ENH-039 : 52 pts + ENH-051 : 8 pts).

---

# Avant toute publication publique (hors MVP)

### ENH-052 — Check-list avant diffusion publique  [Should | 3 pts]
En tant que PO, je veux une liste de contrôle complète avant de publier afin de n'enfreindre aucune licence ni aucun brevet. Les éléments suivants **bloquent la publication, pas la démo** :
- [ ] **Notifier NVIDIA** avant la sortie (Supplément RTX art. 4 : RTX Video SDK / NGX), forme exacte de la mention de marque (art. 7.1), aucun logo ni usage marketing sans accord.
- [ ] **CLUF d'Enhance** (SEC-21) relu (idéalement par un juriste) ; pas de licence copyleft sur l'ensemble.
- [ ] **Brevets HEVC / AV1** (R-6) : les licences NVIDIA laissent cette responsabilité au développeur.
- [ ] **WER** : décision appliquée et vérifiée (SEC-14).
- [ ] Signature des binaires ou SHA-256 publiée (SEC-19, ADR-034) ; SmartScreen.
- [ ] Licences exactes de *Crowd Run* et des œuvres Netflix à confirmer avant toute publication d'extrait ; offre LGPL (sources de Qt et FFmpeg) en place ; « Keep my email addresses private » activé sur GitHub.
- [ ] Éventuel accord écrit de NVIDIA pour `NvOFFRUC.dll` (seulement si le PO veut livrer FRUC).
