# Enhance — Parcours utilisateurs du MVP

Statut : v0.1 (ux-designer), 2026-10-21.
Décisions PO intégrées : interpolation vers 60 i/s uniquement pour les sources < 50 i/s (24, 25, 30 → 60) ; sources 50 et 60 i/s gardent leur cadence ; sortie MP4 avec bascule automatique en MKV + message si des sous-titres sont incompatibles ; qualité d'abord, vitesse affichée ; interface en français ; 100 % locale.

Ton des textes : **vouvoiement discret** et formulations impersonnelles (comme les apps Apple en français) ; boutons à l'infinitif ou au nom (« Démarrer », « Annuler »). Jamais de jargon de codec en façade : « HEVC » / « AV1 » apparaissent uniquement dans un contrôle intitulé « Format » avec une description humaine.

Légende : **P1** Carol, **P2** Hélène, **P3** Maxime. [LT] = point d'accroche pour la vision long terme (non dessiné).

---

## P-01 Premier lancement et prérequis

**Hélène installe Enhance et l'ouvre pour la première fois.**

1. Elle ouvre Enhance. La fenêtre apparaît en < 2 s, zone de dépôt vide, aucun assistant ni écran d'accueil à passer.
2. En arrière-plan (< 1 s), Enhance détecte la carte et le pilote. Si tout va bien : rien n'est affiché, sauf une ligne discrète en bas à gauche « GeForce RTX 5060 · prête ».
3. Elle dépose un fichier : elle est déjà dans le parcours P-02.

**Cas d'erreur / limites**

| Situation | Ce que voit l'utilisateur | Action proposée |
|---|---|---|
| Pilote trop ancien | Bandeau en haut de la zone de dépôt : « Le pilote graphique est trop ancien (version 551). Enhance a besoin de la version 580 ou plus récente. » | Bouton « Comment mettre à jour » (ouvre une feuille d'aide locale, texte statique, aucun lien réseau automatique) ; copie de l'adresse du site NVIDIA à la demande. L'app reste ouverte, dépôt possible, **Démarrer désactivé**. |
| Carte non compatible (pas RTX 50, autre marque) | « Cette carte graphique n'est pas compatible. Enhance nécessite une GeForce RTX série 50. » | Mode lecture seule : on peut ajouter des fichiers et parcourir, pas traiter. |
| Aucune carte NVIDIA détectée | « Aucune carte NVIDIA n'a été trouvée. » | Idem, + conseil de vérifier le pilote. |
| Composant manquant (DLL, modèle) | « Un composant d'Enhance est manquant ou endommagé. Réinstallez l'application. » + « Copier le détail » | Détail technique en dépliant « Détails ». |
| Première fois, dossier de sortie | Aucun choix demandé : sortie à côté de la source (voir P-07). | -- |

Règle : jamais de blocage plein écran ; le bandeau reste, l'app reste explorable.

---

## P-02 Glisser-déposer

**Maxime veut ajouter 12 captures.**

1. Il fait glisser 12 fichiers sur la fenêtre.
2. **Retour visuel immédiat (< 100 ms)** : la zone de dépôt s'illumine (bordure accentuée, léger agrandissement 1,02, icône qui « s'ouvre »), le texte devient « Relâcher pour ajouter 12 vidéos ». Si le survol contient un fichier non vidéo, le texte reste neutre (le tri se fait au dépôt).
3. Il relâche. Les cartes arrivent une à une (cascade de 30 ms, ressort 250 ms) avec vignette, nom, durée, résolution et cadence. Pendant la lecture du fichier, la carte est en état « Lecture… » (squelette animé).
4. Le bouton principal passe de grisé à « Démarrer (12) ».
5. Un petit son/haptique n'existe pas sous Windows : le « moment agréable » est visuel (cascade + léger rebond du bouton).

**Fichiers refusés** : les autres sont ajoutés normalement ; sous la zone (ou sous la file) un message non bloquant apparaît 6 s puis se range dans un lien « 2 fichiers n'ont pas été ajoutés » (dépliable, avec raison par fichier).

| Cas | Message |
|---|---|
| Fichier non vidéo (.docx, .jpg) | « « notes.docx » n'est pas une vidéo. Il n'a pas été ajouté. » |
| Fichier illisible dès le probe | « « film.avi » n'a pas pu être lu. Le fichier est peut-être endommagé. » (la carte n'est pas ajoutée) |
| Doublon (même chemin déjà dans la file) | « « film.mp4 » est déjà dans la liste. » La carte existante clignote une fois (surbrillance 400 ms). |
| Déjà en 4K ou plus | Carte ajoutée avec une pastille « Déjà en 4K » ; au lancement : copie sans agrandissement avec message (voir ENH-009). |
| Dossier déposé (MVP) | « Les dossiers ne sont pas encore pris en charge. Déposez les fichiers qu'il contient. » [LT : ENH-037] |
| Lecteur réseau / fichier verrouillé | « Impossible d'accéder à « x.mkv ». Il est peut-être utilisé par un autre programme. » |

