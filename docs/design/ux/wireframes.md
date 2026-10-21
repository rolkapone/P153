# Enhance — Wireframes basse fidélité

Statut : v0.1 (ux-designer). Fenêtre de référence : 1100 × 720 px, minimum 760 × 520. Pas de barre de menus ; barre de titre Windows avec contrôles standard. Les « ▓░ » figurent une barre de progression.

Vue d'ensemble : liste au centre, panneau Préréglage collé en bas, barre d'état en pied.

---

## W1 — Premier lancement / état vide

```
┌──────────────────────────────────────────────────────────────┐
│ Enhance                                               ─  □  ✕│
├──────────────────────────────────────────────────────────────┤
│                                                      [ ⚙ ]   │
│                                                              │
│   ┌ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┐   │
│                                                              │
│   │                      [ ▭↑ ]                         │   │
│                  Déposez vos vidéos ici                      │
│   │   Enhance les passe en 4K fluide, sur votre         │   │
│       ordinateur. Rien n'est envoyé sur Internet.            │
│   │                                                     │   │
│                  [ Choisir des fichiers… ]                   │
│   │                                                     │   │
│   └ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┘   │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│ ● GeForce RTX 5070 · prête                                   │
└──────────────────────────────────────────────────────────────┘
```
Panneau Préréglage absent. Raison : rien à régler tant que la liste est vide.

### W1b — Prérequis manquant (pilote)
```
├──────────────────────────────────────────────────────────────┤
│ ⚠ Le pilote graphique est trop ancien (version 551).         │
│   Enhance a besoin de la version 580 ou plus récente.        │
│                                    [ Comment mettre à jour ] │
├──────────────────────────────────────────────────────────────┤
│        (zone de dépôt identique, active)                     │
```
Bouton Démarrer désactivé plus tard, info-bulle : « Mettez à jour le pilote pour démarrer. »

### W1c — Survol de dépôt
```
│   ╔══════════════════════════════════════════════════════╗   │
│   ║                      [ ▭↓ ]  (grossit à 1,02)         ║   │
│   ║           Relâcher pour ajouter 12 vidéos             ║   │
│   ╚══════════════════════════════════════════════════════╝   │
```
Bordure pleine accent, fond accent à 12 %, transition 150-250 ms.

---

## W2 — Liste en attente (fichiers ajoutés, avant lancement)

```
┌──────────────────────────────────────────────────────────────┐
│ Enhance                                               ─  □  ✕│
├──────────────────────────────────────────────────────────────┤
│  À traiter · 3 vidéos                [ + Ajouter ] [ ⚙ ]    │
│ ┌──────────────────────────────────────────────────────────┐ │
│ │⋮⋮ ┌─────┐ mariage_1998.avi                        [ ✕ ] │ │
│ │   │vign.│ 12 min 04 s · 720 × 576 · 25 i/s             │ │
│ │   └─────┘ 25 → 60 images/s            ≈ 14 min          │ │
│ ├──────────────────────────────────────────────────────────┤ │
│ │⋮⋮ ┌─────┐ vacances_ete.mkv   [MKV]                [ ✕ ] │ │
│ │   │vign.│ 48 min · 1280 × 720 · 30 i/s                 │ │
│ │   └─────┘ 30 → 60 images/s  ⓘ sous-titres  ≈ 52 min    │ │
│ ├──────────────────────────────────────────────────────────┤ │
│ │⋮⋮ ┌─────┐ capture_zelda.mp4                       [ ✕ ] │ │
│ │   │vign.│ 1 h 02 · 1280 × 720 · 60 i/s                 │ │
│ │   └─────┘ 60 images/s conservées        ≈ 1 h 08        │ │
│ └──────────────────────────────────────────────────────────┘ │
│  2 fichiers n'ont pas été ajoutés ⌄                          │
├──────────────────────────────────────────────────────────────┤
│  Préréglage                                                  │
│  ( Qualité max | ▌Équilibré▐ | Rapide )                     │
│  Le bon compromis entre netteté et durée.                    │
│                                                              │
│  Format     ( ▌Compatible▐ | Compact )   HEVC · AV1          │
│  Lisible presque partout.                                    │
│                                                              │
│  Environ 2 h 14 pour l'ensemble        [   Démarrer (3)   ]  │
├──────────────────────────────────────────────────────────────┤
│ ● GeForce RTX 5070 · prête                                   │
└──────────────────────────────────────────────────────────────┘
```
Notes : le bouton principal est le seul élément plein accent. Le panneau se replie en une ligne (« Équilibré · Compatible ⌃ ») si la fenêtre est basse (< 600 px).

---

## W3 — En cours

