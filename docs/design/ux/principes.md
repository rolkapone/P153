# Enhance — Principes UX et accessibilité

Statut : v0.1 (ux-designer), 2026-10-21.

## Principes (8)

1. **Déposer, lancer, partir.** Le chemin entre « je dépose » et « j'ai ma 4K » tient en 2 gestes : déposer, Démarrer. Tout réglage a une valeur par défaut juste (Équilibré, Compatible). *Test : un nouvel utilisateur démarre un traitement en < 15 s sans lire quoi que ce soit.*
2. **Une fenêtre, peu d'éléments.** Pas d'assistant, pas d'onglets. La liste de vidéos est le centre ; tout ce qui n'a pas de raison d'être visible maintenant est replié ou absent (le panneau Préréglage n'apparaît que s'il y a des fichiers).
3. **Dévoilement progressif.** Les sigles (HEVC, AV1, MKV) restent en second plan ; les options avancées existent plus tard mais sont repliées. Le vocabulaire de façade est celui de l'usage : « Compatible », « Compact », « Qualité max ».
4. **Annuler plutôt que confirmer.** Aucune boîte « Êtes-vous sûr ? » : l'action s'exécute, un bandeau propose « Rétablir » ou « Annuler » pendant 5 s. Les opérations ne détruisent jamais le fichier d'origine. Seule exception (décidée par le PO le 2026-10-21) : quitter l'app pendant un traitement (Ctrl+Q ouvre une feuille ; la croix réduit l'app sans arrêter le traitement).
5. **Une progression honnête.** Images/s réelles, temps restant lissé, « Estimation en cours… » plutôt qu'un faux chiffre. La qualité prime sur la vitesse ; la vitesse est affichée sans embellissement ni alarmisme.
6. **Une erreur = quoi + que faire.** Chaque erreur dit en une phrase ce qui s'est passé, indique si le reste de la file est touché, propose une action. Détail technique replié et copiable. Jamais de code brut en façade, jamais de blocage de la file.
7. **Confiance et confidentialité visibles.** « Rien n'est envoyé sur Internet » est dit au bon endroit (état vide, À propos). Aucun lien réseau qui s'ouvre sans clic explicite de l'utilisateur ; aucune demande de compte.
8. **Du plaisir aux moments clés, jamais gratuit.** Trois moments soignés : le dépôt (cascade des cartes), la fin d'un traitement (coche animée), la comparaison avant / après (plus tard). Le reste est calme : animations 150–350 ms, utilitaires, désactivables.

## Règles d'interaction transverses
- Un seul bouton plein accent à l'écran (l'action principale).
- Le clavier atteint tout ; chaque action de souris a un équivalent clavier (voir `ecrans.md`).
- Les messages passagers ne prennent pas le focus et ne cachent pas la liste.
- Les nombres sont formatés à la française (« 1,2 Gio », « 3 min 05 s »).
- Tutoiement exclu : vouvoiement discret ou formes impersonnelles.
- La mémorisation (préréglage, format, dossier, thème, ordre de la file) est silencieuse.

## Accessibilité (WCAG 2.2 AA adaptée au bureau)

| Domaine | Exigence |
|---|---|
| Clavier | Toute fonction accessible au clavier seul ; ordre de tabulation = ordre visuel (Liste → Panneau Préréglage → Bouton principal → Réglages) ; pas de piège clavier ; Échap ferme les feuilles et restitue le focus à l'élément d'origine. |
| Focus | Anneau de focus visible (≥ 2 px, contraste ≥ 3:1 avec le fond) sur tout élément interactif, y compris cartes et poignées. |
| Glisser-déposer | Jamais le seul moyen : « Choisir des fichiers… » (Ctrl+O) ; réordonnancement alternatif Alt+↑/↓ (WCAG 2.5.7). |
| Narrateur Windows | Chaque contrôle a un nom, un rôle, une valeur ; cartes annoncées « mariage_1998.avi, en cours, 38 pour cent, il reste environ 9 minutes » ; changements d'état annoncés par région vive polie ; progression annoncée ≤ 1 fois / 5 s ; erreurs annoncées en assertif. |
| Contrastes | Texte ≥ 4,5:1 (3:1 pour grand texte), composants et focus ≥ 3:1, dans les thèmes clair **et** sombre. Pas de texte informatif sur une image de fond sans voile. |
| Couleur | Jamais seule pour porter un sens : échec = icône + libellé, pas seulement rouge ; terminé = coche + texte. |
| Mise à l'échelle | Rendu net et sans troncature à 100, 125, 150, 175, 200 % ; le texte suit la taille de texte Windows (jusqu'à 225 %) ; cibles tactiles/clic ≥ 32 × 32 px logiques (24 px minimum absolu, WCAG 2.5.8). |
| Contraste élevé Windows | En mode contraste élevé, l'app adopte les couleurs système (texte, fond, lien, sélection, désactivé) ; bordures visibles sur cartes et boutons ; pas d'information portée par la seule transparence. |
| Mouvement | Respecte « Effets d'animation » désactivé de Windows : transitions remplacées par fondus instantanés ; aucune animation clignotante > 3 Hz ; aucune animation en boucle infinie sauf la progression. |
| Délais | Les bandeaux « Rétablir » restent 5 s mais sont aussi accessibles par Ctrl+Z ; pour le Narrateur, la durée est prolongée à 10 s tant que le focus est sur le bandeau. |
| Texte | Messages qui reviennent à la ligne plutôt que d'être tronqués ; info-bulle jamais seule porteuse d'une information indispensable. |
| Langue | `lang = fr` déclaré ; durées lues naturellement (« 3 minutes 5 secondes »). |
| Erreurs | Identifiées en texte, avec suggestion de correction (WCAG 3.3.3). |

## Critères de test transverses (à reporter dans les stories par le scrum-master)
- Tout parcours P-01 à P-08 réalisable sans souris.
- Aucun texte tronqué à 200 % et à 760 px de large.
- Lecture complète d'une carte par le Narrateur dans chacun de ses 7 états.
- Mode contraste élevé : tous les contrôles et états distinguables.
- « Réduire les animations » : aucune transition > 0 ms de mouvement spatial.