**Alternative clavier** : bouton « Choisir des fichiers… » (Ctrl+O) ouvre le sélecteur Windows standard ; Ctrl+V colle des fichiers copiés dans l'Explorateur [H, à confirmer faisabilité].

**État vide** : illustration sobre (cadre vidéo + flèche vers le haut), « Déposez vos vidéos ici », sous-titre « Enhance les convertit en 4K fluide, sur votre ordinateur. Rien n'est envoyé sur Internet. », bouton « Choisir des fichiers… ».

---

## P-03 Choix du préréglage et du format (HEVC / AV1)

**Hélène ne veut rien choisir ; Carol veut AV1.**

1. Une fois la file non vide, un **panneau de préréglage** (barre en bas ou colonne droite selon la largeur, voir wireframes) affiche trois choix en contrôle segmenté : **Qualité max · Équilibré · Rapide**. **Équilibré** est sélectionné.
2. Sous le contrôle : une ligne de description du préréglage choisi, et l'estimation « Environ 42 min pour l'ensemble de la liste » (avec détail par fichier sur chaque carte : « ≈ 6 min »).
3. Un second contrôle segmenté « Format » : **Compatible** (HEVC) · **Compact** (AV1). Défaut : Compatible. Sous le contrôle, une phrase : « Compatible : lisible presque partout. Compact : fichiers plus petits, nécessite un lecteur récent. » Les sigles HEVC / AV1 sont visibles en légende secondaire grise, pour Carol.
4. Si AV1 non disponible sur la carte : option grisée, info-bulle « Cette carte ne sait pas produire ce format. » (ENH-011).
5. Les choix s'appliquent à toute la liste et sont mémorisés. Changer de préréglage met à jour les estimations avec une animation de nombre (150 ms).
6. **Bulle d'information sur l'interpolation** (pas de réglage MVP) : sur la carte d'un fichier à 24/25/30 i/s, ligne « 30 → 60 images/s » ; à 50/60 i/s : « 60 images/s conservées » (aucune interpolation). Sources entre 31 et 49 i/s : interpolées vers 60 (**décidé par le PO le 2026-10-21** : toute source < 50 i/s est interpolée).
7. Préréglage « Rapide » : description honnête « Plus rapide, finesse un peu moindre » (la décision PO « qualité d'abord » interdit de promettre mieux).
8. [LT] « Réglages avancés » (ENH-035) : absent au MVP (pas de bouton grisé « Bientôt »), l'emplacement est réservé en bas du panneau.

**Conteneur** : jamais demandé. MP4 par défaut. Si une piste de sous-titres n'est pas compatible MP4 (ex. PGS), la carte reçoit une pastille « MKV » et un message avant le lancement : « Ce film contient des sous-titres qui ne rentrent pas dans un fichier MP4. Il sera enregistré au format MKV pour les conserver. » (informatif, sans bouton de confirmation ; lien « Détails » liste les pistes).

---

## P-04 Lancement

1. Un clic sur **Démarrer** (ou Ctrl+Entrée). Pas de confirmation.
2. Prévérifications rapides (< 1 s) : espace disque estimé, VRAM libre, pilote. Si l'espace est insuffisant : bandeau « Il manque environ 14 Gio sur le disque D:. Libérez de l'espace ou choisissez un autre dossier dans les réglages. » **Démarrer** reste actif pour les fichiers qui tiennent ; ceux qui ne tiennent pas sont marqués (voir P-08).
3. Le bouton devient **Pause** ; la première carte passe en « En cours » ; la liste se réorganise avec animation (la carte en cours se déploie : barre, images/s, temps restant).
4. Le Mica / fond de fenêtre reste calme : aucune animation continue gratuite, seule la barre bouge.

---

## P-05 Suivi de la progression

1. La carte en cours affiche : barre de progression, pourcentage, « 38 images/s », « Il reste environ 14 min » et, en dessous, « Étape : Agrandissement en 4K » (étape lisible, pas de jargon : *Lecture · Agrandissement · Fluidité · Enregistrement*).
2. Une **ligne de synthèse globale** en pied de fenêtre : « 3 sur 12 terminées · il reste environ 2 h 10 ».
3. Honnêteté : pendant les ~10 premières secondes, le temps restant affiche « Estimation en cours… » plutôt qu'un chiffre faux. Si la vitesse chute, le temps se met à jour en douceur (lissage), sans sauts.
4. Vitesse sous la durée réelle de la vidéo : mention « plus lent que la lecture » *non alarmante* (« 0,6× la vitesse de lecture »), visible dans un survol/détail, pas en façade.
5. Pendant le traitement, l'utilisateur peut ajouter d'autres fichiers (ils rejoignent la file) ; le préréglage ne s'applique qu'aux jobs **non commencés** (les cartes déjà lancées portent leur préréglage en légende).
6. Le Narrateur annonce la progression au plus 1 fois / 5 s et lors des changements de fichier.

---

## P-06 Pause, annulation, réordonnancement

**Pause** : bouton Pause / Espace. La carte passe en « En pause » (barre grisée, pas d'animation), la VRAM est relâchée (<2 s). Reprise : même bouton. Texte de la carte : « En pause à 38 % ».

**Annulation (pas de boîte de confirmation)** : Échap ou bouton « Annuler » sur la carte. La carte se replie en « Annulé » avec **un bandeau de retour** « Traitement annulé. [Rétablir] » pendant 5 s ; si l'utilisateur clique sur Rétablir dans ces 5 s, le job reprend **depuis le début** (le traitement partiel est supprimé à l'expiration seulement, donc rétablir = relancer proprement ; texte du bouton : « Reprendre depuis le début »). Après 5 s, le fichier partiel est supprimé et la carte reste en « Annulé » avec un bouton « Relancer ».
*À valider avec le gpu-engineer* : l'annulation dans les 5 s peut-elle reprendre sans perte ? Sinon, le libellé reste « Relancer ».

**Retrait d'un fichier de la file** : bouton ✕ ou Suppr. La carte disparaît, avec « Retiré de la liste. [Annuler] » 5 s (le fichier d'origine n'est **jamais** supprimé du disque, c'est dit dans l'info-bulle).

**Réordonnancement** : glisser une carte « En attente » par sa poignée (⋮⋮) ; les autres s'écartent (ressort 250 ms). Clavier : sélectionner la carte, Alt+↑ / Alt+↓. La carte « En cours » ne bouge pas (poignée absente, info-bulle « Le fichier en cours ne peut pas être déplacé »).

**Fermeture de l'app pendant un traitement** : une seule exception à la règle « pas de confirmation » car perte de travail. **Décidé par le PO le 2026-10-21** : la croix réduit la fenêtre dans la barre des tâches sans arrêter le traitement ; Ctrl+Q ouvre une petite feuille « Quitter et abandonner le traitement en cours ? ». Sans traitement en cours, Ctrl+Q quitte directement.

---

## P-07 Fin du traitement

1. Quand un fichier est terminé : la carte se replie avec un **moment agréable** : la barre se remplit, se transforme en coche (animation 300 ms, ressort), la vignette gagne un fin liseré accent ; texte « Terminé en 18 min · 4K 60 i/s · 2,3 Gio ».
2. Boutons sur la carte : **Lire** (application par défaut) et **Afficher dans le dossier** (Explorateur, fichier sélectionné). [LT : « Comparer » à côté, ENH-033].
3. Fin de la file : notification Windows discrète (toast local) + résumé en pied : « 12 vidéos terminées. » avec « Tout afficher dans le dossier ». Si des échecs : « 10 terminées, 2 en échec. » + bouton « Voir les erreurs » qui fait défiler jusqu'à la première.
4. Sortie : à côté de la source, nom `Film_Enhance_4K60.mp4` ; jamais d'écrasement (`_2`). Message de rassurance, en détail de la carte : « Votre fichier d'origine n'a pas été modifié. »
5. Fichier produit supprimé/déplacé : boutons grisés, texte « Le fichier n'est plus à cet emplacement. »
6. Effacer les terminés : bouton « Vider les terminées » (annulable 5 s).

---

## P-08 Erreurs en cours de traitement

Principe : un échec **n'arrête jamais la file**. Chaque erreur dit *ce qui s'est passé*, *si les autres fichiers sont affectés* et *quoi faire*. Détail technique replié (« Détails »), copiable.

| Cas | Message de la carte (état « Échec ») | Action |
|---|---|---|
| Fichier malformé / tronqué | « Ce fichier est endommagé à partir de 12 min 40 s et n'a pas pu être traité. Les autres vidéos continuent. » | « Réessayer », « Retirer ». |
| Format non pris en charge | « Ce format vidéo n'est pas pris en charge. » | « Retirer ». |
| VRAM insuffisante | « La carte graphique n'a pas assez de mémoire pour ce fichier. Fermez les jeux ou autres programmes graphiques, puis réessayez. » | « Réessayer » ; sur Hélène (8 Go) : suggestion « Essayez le préréglage Rapide ». |
| Disque plein (avant) | « Espace insuffisant sur D: (il manque 14 Gio). » | « Changer de dossier de sortie » (ouvre les réglages) ; « Réessayer ». |
| Disque plein (pendant) | « Le disque D: est plein. Le fichier partiel a été supprimé. » | idem |
| Pilote / carte indisponible (mise en veille, pilote réinitialisé) | « La carte graphique ne répond plus. Le traitement de ce fichier a été arrêté. » | « Réessayer ». |
| Écriture impossible (droits) | « Enhance ne peut pas écrire dans ce dossier. » | « Choisir un autre dossier ». |
| Fichier source illisible (`SourceUnreadable` : verrouillé par une autre application ou sans droit de lecture) | « Enhance n'arrive pas à ouvrir ce fichier. Il est peut-être utilisé par une autre application, ou vous n'avez pas le droit de le lire. Les autres vidéos continuent. » Détails repliés : chemin + code Windows. | « Réessayer » (après fermeture de l'autre application), « Afficher dans l'Explorateur », « Retirer ». Aucun changement automatique de la file ; pas de nouvelle tentative silencieuse. |
| Source supprimée entre-temps | « Le fichier d'origine n'est plus là. » | « Retirer ». |
| Erreur inattendue | « Un problème inattendu est survenu. » + « Copier le détail » + « Afficher les journaux » | -- |

Un fichier produit incomplet n'est jamais laissé sur le disque.

### P-08b Première utilisation : préparation de l'interpolation (une seule fois)

Au premier traitement avec génération d'images (~1 à 3 min [H], puis mise en cache : plus jamais d'attente visible).
- **Carte du fichier** (état « En cours », avant la 1re image) : titre « Préparation de la fluidité… », sous-texte « Première utilisation seulement. Cela prend 1 à 3 minutes, puis ce sera instantané. » Barre de progression **indéterminée** (pas de pourcentage, pas d'images/s, pas de temps restant : on ne les invente pas). Si la préparation dépasse 5 min : le sous-texte devient « Cela prend plus de temps que prévu. Enhance travaille toujours. »
- **Barre d'état** : « Préparation de la première utilisation… » (même indétermination). Les autres cartes restent « En attente ».
- **Fin** : la carte bascule en douceur vers la progression normale (images/s, temps restant) ; aucun message de confirmation.
- **Annuler** (bouton ou Échap/Ctrl+Z selon le modèle d'annulation) : arrêt immédiat, la carte repasse « En attente » avec « Préparation interrompue. Elle reprendra au prochain lancement. » Rien de partiel n'est conservé dans le cache ; aucun fichier de sortie. Un « Réessayer/Démarrer » relance la préparation depuis le début.
- **Échec** (disque plein, droits) : message d'erreur standard de P-08 ; le traitement sans génération d'images reste possible.

---

## P-09 Réglages (accès occasionnel)

Ouverts par Ctrl+, : dossier de sortie, thème (Sombre / Clair / Système), « Afficher les journaux », « À propos » (versions, licences tierces, « Aucune donnée n'est envoyée »). Aucun réglage de codec. Voir `ecrans.md`.

---

## Points d'accroche pour la vision long terme (non dessinés)

| Fonction | Où elle s'insère |
|---|---|
| Comparer avant / après (ENH-033) | Bouton « Comparer » sur la carte Terminée ; ouvre une **vue de comparaison** plein cadre remplaçant temporairement la liste (retour par Échap). |
| Aperçu rapide (ENH-036) | Bouton « Aperçu de 5 s » dans la barre de la carte « En attente » ; produit une carte-résultat temporaire. |
| Options avancées (ENH-035) | Disclosure « Avancé » repliée en bas du panneau de préréglage ; emplacement réservé. |
| Préréglages personnalisés (ENH-034) | Entrée « Personnalisé… » en fin du contrôle segmenté (devient un menu). |
| Dossiers entiers (ENH-037) | La zone de dépôt accepte déjà les dossiers ; MVP : message poli. Groupe repliable dans la liste. |
| Reprise après plantage (ENH-038) | Au lancement : bandeau « Un traitement a été interrompu. Reprendre ? » (aucune boîte). La file est déjà persistée (ENH-022). |
| SDR → HDR (ENH-032) | Interrupteur « Couleurs HDR » dans le panneau préréglage. |