```
├──────────────────────────────────────────────────────────────┤
│  En cours                                                    │
│ ┌──────────────────────────────────────────────────────────┐ │
│ │   ┌───────┐ mariage_1998.avi                              │ │
│ │   │ vign. │ ▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░  38 %                   │ │
│ │   │       │ 52 images/s · il reste environ 9 min          │ │
│ │   └───────┘ Étape : Agrandissement en 4K                  │ │
│ │             [ Pause ]  [ Annuler ]                        │ │
│ └──────────────────────────────────────────────────────────┘ │
│  À suivre · 2                                                │
│ ⋮⋮ vacances_ete.mkv         ≈ 52 min                  [ ✕ ] │
│ ⋮⋮ capture_zelda.mp4        ≈ 1 h 08                  [ ✕ ] │
├──────────────────────────────────────────────────────────────┤
│  Équilibré · Compatible (appliqué aux vidéos à venir)  [Pause]│
├──────────────────────────────────────────────────────────────┤
│ 0 sur 3 terminées · il reste environ 2 h 05                  │
└──────────────────────────────────────────────────────────────┘
```
Variante « Estimation en cours… » les 10 premières secondes à la place du temps restant.

### W3b — En pause
```
│ │ ▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░ (grisée)   En pause à 38 %          │
│ │ [ Reprendre ]  [ Annuler ]                                │
```

### W3c — Annulé (5 s)
```
│ ┌──────────────────────────────────────────────────────────┐ │
│ │ mariage_1998.avi    Traitement annulé.      [ Rétablir ]  │ │
│ └──────────────────────────────────────────────────────────┘ │
```
Après 5 s : « Annulé » + [ Relancer ] [ ✕ ].

---

## W4 — Terminé

```
│  Terminées · 3                                               │
│ ┌──────────────────────────────────────────────────────────┐ │
│ │ ✓ ┌─────┐ mariage_1998.avi                              │ │
│ │   │vign.│ Terminé en 14 min · 4K 60 i/s · 2,3 Gio       │ │
│ │   └─────┘ Votre fichier d'origine n'a pas été modifié.  │ │
│ │           [ Lire ]  [ Afficher dans le dossier ]          │ │
│ └──────────────────────────────────────────────────────────┘ │
├──────────────────────────────────────────────────────────────┤
│ 3 vidéos terminées.        [ Tout afficher dans le dossier ] │
└──────────────────────────────────────────────────────────────┘
```
[LT] Un bouton « Comparer » vient se placer entre Lire et Afficher.

### W4b — Terminé avec échecs
```
│ ┌──────────────────────────────────────────────────────────┐ │
│ │ ⚠ ┌─────┐ vieille_cassette.avi                          │ │
│ │   │vign.│ Ce fichier est endommagé à partir de 12 min   │ │
│ │   └─────┘ 40 s et n'a pas pu être traité.               │ │
│ │           [ Réessayer ]  [ Détails ⌄ ]  [ Retirer ]      │ │
│ └──────────────────────────────────────────────────────────┘ │
│ 2 vidéos terminées, 1 en échec.      [ Voir les erreurs ]    │
```

---

## W5 — Panneau de réglages (feuille ancrée à la fenêtre, Ctrl+,)

```
┌──────────────────────────────────────────────────────────────┐
│ Réglages                                              [ OK ] │
├──────────────────────────────────────────────────────────────┤
│  Dossier de sortie                                           │
│   ( ▌À côté de la vidéo d'origine▐ | Choisir un dossier… )   │
│   Les fichiers se terminent par « _Enhance_4K60 ».           │
│                                                              │
│  Apparence                                                   │
│   ( Suivre le système | Sombre | Clair )                     │
│                                                              │
│  Journaux                                                    │
│   Aide à diagnostiquer un problème. Conservés sur            │
│   cet ordinateur uniquement.        [ Afficher les journaux ]│
│                                                              │
│  À propos d'Enhance                                          │
│   Version 0.1 · Aucune donnée n'est envoyée hors de cet      │
│   ordinateur.                       [ Licences tierces ]     │
└──────────────────────────────────────────────────────────────┘
```
Les réglages s'appliquent immédiatement (pas de « Enregistrer ») ; [ OK ] ferme simplement la feuille (Échap aussi).
[LT] Une section « Avancé » (débit, résolution, cadence, conteneur) viendra sous Apparence, repliée.

---

## W6 — Détails de l'erreur (feuille)

```
┌─────────────────────────────────────────────┐
│ Pourquoi ce fichier a échoué                │
│                                             │
│ vieille_cassette.avi                        │
│ Le fichier est endommagé à partir de        │
│ 12 min 40 s. Les autres vidéos continuent.  │
│                                             │
│ ⌄ Détail technique                          │
│   [ code et message du journal, copiable ]  │
│                                             │
│        [ Copier le détail ]  [ Fermer ]     │
└─────────────────────────────────────────────┘
```

---

## W7 — Largeur réduite (< 900 px) et mise à l'échelle 200 %
Cartes sur une seule colonne, vignette 56 px, boutons de carte en icônes avec libellé en info-bulle **et** nom accessible. À 200 %, la fenêtre minimale (760 px logiques) tient dans 1520 px physiques ; le panneau Préréglage passe en lignes empilées, jamais de texte tronqué (retour à la ligne, pas d'ellipse sur les messages d'erreur).
